# THE THREE PCB ISSUES, EXPLAINED — AND HOW TO FIX THEM

Written 2026-08-24, after the PCB check.

---

## FIRST — A CORRECTION TO WHAT I TOLD YOU

Two things in my last message were overstated. Fixing them changes what you should actually do, so read this before spending time on anything.

**1. The Ra-02 footprint is correct.** You are fitting the **breakout board**, not the bare module. Measuring your own photo of it — 8 holes per row, rows about 9.7 pitches apart — gives 2.54 mm pitch with roughly 25 mm between rows. That is exactly what the footprint has. I assumed the bare 17 × 16 mm module. **Nothing to fix. HW-060 closed.**

**2. Only one capacitor needs moving, not four.** I said C6, C7, C8 and C9 were all too far away. When I worked the numbers rather than the rule of thumb:

| Cap | Distance | Verdict |
|---|---|---|
| **C6** 100 nF at U3 | 12.2 mm | **move it** — this one really is degraded |
| C7 100 µF at U3 | 12.1 mm | **fine where it is** |
| C8 10 µF at U3 | 8.3 mm | **fine where it is** |
| C9 10 µF at U2 | 15.0 mm | **fine where it is** |

The arithmetic is below. The short version: bulk capacitors supply current over *milliseconds*, and a centimetre of trace is nothing on that timescale. The 100 nF's whole job is the *nanosecond* timescale, and there a centimetre is everything.

So the real list is shorter than I made it: **move C6, add some vias, and get the top-layer traces off the ground pour.**

---

## THE ONE IDEA BEHIND ALL THREE

Everything below follows from a single fact:

> **Current always travels in a loop. It never just goes somewhere — it has to come back.**

When the radio draws 120 mA, that current leaves the battery, goes through a trace to the radio, *and the same 120 mA comes back* through the ground to the battery. Out and back. Always a closed ring.

Two things follow, and they are the whole of this document:

**A. A long loop resists sudden changes in current.**
Think of water in a pipe. To make the water suddenly flow faster you have to get all the water in that pipe moving, and that takes time. A long thin pipe is sluggish; a short fat one responds immediately. Electricity behaves the same way, and the property is called **inductance**. A PCB trace has roughly **1 nH of inductance per millimetre**.

**B. A big loop is an antenna.**
A ring of wire carrying a changing current radiates — that is literally how an antenna works. The bigger the ring, the more it radiates, and the more it picks up from outside. On a board with a 433 MHz transmitter on it, you want every loop as small as you can make it.

Everything below is about keeping loops small.

---

## ISSUE 1 — C6 IS TOO FAR FROM THE RADIO

### What C6 is for

When the radio transmits, it needs current *right now*. Not "in a moment". The battery is 100 mm away across the board and can only supply 50 mA anyway, so the capacitor next to the chip acts as a local bucket of charge that hands current over instantly.

But "instantly" is exactly what a long loop prevents — see **A** above.

### Why 12 mm is too far

Every capacitor stops behaving like a capacitor above a certain frequency, because the inductance of the wire getting to it takes over. That crossover is called the **self-resonant frequency**. Above it, your capacitor is an inductor and does nothing useful.

C6 is 12.2 mm from U3. The loop is out and back, so about **24 mm**, which at 1 nH/mm is about **20 nH**:

| Where C6 sits | Loop inductance | 100 nF works up to |
|---|---|---|
| 12.2 mm away (now) | ~20 nH | **3.6 MHz** |
| 3 mm away | ~5 nH | **7.1 MHz** |
| ideal, pads touching | ~1.2 nH | 14.5 MHz |

At 12 mm the capacitor has lost most of its useful range. Moving it to 3 mm doubles it, for free.

### Why C7, C8 and C9 are fine — the correction

C7 and C8 are **bulk** capacitors. Their job is to supply 120 mA for the few **milliseconds** a transmission lasts. On that timescale inductance is irrelevant; what matters is plain resistance:

- 12 mm of 0.5 mm-wide 1 oz copper ≈ **12 mΩ**
- at 120 mA that is **1.4 mV**

One and a half millivolts out of 3600. It does not matter.

C9 is even clearer. It holds the 74HC74's supply up during a transmit, and the 74HC74 draws **microamps**. Over a 5 ms burst a 10 µF capacitor supplying 10 µA sags by **5 mV**. The trace to it is irrelevant at those currents.

**So: move C6. Leave C7, C8 and C9 alone.**

### How to fix it in Altium

1. Click **C6** to select it, press **M** → **Move Selection** (or just drag it).
2. Drop it so its two pads sit **right beside U3's 3.3 V pad and U3's nearest GND pad**. Target under 3 mm. Under the module's body is fine if the breakout stands on headers — check the height first.
3. The old tracks will now be rubber-banding. Select the two stale track segments and delete them.
4. Re-route: **Place → Interactive Routing** (shortcut **P, T**), one short trace from C6's plus pad to U3's 3.3 V pad. Keep it as short and as wide as it will go.
5. C6's other pad goes to **GND**. Do not run a long trace for it — drop a **via** right next to that pad and let it connect into the ground pour (see Issue 2 for how).
6. **Tools → Polygon Pours → Repour All.**

---

## ISSUE 2 — ONE VIA ON THE WHOLE BOARD

### What a via is

A via is a plated hole that carries a signal from one side of the board to the other. Your board has exactly **one**, and it is on BATT+.

### What a *stitching* via is

A stitching via does not carry a signal. It exists purely to tie the ground copper on the top to the ground copper on the bottom, in lots of places, so return current can move between layers wherever it needs to instead of detouring to find a crossing point.

### Why this is less serious than I first said

Your board is **entirely through-hole**. Every component lead goes through a plated hole — and a plated hole *is* a via. You have **20 ground pads**, spread all over the board, and every one of them already ties the top pour to the bottom-layer ground.

So the layers are connected in 20 places already. That is why this is worth doing but is not urgent.

### Why it is still worth doing

Two reasons:

1. **The slots from Issue 3.** Where a top-layer trace crosses the pour, it cuts the pour into pieces. Vias on both sides of a slot let those pieces stay properly joined through the bottom layer.
2. **The radio.** U3 has four ground pads, but they are all on the module footprint. A ring of vias around U3 gives the transmit current's return path somewhere short to go, which is what **HW-007** asked for.

### How to fix it in Altium

**The easy automatic way:**

1. **Tools → Via Stitching/Shielding → Add Stitching to Net…**
2. Net: **GND**
3. Via size: use the same as your existing via, or 0.6 mm hole / 1.0 mm pad
4. Grid: **5 mm** is a sensible spacing for this board
5. Tick *Keep the vias inside the polygon*, then OK
6. **Tools → Polygon Pours → Repour All**

**Doing the important ones by hand:**

1. Press **P, V** (Place → Via)
2. Before clicking, press **Tab** and set **Net = GND**
3. Place them:
   - **6–8 around U3**, close to the module footprint
   - **2–3 near U1**
   - **1 next to C6's ground pad** after you move it
   - **2–3 near Q1**, where the switched ground and the battery ground meet
   - a few on each side of any long top-layer trace that stays

Vias cost nothing at the fab. There is no reason to be sparing.

---

## ISSUE 3 — THE TOP-LAYER TRACES CUT THE GROUND POUR APART

**This is the one actually worth your time.**

### What is happening

Your ground pour is on the **Top** layer. Your signals are almost all on the **Bottom** layer — 1305 mm of them. That arrangement is correct and it is what you were asked for: every bottom-layer trace has continuous ground copper directly above it, so its return current flows straight back along the shortest possible path. Small loop. Good.

But you also routed **68 tracks on the Top layer, 295 mm in total**. Every one of those is a **slot cut through the pour**:

| Net | Segments | Length on Top |
|---|---|---|
| **BATT+** | 27 | **75 mm** |
| NetC9_1 (latch rail) | 5 | 64 mm |
| NetC12_1 (74HC74 pin 3) | 5 | 62 mm |
| GND | 16 | 50 mm |
| BATT- | 8 | 31 mm |
| NetU2_2 | 7 | 13 mm |

### Why a slot is bad

Picture a bottom-layer signal trace with its return current flowing in the pour directly above it. Now a top-layer trace crosses its path. The pour has to be interrupted to make room for it — there is a gap in the copper.

The return current cannot cross that gap. It has to run **along** the slot until it reaches the end, go around, and come back. A short return path has just become a long detour, and the loop that was tiny is now large. Both **A** and **B** from the top of this page get worse at once.

This is the trap that catches people who add a pour and think the job is done: the board *looks* like it has a ground plane, passes every visual check, and still behaves like a board without one. A 75 mm slot is a very long detour.

### What matters most

The **75 mm of BATT+ on Top** is the worst one, because it is long. Deal with that first.

The **50 mm of GND on Top** is fine — that is pour-net copper on the pour's own net, so it does not cut anything. Ignore it.

### How to fix it in Altium

The good news: because this board is entirely through-hole, **moving a trace from Top to Bottom needs no extra vias**, as long as both ends land on component pads. The pads already go through the board.

**To move one track:**

1. Click the track segment to select it.
2. In the **Properties** panel, change **Layer** from *Top Layer* to *Bottom Layer*.
3. If the whole run is several segments, select them all first — click one, then **Edit → Select → Connected Copper** (shortcut **Ctrl+H**) picks up the whole connected run.

**Or re-route it, which usually gives a tidier result:**

1. Select the run and delete it.
2. Press **P, T** for interactive routing.
3. Before you start, press **Ctrl+Shift+scroll wheel** or the **`*`** key to make the Bottom layer active.
4. Route it on Bottom.

**Do it in this order:**

1. **BATT+** — the 75 mm one. Biggest win.
2. **BATT-** — 31 mm.
3. **NetC9_1** and **NetC12_1** — 64 and 62 mm, but they are low-speed latch nets so they matter less than BATT+.
4. Leave **GND**. It is pour copper.
5. Anything that genuinely will not fit on Bottom: keep it short and keep it **away from U3**.

Then **Tools → Polygon Pours → Repour All** and look at the result. The pour should be one large continuous area with only small interruptions, not a set of islands joined by thin necks.

---

## THE SMALL ONES

**Remove Dead Copper.** Double-click the polygon → tick **Remove Dead Copper** → OK → repour. This deletes isolated islands of copper that connect to nothing. On a 433 MHz board, floating metal can resonate. One tick box.

**J1's boss pad has no copper ring.** Its pad is 43.3 mil across and its hole is also 43.3 mil, so the drill removes the entire pad. It is the plastic mounting peg of the connector and carries no signal — change it to a **non-plated hole** (double-click the pad → set **Plated** off, or set it to a hole with no pad). Some fabs will query it as drawn.

**Two silkscreen violations on S1's pads.** Your own DRC already found these and they are saved in the file:

```
[Top Overlay] to [Top Solder] clearance [0.25mm]  at (2127.95, 1151.57) mil
[Top Overlay] to [Top Solder] clearance [0.25mm]  at (3423.23, 1151.57) mil
```

That is the reed switch's pad row — silkscreen ink printed across bare pads. Ink on a pad gives a bad solder joint, and on a hand-built board that reads exactly like a cold joint and costs an hour to find. Move or trim the S1 outline on the Top Overlay layer.

**M3 mounting holes are 2.5 mm from the board edge.** That leaves 1.0 mm of board between the hole and the edge. An M3 washer is about 7 mm across and will hang over the edge on two sides, and 1 mm of material next to a screw hole is where a board cracks when someone over-tightens it. Move them in to about 3.5 mm if the enclosure bosses allow. While you are there: the top pair is at y = 93.25 mm and the bottom pair at y = 28.00 mm — square that up.

---

## THE ORDER TO DO IT IN

1. **Move the top-layer traces to Bottom**, BATT+ first. *(biggest win)*
2. **Move C6** hard against U3's 3.3 V and GND pads.
3. **Add stitching vias** — automatic grid, plus a ring around U3 by hand.
4. Tick **Remove Dead Copper**.
5. J1 boss → non-plated hole. Trim the silkscreen off S1's pads. Move the M3 holes in.
6. **Tools → Polygon Pours → Repour All**, then run DRC. It should come back empty.

Leave C7, C8 and C9 where they are.
