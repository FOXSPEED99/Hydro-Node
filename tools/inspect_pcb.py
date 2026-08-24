#!/usr/bin/env python3
"""Parse an Altium .PcbDoc: layers, tracks, pads, vias, polygons, components."""
import olefile, struct, sys
from collections import Counter, defaultdict

MIL = 10000.0  # internal units per mil

LAYER = {1:"Top", 32:"Bottom", 33:"TopOverlay", 34:"BottomOverlay",
         35:"TopPaste", 36:"BottomPaste", 37:"TopSolder", 38:"BottomSolder",
         55:"DrillGuide", 56:"KeepOut", 71:"DrillDrawing", 72:"MultiLayer",
         73:"ConnectLayer", 74:"MultiLayer"}
for i in range(2, 32):  LAYER[i] = "Mid%d" % (i-1)
for i in range(57, 71): LAYER[i] = "Mech%d" % (i-56)

def lname(n): return LAYER.get(n, "layer%d" % n)

o = olefile.OleFileIO(sys.argv[1] if len(sys.argv) > 1 else 'pcb.PcbDoc')

def stream(n):
    try: return o.openstream(n).read()
    except Exception: return b""

def textrecs(name):
    """Streams of [u32 len][payload\\0] holding |KEY=VAL| strings."""
    d = stream(name); out = []; i = 0
    while i + 4 <= len(d):
        ln = struct.unpack('<I', d[i:i+4])[0]; i += 4
        s = d[i:i+ln].rstrip(b'\x00').decode('latin-1'); i += ln
        rec = {}
        for kv in s.split('|'):
            if '=' in kv:
                k, v = kv.split('=', 1); rec[k.upper()] = v
        if rec: out.append(rec)
    return out

def binrecs(name):
    """Streams of [u8 type][u32 len][payload]."""
    d = stream(name); out = []; i = 0
    while i + 5 <= len(d):
        t = d[i]; ln = struct.unpack('<I', d[i+1:i+5])[0]
        out.append((t, d[i+5:i+5+ln])); i += 5 + ln
    return out

def u16(b, k): return struct.unpack('<H', b[k:k+2])[0]
def i32(b, k): return struct.unpack('<i', b[k:k+4])[0]

# ---- nets ------------------------------------------------------------
nets = [r.get('NAME', '?') for r in textrecs('Nets6/Data')]
def netname(n): return nets[n] if 0 <= n < len(nets) else ("<none>" if n == 0xFFFF else "?%d" % n)

# ---- components ------------------------------------------------------
comps = textrecs('Components6/Data')
cdes = [c.get('SOURCEDESIGNATOR', c.get('NAME', '?')) for c in comps]
def cname(n): return cdes[n] if 0 <= n < len(cdes) else "-"

# ---- tracks ----------------------------------------------------------
tracks = []
for t, b in binrecs('Tracks6/Data'):
    if len(b) < 33: continue
    tracks.append(dict(layer=b[0], net=u16(b, 3), comp=u16(b, 7),
                       x1=i32(b, 13), y1=i32(b, 17), x2=i32(b, 21), y2=i32(b, 25),
                       w=i32(b, 29)))

# ---- arcs ------------------------------------------------------------
arcs = []
for t, b in binrecs('Arcs6/Data'):
    if len(b) < 45: continue
    arcs.append(dict(layer=b[0], net=u16(b, 3), comp=u16(b, 7),
                     cx=i32(b, 13), cy=i32(b, 17), r=i32(b, 21), w=i32(b, 41)))

# ---- vias ------------------------------------------------------------
vias = []
for t, b in binrecs('Vias6/Data'):
    if len(b) < 33: continue
    vias.append(dict(layer=b[0], net=u16(b, 3), comp=u16(b, 7),
                     x=i32(b, 13), y=i32(b, 17), dia=i32(b, 21), hole=i32(b, 25),
                     lo=b[29], hi=b[30]))

# ---- pads ------------------------------------------------------------
pads = []
for t, b in binrecs('Pads6/Data'):
    # pad record: u8 namelen-block then a fixed body; name is length-prefixed
    if len(b) < 10: continue
    nl = struct.unpack('<I', b[0:4])[0]
    name = b[4:4+nl].rstrip(b'\x00').decode('latin-1')
    p = 4 + nl
    # three sub-blocks each [u32 len][data]; the main one is the 3rd
    blocks = []
    q = p
    for _ in range(4):
        if q + 4 > len(b): break
        ln = struct.unpack('<I', b[q:q+4])[0]; q += 4
        blocks.append(b[q:q+ln]); q += ln
    main = None
    for blk in blocks:
        if len(blk) >= 100: main = blk; break
    if main is None: continue
    pads.append(dict(name=name, layer=main[0], net=u16(main, 3), comp=u16(main, 7),
                     x=i32(main, 13), y=i32(main, 17),
                     xs=i32(main, 21), ys=i32(main, 25), hole=i32(main, 45)))

# ---- polygons --------------------------------------------------------
polys = textrecs('Polygons6/Data')

# ---- regions (poured copper) ----------------------------------------
regions = binrecs('Regions6/Data')
shaperegions = binrecs('ShapeBasedRegions6/Data')

# ---- board -----------------------------------------------------------
board = textrecs('Board6/Data')
b0 = board[0] if board else {}

print("=" * 78)
print("BOARD")
print("=" * 78)
for k in ('FILENAME', 'DATE', 'TIME', 'LAYERSTACK', 'ORIGINX', 'ORIGINY'):
    if k in b0: print("  %-14s %s" % (k, b0[k]))
lstack = [(k, v) for k, v in b0.items() if k.startswith('LAYER') and k.endswith('NAME') and v]
used = sorted({t['layer'] for t in tracks} | {a['layer'] for a in arcs} |
              {v['layer'] for v in vias} | {p['layer'] for p in pads})
print("  layers in use:", ', '.join("%s(%d)" % (lname(l), l) for l in used))

# board outline from Mech/KeepOut/BoardOutline tracks
out = [t for t in tracks if lname(t['layer']).startswith(('Mech', 'KeepOut'))]
allx = [c for t in tracks for c in (t['x1'], t['x2'])]
ally = [c for t in tracks for c in (t['y1'], t['y2'])]
if out:
    ox = [c for t in out for c in (t['x1'], t['x2'])]
    oy = [c for t in out for c in (t['y1'], t['y2'])]
    print("  outline layers extent: %.1f x %.1f mm" %
          ((max(ox)-min(ox))/MIL*0.0254, (max(oy)-min(oy))/MIL*0.0254))
print("  all-copper extent: %.1f x %.1f mm" %
      ((max(allx)-min(allx))/MIL*0.0254, (max(ally)-min(ally))/MIL*0.0254))

print()
print("=" * 78)
print("COUNTS")
print("=" * 78)
print("  components %d   nets %d   pads %d   tracks %d   arcs %d   vias %d" %
      (len(comps), len(nets), len(pads), len(tracks), len(arcs), len(vias)))
print("  polygons %d   regions %d   shape-based regions %d" %
      (len(polys), len(regions), len(shaperegions)))

print()
print("=" * 78)
print("TRACKS BY LAYER")
print("=" * 78)
bylayer = Counter(lname(t['layer']) for t in tracks)
for l, n in bylayer.most_common():
    ws = Counter(round(t['w']/MIL, 2) for t in tracks if lname(t['layer']) == l)
    sig = l in ('Top', 'Bottom') or l.startswith('Mid')
    print("  %-14s %4d tracks   widths(mil): %s%s" %
          (l, n, dict(sorted(ws.items())), "   <-- signal" if sig else ""))

print()
print("=" * 78)
print("VIAS")
print("=" * 78)
if not vias:
    print("  NONE")
for v in vias:
    print("  net=%-18s x=%.1f y=%.1f mil  pad=%.1f mil  hole=%.1f mil  layers %d-%d" %
          (netname(v['net']), v['x']/MIL, v['y']/MIL, v['dia']/MIL, v['hole']/MIL, v['lo'], v['hi']))

print()
print("=" * 78)
print("POLYGONS (copper pour)")
print("=" * 78)
if not polys:
    print("  NONE")
for i, p in enumerate(polys, 1):
    vs = [k for k in p if k.startswith('VX')]
    print("  #%d layer=%-8s net=%-14s type=%s hatch=%s pourover=%s removedead=%s "
          "track=%s grid=%s vertices=%d" %
          (i, p.get('LAYER'), p.get('NET', '<none>'), p.get('POLYGONTYPE'),
           p.get('HATCHSTYLE'), p.get('POUROVER'), p.get('REMOVEDEAD'),
           p.get('TRACKWIDTH'), p.get('GRIDSIZE'), len(vs)))
    xs = [float(p['VX%d' % k].replace('mil', '')) for k in range(len(vs)) if 'VX%d' % k in p]
    ys = [float(p['VY%d' % k].replace('mil', '')) for k in range(len(vs)) if 'VY%d' % k in p]
    if xs:
        print("        extent %.1f x %.1f mm" %
              ((max(xs)-min(xs))*0.0254, (max(ys)-min(ys))*0.0254))

print()
print("=" * 78)
print("NETS: routing per net")
print("=" * 78)
tl = defaultdict(lambda: defaultdict(float))
for t in tracks:
    if t['net'] != 0xFFFF and lname(t['layer']) in ('Top', 'Bottom'):
        d = ((t['x2']-t['x1'])**2 + (t['y2']-t['y1'])**2) ** .5
        tl[netname(t['net'])][lname(t['layer'])] += d/MIL
padnets = Counter(netname(p['net']) for p in pads if p['net'] != 0xFFFF)
print("  %-20s %5s %10s %10s" % ("net", "pads", "top(mil)", "bottom(mil)"))
for n in sorted(set(list(tl.keys()) + list(padnets.keys()))):
    print("  %-20s %5d %10.0f %10.0f" %
          (n, padnets.get(n, 0), tl[n].get('Top', 0), tl[n].get('Bottom', 0)))

unrouted = [n for n in padnets if padnets[n] >= 2 and n not in tl]
print("\n  nets with 2+ pads and NO copper track:", unrouted or "none")
