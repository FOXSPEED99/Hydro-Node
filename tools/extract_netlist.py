#!/usr/bin/env python3
"""
Extract a netlist from a Hydro Node Altium schematic (.SchDoc).

The sheet carries almost no net labels and no power ports, so connectivity has
to be rebuilt from wire geometry, junction dots and pin end points. Re-run this
after any schematic edit and diff the result against SCHEMATIC-CHECK.md.

    pip install olefile
    python3 tools/extract_netlist.py "path/to/Hydro Node Schematic.SchDoc"

PIN GEOMETRY - THIS IS THE PART THAT IS EASY TO GET WRONG.
A pin record stores Location.X/Y (where the pin meets the component body) plus
PinLength and an orientation in the low two bits of PinConglomerate. The
electrical end - the end a wire attaches to - is:

    Location + PinLength * direction

Treating Location itself as the connection point, or testing both ends, merges
nets that are not connected. On this sheet, testing both ends shorted C1 and R7
across their own two pins because a wire ran past the far end of J1/J2's pins.
The self-check below catches that class of error: no two-pin part may have both
pins on one net.
"""

import struct
import sys
from collections import defaultdict

import olefile

DEFAULT = ("Hydro Node Device/Hydro Node Parts & Schematic/"
           "Schematic/Hydro Node Schematic.SchDoc")

RECORD_COMPONENT = "1"
RECORD_PIN = "2"
RECORD_NETLABEL = "25"
RECORD_WIRE = "27"
RECORD_JUNCTION = "29"
RECORD_DESIGNATOR = "34"
RECORD_PARAMETER = "41"

# low 2 bits of PinConglomerate: 0=right 1=up 2=left 3=down
DIRECTIONS = [(1, 0), (0, 1), (-1, 0), (0, -1)]


def read_records(path):
    blob = olefile.OleFileIO(path).openstream("FileHeader").read()
    out, i = [], 0
    while i + 4 <= len(blob):
        length = struct.unpack("<I", blob[i:i + 4])[0] & 0xFFFFFF
        i += 4
        raw = blob[i:i + length].rstrip(b"\x00").decode("latin-1")
        i += length
        rec = {}
        for kv in raw.split("|"):
            if "=" in kv:
                k, v = kv.split("=", 1)
                rec[k] = v
        out.append(rec)
    return out


def owner(rec):
    try:
        return int(rec.get("OwnerIndex", -1))
    except ValueError:
        return -1


def on_segment(point, seg):
    (x1, y1), (x2, y2) = seg
    x, y = point
    if (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1) != 0:
        return False
    return min(x1, x2) <= x <= max(x1, x2) and min(y1, y2) <= y <= max(y1, y2)


def build(records):
    comps = {}
    for idx, rec in enumerate(records):
        if rec.get("RECORD") == RECORD_COMPONENT:
            # the designator and parameter records point at the component's
            # index minus one
            comps[idx - 1] = {"lib": rec.get("LibReference"), "des": None,
                              "val": None, "pins": []}
    for rec in records:
        oi = owner(rec)
        if oi not in comps:
            continue
        if rec.get("RECORD") == RECORD_DESIGNATOR:
            comps[oi]["des"] = rec.get("Text")
        elif rec.get("RECORD") == RECORD_PARAMETER and rec.get("Name") == "Comment":
            comps[oi]["val"] = rec.get("Text")

    pins = []
    for rec in records:
        if rec.get("RECORD") != RECORD_PIN:
            continue
        x, y = int(rec["Location.X"]), int(rec["Location.Y"])
        length = int(rec.get("PinLength", 0))
        dx, dy = DIRECTIONS[int(rec.get("PinConglomerate", 0)) & 3]
        oi = owner(rec)
        comps.get(oi, {}).get("pins", []).append(len(pins))
        pins.append({"owner": oi, "des": rec.get("Designator"),
                     "name": rec.get("Name"),
                     "end": (x + dx * length, y + dy * length)})

    segs = []
    for rec in records:
        if rec.get("RECORD") != RECORD_WIRE:
            continue
        n = int(rec["LocationCount"])
        pts = [(int(rec["X%d" % k]), int(rec["Y%d" % k])) for k in range(1, n + 1)]
        segs.extend(zip(pts, pts[1:]))

    junctions = {(int(r["Location.X"]), int(r["Location.Y"]))
                 for r in records if r.get("RECORD") == RECORD_JUNCTION}
    labels = [(r.get("Text"), (int(r["Location.X"]), int(r["Location.Y"])))
              for r in records if r.get("RECORD") == RECORD_NETLABEL]
    return comps, pins, segs, junctions, labels


def solve(pins, segs, junctions, labels):
    parent = {}

    def find(a):
        parent.setdefault(a, a)
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    def union(a, b):
        parent[find(a)] = find(b)

    for i in range(len(segs)):
        find(("S", i))

    for i, s in enumerate(segs):
        for j, t in enumerate(segs):
            if i >= j:
                continue
            if set(s) & set(t):                       # shared endpoint
                union(("S", i), ("S", j))
            elif any(p in (t[0], t[1]) or on_segment(p, t) for p in s) or \
                 any(p in (s[0], s[1]) or on_segment(p, s) for p in t):
                union(("S", i), ("S", j))             # T-junction
            else:
                for jp in junctions:                  # crossing with a dot
                    if on_segment(jp, s) and on_segment(jp, t):
                        union(("S", i), ("S", j))
                        break

    attached = defaultdict(bool)
    for k, pin in enumerate(pins):
        for i, s in enumerate(segs):
            if on_segment(pin["end"], s):
                union(("P", k), ("S", i))
                attached[k] = True
        for k2 in range(k + 1, len(pins)):            # pin touching pin
            if pin["end"] == pins[k2]["end"]:
                union(("P", k), ("P", k2))
                attached[k] = attached[k2] = True

    named = defaultdict(set)
    for text, pt in labels:
        for i, s in enumerate(segs):
            if on_segment(pt, s):
                named[find(("S", i))].add(text)

    nets = defaultdict(list)
    for k in range(len(pins)):
        if attached[k]:
            nets[find(("P", k))].append(k)
    return nets, named, attached, find


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT
    comps, pins, segs, junctions, labels = build(read_records(path))
    nets, named, attached, find = solve(pins, segs, junctions, labels)

    def label(k):
        c = comps.get(pins[k]["owner"], {})
        return "%s.%s(%s)" % (c.get("des"), pins[k]["des"], pins[k]["name"])

    print("=== COMPONENTS ===")
    for _, c in sorted(comps.items(), key=lambda kv: str(kv[1]["des"])):
        print("%-6s %-12s %-40s %d pins" %
              (c["des"], c["val"], c["lib"], len(c["pins"])))

    print("\n=== NETS ===")
    order = sorted(nets.items(), key=lambda kv: -len(kv[1]))
    for n, (root, members) in enumerate(order, 1):
        names = "/".join(sorted(named.get(root, set())))
        tag = ' "%s"' % names if names else ""
        print("N%02d%-9s [%d]: %s" %
              (n, tag, len(members), ", ".join(sorted(label(k) for k in members))))

    print("\n=== PINS WITH NO WIRE ATTACHED ===")
    for _, c in sorted(comps.items(), key=lambda kv: str(kv[1]["des"])):
        dead = [pins[k] for k in c["pins"] if not attached[k]]
        if dead:
            print("  %-6s %-12s %s" % (c["des"], c["val"],
                  ", ".join("%s(%s)" % (p["des"], p["name"]) for p in dead)))

    # Self-check. A two-pin passive with both pins on one net means the pin
    # geometry was misread, or the sheet really does short that part out.
    print("\n=== SELF-CHECK ===")
    shorted = []
    for _, c in comps.items():
        if len(c["pins"]) != 2:
            continue
        k1, k2 = c["pins"]
        if attached[k1] and attached[k2] and find(("P", k1)) == find(("P", k2)):
            shorted.append(c["des"])
    if shorted:
        print("  FAIL - two-pin parts shorted across themselves: %s" %
              ", ".join(sorted(shorted)))
        return 1
    print("  OK - no two-pin part is shorted across its own pins")
    return 0


if __name__ == "__main__":
    sys.exit(main())
