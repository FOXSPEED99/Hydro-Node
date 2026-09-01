# PCB CHECK — Hydro_Node_PCB.PcbDoc

> **Superseded in part — read `PCB-FIXES.md` first.** Three findings below were corrected on
> 2026-08-24: the Ra-02 footprint is **correct** (you fit the breakout board, not the bare module);
> only **C6** needs moving, not C7/C8/C9; and the missing stitching vias matter less than stated,
> because 20 through-hole ground pads already tie the layers. The one item that stands as written
> is the top-layer routing slotting the pour.

Checked 2026-08-24. Board file dated 8/24/2026 2:47 PM.
Geometry read straight out of the file — every number below is measured, not estimated.

---

## VERDICT

**One fault stops the board. Three more will hurt the radio. The rest is small.**

The routing itself is clean: **every net is fully connected**, minimum clearance between different nets is **0.351 mm**, and there is now a **solid ground pour over the whole board**. That last one closes HW-004, the blocker that has been open since the first review.

| | | |
|---|---|---|
| ❌ | **U3 footprint is 25.40 mm between the pad rows. The Ra-02 is 16 mm wide.** | The module physically cannot fit. Boards would be scrap. |
| ❌ | Radio and latch decoupling sit **8–15 mm** from their chips | The caps cannot do their job at that distance |
| ❌ | **1 via on the whole board**, and it is on BATT+ | No ground stitching at all |
| ❌ | **295 mm of top-layer routing cuts slots through the ground pour** | Return current has to detour around every slot |
| ⚠️ | Remove Dead Copper is off · J1 boss pad has zero annular ring · 2 silkscreen violations on S1 · M3 holes 2.5 mm from the edge | small, all quick |

---

## 1. THE RA-02 FOOTPRINT WILL NOT TAKE THE MODULE — BLOCKER

Measured from `U3`'s pads:

| | Footprint on your board | Ai-Thinker Ra-02 |
|---|---|---|
| Pad rows | **25.40 mm apart** (1.000 inch) | module is **16 mm** wide |
| Pitch along the row | **2.54 mm** (0.100 inch) | **2.0 mm** |
| Row length, 8 pads | **17.78 mm** | module is **17 mm** long |

The module is **17 × 16 × 3.2 mm**. Its two rows of pads can be at most 16 mm apart because that is how wide the whole module is. **They cannot span 25.4 mm.** And the pitch is 2.0 mm, so even the row itself does not line up — by the eighth pad the error has accumulated to 3.8 mm.

This is HW-060, and it is now confirmed rather than suspected.

**Before anything else, answer this:** are you soldering the **bare Ra-02**, or an **Ra-02 breakout adapter board** with 2.54 mm headers? The footprint is named `RA-02_BREAKOUT_THT_2X8`, so a breakout may well be the intention.

- **Bare module** → the footprint is wrong and has to be rebuilt at 2.0 mm pitch. Measure your module with callipers first.
- **Breakout board** → measure the breakout. 25.4 mm between its header rows is possible but unusual; most are narrower.

Nothing else on this page matters until this is settled, because fixing it moves `U3` and everything routed to it.

---

## 2. DECOUPLING IS TOO FAR FROM THE PARTS IT SERVES — MAJOR

This is the condition attached to HW-013 when it was closed at the schematic stage: a capacitor only works where it is placed. Measured nearest pad to nearest pad:

| Cap | Serves | Distance | |
|---|---|---|---|
| C1 | J1 flow | 3.6 mm | ✅ |
| C5 | U1 Pro Mini | 4.0 mm | ✅ |
| C4 | BATT | 4.9 mm | ✅ |
| C12 | U2 pin 3 | 5.1 mm | ✅ |
| C10, C11 | U2 | 5.3 mm | ✅ |
| C2 | J3 ultrasonic | 5.7 mm | ✅ acceptable |
| **C8** | **U3 radio, 10 µF bulk** | **8.3 mm** | ❌ |
| **C7** | **U3 radio, 100 µF bulk** | **12.1 mm** | ❌ |
| **C6** | **U3 radio, 100 nF** | **12.2 mm** | ❌ |
| **C9** | **U2 latch hold-up, 10 µF** | **15.0 mm** | ❌ |

The four that fail are the four that matter most.

**C6, C7, C8** exist for one reason: the SX1278 pulls up to 120 mA for a few milliseconds and the cells can only supply about 50 mA. The capacitors have to hand that current over *instantly*. A 12 mm trace is roughly 12 nH of inductance in the way, and inductance is exactly what stops current changing quickly. At that distance the capacitor is supplying the burst through the very thing it was fitted to bypass.

**C9** is HW-042's fix — it holds the 74HC74's supply up through the transmit burst so the latch does not forget "on" and shut the device down on a roof. At 15 mm it is holding up the wrong end of a wire.

**Fix:** move C6 hard against U3's 3.3 V and GND pads — target under 3 mm, with C8 and C7 just outside it in that order. Move C9 against U2 pins 14 and 7. This is a placement change, not a schematic change.

---

## 3. ONE VIA ON THE ENTIRE BOARD — MAJOR

| | |
|---|---|
| Vias | **1** |
| Its net | **BATT+** |
| Ground stitching vias | **0** |

The ground pour is on the **Top** layer. Almost all the routing — 1305 mm of it across 305 segments — is on the **Bottom** layer. The pour reaches the bottom-layer ground only through the 20 through-hole ground pads that happen to exist.

That is not nothing, but it is not stitching. Return current from a bottom-layer signal has to travel sideways to the nearest ground pad before it can get up into the pour, which is the loop area HW-004 was written about. **HW-007** asked specifically for stitching around the Ra-02.

**Fix:** stitch vias into the pour — a loose grid across the board, and a tight ring around the radio, the Pro Mini and both grounds' meeting point. A dozen well-placed vias cost nothing and are the whole reason for having a pour.

---

## 4. TOP-LAYER ROUTING CUTS THE POUR INTO PIECES — MAJOR

68 tracks on the Top layer, **295 mm total**, every one of them a slot through the ground pour:

| Net | Segments | Length on Top |
|---|---|---|
| BATT+ | 27 | **75 mm** |
| NetC9_1 (latch rail) | 5 | 64 mm |
| NetC12_1 (74HC74 pin 3) | 5 | 62 mm |
| GND | 16 | 50 mm |
| BATT- | 8 | 31 mm |
| NetU2_2 | 7 | 13 mm |

A pour is only a ground plane where it is continuous. Where a trace crosses it, the return current underneath has to go around — and around a 75 mm trace is a long way. This is the slotted-plane trap noted in `HYDRO-NODE-REFERENCE.md` §11.

**Fix:** the Bottom layer already carries 1305 mm of routing and clearly has room. Move as much of these 295 mm to Bottom as will go, especially the BATT+ run. Whatever has to stay on Top should be short and nowhere near U3.

The 50 mm of *GND* on Top is a different case — that is pour-net copper and is harmless, though it is also unnecessary once the pour is stitched properly.

---

## 5. THE SMALL ONES

**Remove Dead Copper is off.** The polygon has `REMOVEDEAD=FALSE`, so isolated islands of copper that reach nothing are kept. On a 433 MHz board, unconnected metal is metal that can resonate. Turn it on and repour.

**J1's boss pad has zero annular ring.** Pad 43.3 mil, hole 43.3 mil — the drill goes straight through the whole pad, leaving no copper. It is the mounting boss of the `B2B-XH-AM` connector, so it carries no signal. Change it to a **non-plated hole**; some fabs will query it as drawn.

**Two silkscreen violations, already flagged by your own DRC.** Both stored in the file:

```
[Top Overlay] to [Top Solder] clearance [0.25mm]  at (2127.95, 1151.57) mil  Track vs Pad
[Top Overlay] to [Top Solder] clearance [0.25mm]  at (3423.23, 1151.57) mil  Track vs Pad
```

That y-coordinate is exactly **S1's pad row** — silkscreen ink printed over the reed switch's exposed pads. Ink on a pad means a bad joint. Trim the outline.

**M3 mounting holes sit 2.5 mm from the board edge.** Board is 90 × 70 mm (x 25.5–115.5, y 25.5–95.5); the holes are at 28 and 113, 28 and 93.25. That leaves **1.0 mm of board between the hole and the edge**. An M3 washer is about 7 mm across and will overhang on two sides. Move them in to about 3.5 mm from the edge if the enclosure allows.

The top pair is at y = 93.25 while the bottom pair is at y = 28.00 — 2.25 mm from one edge, 2.5 mm from the other. Probably not deliberate.

---

## WHAT IS RIGHT

| Check | Result |
|---|---|
| **Every net fully connected** | ✅ no unrouted connections, no split nets |
| Minimum different-net clearance | **0.351 mm** — comfortable, most fabs want 0.15 mm |
| Track widths | 0.3 mm (38 segments) and 0.5 mm (335) — easily manufacturable |
| Annular ring | ≥ 0.25 mm everywhere except the J1 boss |
| **Ground pour** | solid, on Top, covering the whole 90 × 70 mm board — **HW-004 closed** |
| GND pads inside the pour | 20 of 20 |
| Component collisions | none — nothing sits inside the U1 or U3 pad envelope |
| Duplicate designators | none |
| S1 stray symbol pins | did **not** create phantom pads — S1 has 2 pads, correctly |

### Footprint pitches, measured

| Ref | Measured | Should be | |
|---|---|---|---|
| U2 74HC74 | 2.54 mm pitch, **7.62 mm** rows | DIP-14, 0.3 inch | ✅ |
| U1 Pro Mini | 2.54 mm pitch, **15.24 mm** rows, 30 pads | 0.6 inch rows | ✅ |
| Q1 IRLZ44N | **2.54 mm** | TO-220 | ✅ |
| J2, J3 | **2.50 mm** | JST XH is metric 2.5 mm | ✅ |
| S1 reed | **35.00 mm** between pads, 2 pads | formed axial leads | ✅ measure your reed |
| **U3 Ra-02** | **2.54 mm pitch, 25.40 mm rows** | module is 17 × 16 mm | ❌ |

### Placement decisions that came out well

- **Sensor connectors J1, J2, J3 all on the left edge** (x ≈ 30 mm); **BATT on the right** (x ≈ 108 mm). That is 78 mm of separation — HW-011's reasoning holds on the board, not just in the enclosure.
- **S1 has moved from the middle of the board to the bottom edge** (y ≈ 29 mm, board edge at 25.5). **HW-015 closed** — the magnet spot is now reachable from outside without the reed being buried in the middle of the layout.
- **D3 → J2.3 is on the board.** `NetJ2_3` contains U1, R7 and J2 — the change from the last review is in.

---

## THE ORDER TO FIX THINGS

1. **Settle the Ra-02 footprint.** Bare module or breakout? Measure it. Everything else moves when this does.
2. **Move C6, C7, C8 to U3 and C9 to U2.** Placement only.
3. **Stitch the pour with vias** — a grid, plus a ring around U3.
4. **Push the top-layer routing down to Bottom**, BATT+ first.
5. Remove Dead Copper on · J1 boss to a non-plated hole · trim the silkscreen off S1's pads · move the M3 holes in.
6. Re-run DRC. It should come back empty.

---

## HOW THIS WAS READ

`.PcbDoc` is an OLE compound file. The object streams use two framings — `[u32 len][text]` for board, nets, polygons and components, and `[u8 type][u32 len][payload]` for tracks, arcs and vias. Pads are different again: a Pascal-string name, then a fixed 194-byte body found by scanning for its length marker.

Every geometric record shares a 13-byte header — layer, net index, component index — with coordinates as 32-bit integers at **10,000 units per mil**.

Connectivity was rebuilt by copper overlap: two tracks connect when the distance between their centrelines is under the sum of their half-widths; a pad connects to a track when the distance from the pad centre is under the pad radius plus the track half-width; through-hole pads bridge both layers; the polygon joins every same-net pad inside its outline. Testing endpoints alone reports false breaks — it wrongly split NetJ2_1 and NetC11_1 on the first pass, and both are fine.
