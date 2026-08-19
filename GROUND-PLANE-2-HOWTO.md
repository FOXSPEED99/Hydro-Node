# GROUND PLANE — HOW TO DO IT, STEP BY STEP
Hydro Node, issue **HW-004** + **HW-007**. Altium Designer.

Written assuming you have **not** done PCB layout before. Read `GROUND-PLANE-1-WHAT.md` first for why.
Scope: the ground plane only. No new components.

**Time:** about 2–3 hours the first time.
**Work in phases.** Each phase ends with a check. Do not move on until it passes.

---

## THE THREE NET NAMES YOU NEED

I read these out of your PCB file, so they are exact. Altium auto-named them from the components they touch:

| What it is | **Type this in Altium** | Where it goes |
|---|---|---|
| Switched ground | **`NetC3_1`** | The big pour |
| Raw battery ground | **`Net1_1`** | The small pour |
| Battery positive | `Net1_2` | (reference only) |

Do not guess these. Copy them exactly, including capitals.

---

## PHASE 0 — BACK UP (2 minutes, do not skip)

1. Close Altium.
2. In Windows Explorer, go to `Hydro Node Device\Hydro Node Parts & Schematic\`
3. Right-click the **`Schematic`** folder → **Copy**, then **Paste** in the same place.
4. Rename the copy to **`Schematic - BACKUP before ground plane`**.

If anything goes wrong later, delete the broken folder and rename the backup back.

✅ **Check:** two folders exist.

---

## PHASE 1 — OPEN AND ORIENT (10 minutes)

1. Open Altium. **File → Open** → `Hydro-Node.PcbDoc`.
2. Look at the **bottom of the screen** — a row of coloured layer tabs: `Top Layer`, `Bottom Layer`, `Mechanical 1`, and so on.
3. Press **`L`** to open the **View Configuration** panel. This is where you show and hide layers.
4. Click the **`Top Layer`** tab at the bottom. It becomes the *active* layer — anything you place now goes on it.

**Learn these three now — you will use them constantly:**

| Key | Does |
|---|---|
| **`Ctrl` + mouse wheel** | Zoom |
| **Right-click + drag** | Pan |
| **`Ctrl+D`** | View Options — turn on **Draft** mode to see copper as outlines instead of solid fill |

✅ **Check:** you can zoom, pan, and switch between the Top and Bottom layer tabs.

---

## PHASE 2 — MOVE THE 6 TOP-LAYER TRACKS DOWN (15 minutes)

The pour goes on the Top layer, so the Top layer has to be clear of signal traces.

**There are only 6 of them.** Find them:

1. **Edit → Find Similar Objects**, or press **`Shift+F`** and click on any track.
2. In the dialog, set **Object Kind** = `Track` and **Layer** = `Top Layer`, both to **Same**.
3. Set **Object Kind** = `Track` and everything else to **Any**.
4. Tick **Select Matching** at the bottom → **Apply** → **OK**.

Altium now has those 6 tracks selected. Then:

5. **Edit → Move → Move Selection to Layer...**
6. Choose **`Bottom Layer`** → **OK**.

If two traces now overlap on the bottom layer, Altium will flag it in the DRC later — deal with it in Phase 7.

✅ **Check:** click the `Top Layer` tab, press **`Ctrl+D`** and hide the Bottom layer. The board should show **pads only, no traces**.

---

## PHASE 3 — CREATE THE MAIN GROUND POUR (20 minutes)

This is the important one.

1. Make sure **`Top Layer`** is the active tab at the bottom.
2. **Place → Polygon Pour**, or press **`P`** then **`G`**.
3. The **Polygon Pour** dialog opens. Set:

| Field | Set to | Why |
|---|---|---|
| **Fill Mode** | **Solid (Copper Regions)** | Solid copper. Not hatched. |
| **Layer** | **Top Layer** | |
| **Connect to Net** | **`NetC3_1`** | The switched ground |
| **Pour Over** | **Pour Over All Same Net Objects** | |
| **Remove Dead Copper** | **ticked** | Deletes isolated scraps that connect to nothing |

4. Click **OK**. The cursor becomes a crosshair.
5. **Draw the outline.** Click each corner of the board, just **inside** the board edge — leave about **0.5 mm** of gap so copper does not run off the edge. Click the four corners, then **right-click** to finish.
6. Altium fills the area with copper. Give it a moment.

✅ **Check:** the top of the board is now mostly copper. Zoom in on a pad that is *not* ground — it should have a clean ring of empty space around it. Zoom in on a ground pad — it should be joined to the copper.

**If nothing filled:** you almost certainly typed the net name wrong. Double-click the pour → check **Connect to Net** says exactly `NetC3_1`.

---

## PHASE 4 — CREATE THE SECOND, SMALLER POUR (10 minutes)

The latch circuit sits on the *other* ground and needs its own island.

1. Still on **`Top Layer`**.
2. **`P`**, **`G`** again.
3. Same settings, except **Connect to Net** = **`Net1_1`**.
4. Draw a **small** outline around **U1, S1 and Q1** only — the flip-flop, the reed switch and the MOSFET, in the right-hand area of the board.
5. Right-click to finish.

> ⚠️ **The two pours must not touch.** They are separated by the MOSFET on purpose. If they join, you short across the master power switch and the device can never turn off. Leave a visible gap — Altium's clearance rule will normally hold them apart, but check by eye.

✅ **Check:** two distinct copper areas on the Top layer, with a clear gap between them.

---

## PHASE 5 — CONNECT THE THREE MISSING RADIO GROUND PINS (30 minutes)

This is **HW-007**. Three of the Ra-02's four ground pins are connected to nothing: **J1 pin 1, J1 pin 2, J2 pin 1**.

They cannot simply be assigned in the PCB — Altium is schematic-driven, so the schematic has to say it first or the next sync will undo your work.

**In the schematic:**

1. Open **`Hydro-Node.SchDoc`**.
2. Find **J1** (the 8-pin header). Pins 1 and 2 are both labelled GND and have short stubs going nowhere.
3. Press **`P`** then **`W`** (Place Wire). Draw a wire from J1 pin 1 to the same net that J2 pin 8 is on — the existing switched-ground net.
4. Do the same for **J1 pin 2** and **J2 pin 1**.
5. **Save** the schematic (`Ctrl+S`).

**Push it into the PCB:**

6. With the schematic open: **Design → Update PCB Document Hydro-Node.PcbDoc**
7. The **Engineering Change Order** window opens, listing what will change.
8. Click **Validate Changes** — every row should get a green tick.
9. Click **Execute Changes** → **Close**.

✅ **Check:** back in the PCB, those three pads now belong to `NetC3_1` and join the pour when it repours.

**If Validate shows red X marks:** stop and read what it says. Usually it means a component or footprint does not match. Do not force it.

---

## PHASE 6 — SET HOW PADS CONNECT TO THE POUR (10 minutes)

By default Altium connects pads with **thermal relief** — four thin spokes instead of solid copper. That is good for hand soldering but bad for a radio ground.

1. **Design → Rules**
2. In the tree on the left: **Plane → Polygon Connect Style**
3. The default rule appears on the right. Leave it as **Relief Connect** — that keeps the through-hole parts hand-solderable.
4. Now add a second rule just for the radio. Right-click **Polygon Connect Style** → **New Rule**.
5. Name it `RF_Direct`.
6. Set its scope to the radio's ground pads. In the **Where The Object Matches** box choose **Custom Query** and enter:

   ```
   InComponent('J1') Or InComponent('J2')
   ```

7. Set **Connect Style** = **Direct Connect**.
8. Drag this new rule **above** the default one in the priority list (there is a **Priorities...** button).
9. **OK**.

✅ **Check:** after the next repour, the Ra-02 ground pads sit in solid copper; other ground pads have little four-spoke wheels.

---

## PHASE 7 — REPOUR AND CHECK (20 minutes)

1. **Tools → Polygon Pours → Repour All**. Confirm.
2. **Tools → Design Rule Check** → **Run Design Rule Check**.
3. A report opens. Work through anything listed.

**Errors you should expect and how to read them:**

| Message | Means | Fix |
|---|---|---|
| *Clearance Constraint* | Two things are too close | Usually a track you moved in Phase 2. Nudge it. |
| *Un-Routed Net* | A connection is missing | Check it is not one of your Phase 5 wires |
| *Short-Circuit Constraint* | Two nets are touching | **Check the two pours are not joined** |

4. Now look for **isolated islands** — copper patches surrounded by clearance that connect to nothing. **Remove Dead Copper** should have handled these, but scan the board by eye. Any leftover island is a small floating antenna.

5. **Look for slots.** Zoom out and look at the main pour as a whole. If a line of pads or a track cuts a long channel through it, the return current has to detour around that channel — which recreates the very loop the plane exists to remove. Where you see one, either move the offending track or bridge the pour across the gap.

6. **Check under the radio.** Zoom in on the Ra-02 footprint. The copper beneath it should be **completely solid**, no channels, no gaps.

✅ **Check:** DRC clean, no islands, no slots, solid under the radio.

---

## PHASE 8 — SAVE AND OUTPUT

1. **`Ctrl+S`** to save the PCB.
2. For the fab: **File → Fabrication Outputs → Gerber Files**, and **NC Drill Files**.
3. Before sending anything, open the Gerbers in a free viewer and confirm the top copper layer really shows the pour.

---

## PHASE 9 — PROVE IT WORKED

Process checks tell you the file is right. Only one thing tells you the *board* is better:

**Have the Hub log RSSI from the same Node, at the same physical location, on the old board and on the new one.** The improvement in the radio's ground reference should show up directly in that number.

Record the result against **HW-004** in the issue tracker.

---

## IF YOU GET STUCK

Tell me which phase, what you clicked, and what the screen says. The two places people most often trip:

- **Phase 3, nothing fills** → the net name is wrong. It is `NetC3_1`, exactly.
- **Phase 5, red X on Validate** → do not force the ECO. Send me the message text.
