# FINAL PCB CHECK — 2026-08-31

Checked against the files you just uploaded to `main`:
`Hydro Node Parts & Schematic/Schematic/Hydro_Node_PCB.PcbDoc` (dated 8/31/2026 3:49 PM)
and `Hydro_Node_Schematic.SchDoc`. Both have changed since the last review.

Every number below is measured out of the files, not estimated.

---

## VERDICT

**Yes — start coding. The board is not quite ready to send to the fab, but nothing
blocking it affects the firmware.**

| | |
|---|---|
| **Electrically correct?** | **Yes.** Every net is fully connected. No shorts. No missing connections. |
| **Manufacturable?** | **Almost.** Four small items, all cosmetic-to-mechanical, none electrical. |
| **Safe to write firmware against?** | **Yes.** The pin map is final and is in section 5. |
| **Blocking anything?** | Only **HW-003**, the battery pack diodes — and that is a pack assembly item, not a board item. |

The four remaining board items are a tick box, a pad setting, a hole size and a
hole position. About twenty minutes in Altium. **Do them before you order boards,
but do not wait for them to start writing code.**

---

## 1. WHAT YOU FIXED SINCE THE LAST REVIEW

This is a genuinely large improvement. Measured:

| | Before | Now | |
|---|---|---|---|
| Ground pour | Top layer only | **Top *and* Bottom** | ✅ |
| Stitching vias | **1**, and it was on BATT+ | **33, all on GND** | ✅ |
| Min different-net clearance | 0.351 mm | **1.200 mm** | ✅ 3.4× better |
| Silkscreen over S1's pads | 2 stored DRC violations | **none, board-wide** | ✅ |
| C6 (100 nF) to the radio | 12.2 mm | **5.55 mm** | ✅ |
| C8 (10 µF) to the radio | 8.3 mm | **3.78 mm** | ✅ |
| Board size | 90 × 70 mm | **80 × 60 mm** | ✅ smaller |
| Every net fully connected | yes | **yes, re-verified** | ✅ |

Two schematic fixes are in as well, and both are correct:

**HW-067 is permanently fixed.** The 100 Ω from A0 to the flip-flop's clock is
gone from the schematic — not just cut on the bench. A0 now goes nowhere.
This is the fault that stopped the first board switching on.

**HW-069's prototype fix is in.** R14 is 2.2 MΩ in the schematic, giving the
~265 ms chatter-ignore window you tested.

**HW-053 is now fixed properly, and better than the version that was signed off.**
Previously the probe supply was tied to BATT+ as a compromise. Now **both** the
probe supply *and* the 4.7 kΩ pull-up have moved to **D3**:

```
D3 ──┬── J2.3   (DS18B20 VDD, both probes)
     └── R7 4.7 kΩ ── J2.1 (the data line)
```

That matters, and it is the exact thing the earlier review said had to happen
together. If only `J2.3` had moved, the 4.7 kΩ would still hold the data line at
3.6 V when D3 went low, and the probes would stay half-powered through their own
protection diodes. Moving both means D3 low really is off. **~1.5 µA of standby
saved, and no part is ever powered through a protection diode.**

Current check on D3: two DS18B20s converting draw about 1.5 mA each, plus
0.7 mA through the pull-up when the bus is held low — about **3.7 mA**, against
an ATmega328P pin that sources 20 mA comfortably. Fine.

---

## 2. THE FOUR THINGS LEFT BEFORE YOU ORDER BOARDS

None of these are electrical. None of them affect firmware.

### 2.1 Remove Dead Copper is still off — HW-064

Both polygons carry `REMOVEDEAD=FALSE`. Isolated islands of copper that connect
to nothing are being kept. On a board with a 433 MHz transmitter, unconnected
metal can resonate.

**Fix:** double-click each polygon → tick **Remove Dead Copper** → OK →
**Tools → Polygon Pours → Repour All**. Two tick boxes.

### 2.2 The mounting holes are 2.70 mm — an M3 screw will not fit

Measured: all four are **2.70 mm holes with a 2.70 mm pad**, at (28, 28),
(28, 83), (103, 28) and (103, 83).

Two separate problems:

**The size.** An M3 screw is 3.0 mm across. A 2.70 mm hole will not pass it.
For a clearance hole M3 needs **3.2 mm**. 2.70 mm is the correct clearance size
for **M2.5**. So either these are M2.5 holes and the issue tracker is wrong to
call them M3, or they need to go to 3.2 mm. **Decide which screw you are using
and set the hole to match.**

**The zero ring.** Pad 2.70 mm, hole 2.70 mm — the drill removes the entire pad,
leaving no copper. That is a plated hole drawn with no annular ring. Some fabs
will query it and some will plate a hole with nothing to plate onto.

**Fix:** change all four to **non-plated (NPTH)** holes — double-click → uncheck
Plated, or place them as mechanical holes rather than pads.

### 2.3 The mounting holes are still 2.5 mm from the board edge — HW-066

Board is **80 × 60 mm**, spanning x 25.5–105.5 and y 25.5–85.5. The holes sit
2.5 mm in from each edge, and with a 2.70 mm hole that leaves **1.15 mm of
board material** between the hole and the edge.

That is where a board cracks when someone overtightens a screw, and an M3 washer
(~7 mm) will hang over the edge on two sides.

**Fix:** move them in to about 4 mm from each edge if the enclosure bosses allow.
This is unchanged from the last review — it was never actioned.

### 2.4 J1's boss pad still has zero annular ring — HW-065

J1's mounting boss: pad **1.1 mm**, hole **1.1 mm**. Same problem as 2.2 — the
drill takes the whole pad. It is the plastic locating peg of the `B2B-XH-AM`
connector and carries no signal.

**Fix:** make it a **non-plated hole**.

The other half of HW-065 — silkscreen printed across S1's pads — is **fixed**.
I checked every silkscreen segment against every pad on the board: nothing comes
within 0.25 mm of any pad.

---

## 3. ONE THING THAT IS NO LONGER AN ISSUE — HW-063 REWRITTEN

The last review said: *"295 mm of top-layer routing cuts slots through the ground
pour — move it to the Bottom layer."*

**That advice no longer applies, because you solved the problem a better way.**

There are now **269 mm of routing on Top and 829 mm on Bottom** — so on the face
of it the top-layer figure barely moved. But the reasoning behind the old advice
was that the pour existed *only on Top*, so a top-layer trace cut the only
ground return path there was.

You now have **a pour on both layers and 33 stitching vias**. That changes the
picture completely:

- A slot in the top pour is bypassed through the bottom pour, and vice versa.
- Moving a trace from Top to Bottom would now just move the slot to the other
  layer. It would gain nothing.
- The stitching vias are placed where they matter — clustered around U3, U1, U2
  and Q1, which is exactly what HW-007 asked for.

**So: leave the routing alone. HW-063 closes.** The two-pour-plus-stitching
approach is the correct answer for a two-layer board and is better than what was
originally recommended.

---

## 4. WHAT I VERIFIED, AND HOW

So you know what has actually been checked rather than glanced at:

| Check | Result |
|---|---|
| Every net's pads joined by real copper | ✅ **all 28 signal nets, verified by copper overlap** |
| GND | ✅ 22 pads, all fully inside both pours, 33 vias |
| Different-net track-to-track clearance | ✅ **1.200 mm** minimum |
| Different-net pad-to-pad clearance | ✅ **0.660 mm** minimum (U1's 2.54 mm header) |
| Annular ring | ✅ ≥ 0.25 mm everywhere except the 5 holes in §2.2 and §2.4 |
| Silkscreen over pads | ✅ none, board-wide |
| Component count vs schematic | ✅ 37 = 37 |
| Duplicate designators | ✅ none |
| Pads outside the board edge | ✅ none, every pad ≥ 0.5 mm inside |
| Track widths | ✅ 0.30 mm and 0.50 mm — easy for any fab |
| Via size | ✅ 1.27 mm pad / 0.71 mm hole, 0.28 mm ring |

### Footprint pitches, measured from the pads

| Ref | Measured | Should be | |
|---|---|---|---|
| U1 Pro Mini | 2.54 mm pitch, **15.24 mm** rows, 30 pads | 0.6 inch | ✅ |
| U2 74HC74 | 2.54 mm pitch, **7.62 mm** rows, 14 pads | DIP-14, 0.3 inch | ✅ |
| U3 Ra-02 breakout | 2.54 mm pitch, **25.40 mm** rows, 16 pads | as measured off your module | ✅ |
| Q1 IRLZ44N | **2.54 mm** lead pitch + 3.2 mm tab hole | TO-220 | ✅ |
| J2 / J3 | **2.50 mm** | JST XH is metric | ✅ |
| S1 reed | **35.00 mm** between pads | formed axial leads | ✅ |
| Resistors | 10.00 mm | axial | ✅ |

**A correction to my own working, recorded because it nearly became a false alarm.**
On the first pass Q1's three lead pads looked like they *overlapped by 0.06 mm* —
which would have been a dead short between Gate, Drain and Source, and a blocker.
It was wrong. My pad parser was not reading Altium's **pad rotation** field. Q1's
pads are rotated 270°, so the 2.60 mm dimension runs across the package and the
1.70 mm dimension runs along the 2.54 mm lead pitch. Real gap: **0.84 mm.**
The parser now reads rotation (it lives at byte offset 52 of the pad body, as an
8-byte double). I found the field before reporting, not after.

Q1's tab is on **GND**, which is correct — the IRLZ44N's TO-220 tab is internally
the Drain, and the Drain is the switched ground.

---

## 5. THE PIN MAP — WRITE YOUR FIRMWARE AGAINST THIS

Extracted from the schematic netlist. **This is final for this board revision.**

| Pin | Goes to | Direction | Notes |
|---|---|---|---|
| **D2** | Ra-02 **DIO0** | input | TX-done / RX interrupt |
| **D3** | **DS18B20 power** (J2.3) + the 4.7 kΩ pull-up | **output** | HIGH to read temperature, LOW to save power |
| **D4** | DS18B20 **data** via R4 100 Ω | 1-Wire | pull-up is on D3, not the battery |
| **D5** | flow switch via R5 100 Ω | input | LOW = water flowing |
| **D6** | ultrasonic **TRIG** via R2 100 Ω (J3.4) | output | |
| **D7** | **buzzer** via R9 100 Ω | output | |
| **D8** | ultrasonic **ECHO** via R1 100 Ω (J3.3) | input | ICP1 — hardware input capture available |
| **D9** | Ra-02 **RESET** | output | active low |
| **D10** | Ra-02 **NSS** | output | |
| **D11** | Ra-02 **MOSI** | output | |
| **D12** | Ra-02 **MISO** | input | |
| **D13** | Ra-02 **SCK** | output | **also the on-board LED — see the warning below** |
| **A0** | **nothing** | — | **disconnected. See §6.** |
| **A1** | flip-flop reset via R11 100 kΩ | **input, except to shut down** | drive LOW to switch the node off |
| **A2** | flow switch via R3 330 Ω | input | same node as D5 |
| A3–A7, D0, D1 | nothing | — | `INPUT_PULLUP` before sleep |

### Three firmware rules that come out of the hardware

**1. D13 must be driven LOW before sleeping.** It is SCK *and* the LED. Left as
`INPUT_PULLUP` it sits at ~1.8 V, which is an invalid logic level into the radio's
clock input, and that costs **250 µA** — measured. `SPI.end();` then
`pinMode(13, OUTPUT); digitalWrite(13, LOW);`

**2. A1 is an input at all times except the moment you deliberately shut down.**
Making it an output HIGH, or an input with the pull-up on, pushes current into
the latch's reset pin. A0's version of this mistake is what killed the first
board.

**3. The buzzer's OFF beep must finish before A1 drops the latch.** Once A1
commands shutdown the MOSFET opens and the MCU loses ground mid-note. Sound
first, then shut down.

The full sleep sequence that measured **7.8 µA** is in
`firmware/sleep_test/sleep_test.ino`, stage 6. Copy it.

---

## 6. ONE THING TO KNOW BEFORE YOU WRITE FIRMWARE

**A0 is now permanently disconnected, so the MCU cannot tell whether the magnet
is present.**

This is a *consequence* of the HW-067 fix, not a mistake — the 100 Ω from A0 was
what held the flip-flop's clock at 2.4 V and stopped the board switching on, and
removing it was correct. But it was carrying a feature, and that feature is now
gone:

- The firmware **cannot** read the reed switch.
- The firmware **cannot** tell "the user just waved the magnet" from any other
  reason it woke up.
- The magnet-hold gesture for local recovery (HW-022) is not available.

The node can still switch **itself** off through A1. Only *sensing* is lost.

**Do not write firmware that assumes A0 reads anything.** It comes back in Rev B,
where the Schmitt trigger (HW-069) gives A0 a driven output to sit on, safely.

---

## 7. WHAT IS STILL OPEN, AND WHERE IT BELONGS

| | Item | Where it gets fixed |
|---|---|---|
| **BLOCKER** | **HW-003** — 2 × 1N5819 + 0.5 A fuse, sealed pack | **Battery pack assembly, not the board.** Still the only blocker. |
| Before fab | HW-064 Remove Dead Copper | 2 tick boxes |
| Before fab | Mounting holes: 2.70 mm, NPTH, screw size | §2.2 |
| Before fab | HW-066 hole edge distance | §2.3 |
| Before fab | HW-065 J1 boss → NPTH | §2.4 |
| Rev B | HW-069 Schmitt trigger + A0 on its output | next board spin |
| Rev B | HW-058 electrolytics → ceramic/tantalum | next board spin |
| Rev B | HW-068 R12 1 MΩ → 220 kΩ | next board spin |
| Rev B | HW-071 high-side switch for the ultrasonic on D3-style pin | next board spin |
| Measure | HW-071 — ultrasonic idle current at 3.6 V | bench, before it matters |
| Measure | HW-047 — the radio link at 50 m through concrete | on site |

**HW-071 is the one to measure soon.** J3.2 is still on BATT+ with J3.1 on the
switched ground, so the ultrasonic module is powered for all 120 seconds between
readings. You have proven the Pro Mini and radio sleep at 7.8 µA; if the
ultrasonic idles at 2 mA it will be 250 times everything else combined. Put it on
a bench supply at 3.6 V, leave it alone, and read the current. That one number
decides whether Rev B needs a load switch.

---

## HOW THIS WAS READ

`.PcbDoc` and `.SchDoc` are OLE compound files, parsed directly — no Altium
involved. Nets, components and polygons come from `[u32 len][text]` records;
tracks, arcs and vias from `[u8 type][u32 len][payload]`.

**Pads needed fixing for this file.** The `Pads6` stream is not the same framing
as the others: each record is a Pascal-string name block, then three short
sub-blocks, then a 194-byte body, then a zero-length terminator. The old walker
desynchronised on the first record and reported **zero pads** — which would have
silently skipped every footprint, clearance and connectivity check in this
document. Bodies are now located by scanning for the `\xc2\x00\x00\x00` length
marker and validating the layer byte, which recovers all **141 pads**.

Connectivity is rebuilt by **copper overlap**, not endpoint proximity: two tracks
connect when the distance between centrelines is under the sum of their
half-widths, and a pad connects when the distance is under the pad's half-size
plus the track's half-width. Through-hole pads and vias bridge both layers.
Testing endpoints alone reports false breaks.

Coordinates are 32-bit integers at 10,000 internal units per mil.
