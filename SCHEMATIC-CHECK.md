# SCHEMATIC CHECK — Hydro Node Schematic.SchDoc

Checked 2026-08-22 against `BUILD-SHEET.md`.
Netlist rebuilt from the file with `tools/extract_netlist.py`.

---

## VERDICT

**Do not start the PCB yet. 5 faults.** Two of them stop the device working.

Everything else checks out. All 38 components are placed, all values are right, and 29 of the 34 connections match the build sheet exactly.

| | Fault | Effect |
|---|---|---|
| 1 | **D3 → temperature connector VCC is missing** | Both DS18B20 probes have no power. No temperature reading, and no correction on the ultrasonic distance. |
| 2 | **Ultrasonic connector pin order is wrong** | With a straight cable, 3.6 V goes into the module's RX pin and the module gets no power. |
| 3 | **Buzzer has no series resistor** | Peak current at each edge is above the D7 pin's absolute maximum. |
| 4 | **3 of the Ra-02's 4 GND pins are unconnected** | RF return path. Already logged as HW-007. |
| 5 | **4 Pro Mini power/ground pins unconnected + a stray reed symbol off-sheet** | DRC noise; loses ground stitching points on the PCB. |

---

## THE 5 FAULTS

### 1. The temperature probes have no supply — BLOCKER

`U1.JP7_7 (D3)` has no wire on it. `J2.3` connects to one thing only: `R7.1`.

```
now      J2.3 ──── R7 (4.7 kΩ) ──── J2.1 ──── R4 (100 Ω) ──── D4
                                      │
                                    (probe DATA)

needed   J2.3 ──── D3                       ← the missing wire
             └──── R7 (4.7 kΩ) ──── J2.1
```

`J2.3` is the DS18B20 VCC pin. Right now it floats. The only thing holding it anywhere is the 4.7 kΩ back to the data line, so the probes sit with their VDD pin undefined — which the DS18B20 datasheet does not allow. VDD must be a real supply, or tied to GND for parasite power. Floating is neither.

D3 was the pin that switches probe power on and off, so the probes only draw current during a reading.

**Fix:** wire `U1.JP7_7 (D3)` to `J2.3`.

---

### 2. Ultrasonic connector pin order does not match the module — MAJOR

The RCWL-1670 board's four pads, left to right, are printed **`GND · RX · TX · +5V`**. J3 is wired:

| J3 pin | Schematic | Module pad at that position | |
|---|---|---|---|
| 1 | GND | GND | ✅ |
| 2 | **BATT+** | **RX** | ❌ |
| 3 | TX (→ R1 → D8) | TX | ✅ |
| 4 | **RX (→ R2 → D6)** | **+5V** | ❌ |

Pins 2 and 4 are swapped. With a straight-through 1:1 cable:
- 3.6 V lands on the module's RX input
- the module's supply pin gets driven by an MCU output through 100 Ω
- the module never powers up

The cable is hand-made, so a crossed cable would work — but a crossed cable is what HW-001 already flags as a fault waiting to happen. Fix it in the schematic so a straight cable is correct.

**Fix:** swap the nets on `J3.2` and `J3.4`. J3 then reads `GND · RX · TX · +5V`, matching the module and matching the build sheet's tape label.

---

### 3. Buzzer LS1 is driven straight off D7 with no series resistor — MAJOR

`N23: LS1.P(P), U1.JP7_3(D7)` — a direct wire.

The CPT-1255C-090 is an externally driven piezo with an electrostatic capacity of **8,400–15,600 pF**. A capacitor connected straight to a logic pin draws its current at the switching edge, limited only by the pin's own output impedance. That peak is well above the ATmega328P's **40 mA absolute maximum per pin**.

**Fix:** put a **100 Ω** resistor between D7 and `LS1.P`. At the 4 kHz rated frequency the buzzer's own impedance is around 3 kΩ, so 100 Ω costs no measurable loudness.

Three further points on the buzzer, which replaced the LED and 1 kΩ from the build sheet:

- Rated output is **70 dB at 10 cm at 3 Vp-p**. Driven single-ended from 3.3 V you get about that. Inside a sealed enclosure on a roof, expect much less — it needs a **sound port**, and a sound port is another hole to seal (HW-027, HW-028).
- The LED is gone, so there is no longer any *persistent* indication of state. A beep tells you something happened; it does not tell you whether the device is currently on. That is the substance of **HW-044**, which stays open.
- Two pins driven in antiphase would give 6.6 Vp-p and about +6 dB. Optional — say the word and it goes in.

---

### 4. Ra-02: 3 of 4 GND pins unconnected — MAJOR (existing HW-007)

Connected: `U3.9`. Unconnected: `U3.1`, `U3.2`, `U3.16`.

HW-007 was written when the symbol showed three GND pins; this symbol has four. Nothing has changed on the board. At 433 MHz the return current wants the shortest path back, and one ground pin forces all of it through one point. This compounds **HW-004** (no ground plane).

**Fix:** wire all four to the switched ground net, each with its own wire.

---

### 5. Unconnected power pins and a stray symbol — MINOR

**Pro Mini.** `JP1_4 (VCC)`, `JP1_5 (GND)`, `JP1_6 (GND)`, `JP7_9 (GND_2)` have no wires. These are the same nets as `JP6_4 (VCC_1)` and `JP6_2 (GND_1)` inside the module, so nothing is electrically wrong — but every one of them is a pad that could tie the module into the ground pour. Leaving them open throws away stitching points on a board that already has HW-004 against it, and it fills the DRC report with unconnected-pin errors that will hide a real one.

**Fix:** wire `JP1_4` to BATT+, and `JP1_5`, `JP1_6`, `JP7_9` to the switched ground.

**Reed switch.** The `S1` symbol has `PartCount=2` and carries **two extra pins, also numbered 1 and 2**, parked at roughly **(1060, −180)** — far below and to the right of the drawing, off the visible sheet, unconnected. The pins that matter are wired correctly at (470, 260) and (470, 320).

**Fix:** open the S1 symbol and confirm whether the reed footprint really has 4 pads. If it has 2, delete the stray pair. If it has 4, wire or explicitly mark the spare pair as no-connect.

**Not a fault:** `U1.JP6_3 (RST_1)` is left open, which is correct — the Pro Mini module carries its own reset pull-up.

---

## LINE-BY-LINE AGAINST THE BUILD SHEET

Net names as they appear in the file: **`BATT+` = RED · `BATT-` = BLACK · `GND` = BLUE (switched) · N04 = GREEN.**

The green rail carries no net label. Worth adding one — call it `VLATCH` — so nobody mistakes it for BATT+ on the PCB.

### Stage 3 — 74HC74 (U2)

| Step | Wanted | Schematic | |
|---|---|---|---|
| 3.2 | pin 14 → GREEN | `U2.14 (VCC)` → N04 | ✅ |
| 3.3 | pin 7 → BLACK | `U2.7 (GND)` → BATT- | ✅ |
| 3.4 | pin 4 → GREEN | `U2.4 (1~SD)` → N04 | ✅ |
| 3.5 | pin 10 → GREEN | `U2.10 (2~SD)` → N04 | ✅ |
| 3.6 | pin 13 → GREEN | `U2.13 (2~RD)` → N04 | ✅ |
| 3.7 | pin 11 → BLACK | `U2.11 (2CP)` → BATT- | ✅ |
| 3.8 | pin 12 → BLACK | `U2.12 (2D)` → BATT- | ✅ |
| 3.9 | pin 2 → pin 6 | N10: `U2.2 (1D)`, `U2.6 (1~Q)` | ✅ |
| 3.10 | pins 8, 9 open | both unconnected | ✅ |

### Stage 4 — chip supply

| Step | Wanted | Schematic | |
|---|---|---|---|
| 4.1 | diode plain→RED, stripe→GREEN | `D1.2 (A)` → BATT+, `D1.1 (K)` → N04 | ✅ |
| 4.2 | 10 µF +→GREEN, −→BLACK | `C9.1 (P)` → N04, `C9.2 (N)` → BATT- | ✅ |
| 4.3 | 100 nF GREEN–BLACK | `C10` N04 ↔ BATT- | ✅ |

### Stage 5 — MOSFET

| Step | Wanted | Schematic | |
|---|---|---|---|
| 5.2 | drain → BLUE | `Q1.2 (D)` → GND | ✅ |
| 5.3 | source → BLACK | `Q1.3 (S)` → BATT- | ✅ |
| 5.4 | 1 MΩ gate → BLACK | `R8` (1 MΩ) N09 ↔ BATT- | ✅ |
| 5.5 | 1 kΩ gate → pin 5 | `R10` (1 kΩ) N09 ↔ `U2.5 (1Q)` | ✅ |

N09 has 3 nodes: `Q1.1 (G)`, `R10.2`, `R8.2`. Matches the build sheet's leg count.

### Stage 6 + 14.7 — 74HC74 pin 1

| Step | Wanted | Schematic | |
|---|---|---|---|
| 6.1 | 1 MΩ pin 1 → GREEN | `R11` (1 MΩ) N07 ↔ N04 | ✅ |
| 6.2 | 100 nF pin 1 → BLACK | `C11` N07 ↔ BATT- | ✅ |
| 14.7 | 100 kΩ A1 → pin 1 | `R9` (100 kΩ) A1 ↔ N07 | ✅ |

N07 has 4 nodes. Matches.

### Stage 7 + 14.6 — reed and 74HC74 pin 3

| Step | Wanted | Schematic | |
|---|---|---|---|
| 7.1 | reed leg 1 → GREEN | `S1.2` → N04 | ✅ |
| 7.2 | 100 Ω reed leg 2 → pin 3 | `R12` (100 Ω) `S1.1` ↔ N05 | ✅ |
| 7.3 | 470 kΩ pin 3 → BLACK | `R14` (470 kΩ) N05 ↔ BATT- | ✅ |
| 7.4 | 100 nF pin 3 → BLACK | `C12` N05 ↔ BATT- | ✅ |
| 14.6 | 100 Ω A0 → pin 3 | `R13` (100 Ω) A0 ↔ N05 | ✅ |

N05 has 5 nodes. Matches.

### Stage 9 — bulk decoupling

| Step | Wanted | Schematic | |
|---|---|---|---|
| 9.1 | 100 nF RED–BLACK | `C4` | ✅ |
| 9.2 | 100 nF RED–BLUE | `C2`, `C3`, `C5`, `C6` (four of them, one per device) | ✅ |

### Stage 10 — ultrasonic (J3)

| Step | Wanted | Schematic | |
|---|---|---|---|
| 10.2 | +5V pin → RED | on `J3.2`, **wrong position** | ❌ fault 2 |
| 10.3 | GND pin → BLUE | `J3.1` → GND | ✅ |
| 10.4 | 100 nF across the connector | `C2`, sits beside J3 | ✅ |
| 14.12 | 100 Ω D6 → RX | `R2` D6 ↔ `J3.4`, **wrong position** | ❌ fault 2 |
| 14.13 | 100 Ω D8 → TX | `R1` D8 ↔ `J3.3` | ✅ |

Echo is on **D8**, not D7. **HW-018 is fixed.**

### Stage 11 — temperature (J2)

| Step | Wanted | Schematic | |
|---|---|---|---|
| 11.2 | GND pin → BLUE | `J2.2` → GND | ✅ |
| 11.3 | 4.7 kΩ DATA → VCC | `R7` `J2.1` ↔ `J2.3` | ✅ |
| 14.10 | **D3 → VCC pin** | **missing** | ❌ fault 1 |
| 14.11 | 100 Ω D4 → DATA | `R4` D4 ↔ `J2.1` | ✅ |

Pin order on the connector is **DATA · GND · VCC** — the reverse of the build sheet's `VCC · GND · DATA`. Harmless with a hand-made cable, but update the build sheet label and the silkscreen (HW-038) so the two agree.

### Stage 12 — flow switch (J1)

| Step | Wanted | Schematic | |
|---|---|---|---|
| 12.2 | pin B → BLUE | `J1.1` → GND | ✅ |
| 12.3 | 1 MΩ pin A → RED | `R6` (1 MΩ) N06 ↔ BATT+ | ✅ |
| 12.4 | 100 nF pin A → BLUE | `C1` N06 ↔ GND | ✅ |
| 14.8 | 330 Ω A2 → pin A | `R3` (330 Ω) A2 ↔ N06 | ✅ |
| 14.9 | 100 Ω D5 → pin A | `R5` (100 Ω) D5 ↔ N06 | ✅ |

N06 has 5 nodes. Matches. **HW-020 is fixed.**

### Stage 13 — radio (U3)

| Step | Wanted | Schematic | |
|---|---|---|---|
| 13.2–13.5 | four separate GND wires | only `U3.9` | ❌ fault 4 |
| 13.6 | 3V3 → RED | `U3.3` → BATT+ | ✅ |
| 13.7 | 100 nF at the socket | `C6` | ✅ |
| 13.8 | 10 µF at the socket | `C8` (10 µF, 50 V) | ✅ |
| 13.9 | 100 µF at the socket | `C7` (100 µF, 10 V) | ✅ |
| 13.10 | DIO1–DIO5 open | all five unconnected | ✅ |

### Stage 14 — Pro Mini (U1)

| Step | Wanted | Schematic | |
|---|---|---|---|
| 14.2 | VCC → RED | `JP6_4` → BATT+ | ✅ |
| 14.3 | GND → BLUE | `JP6_2` → GND | ✅ |
| 14.4 | 100 nF VCC–GND | `C5` | ✅ |
| 14.5 | RAW open | `JP6_1` unconnected | ✅ |
| 14.14 | D2 → DIO0 | `U3.5` | ✅ |
| 14.15 | D9 → RESET | `U3.4` | ✅ |
| 14.16 | D10 → NSS | `U3.15` | ✅ |
| 14.17 | D11 → MOSI | `U3.14` | ✅ |
| 14.18 | D12 → MISO | `U3.13` | ✅ |
| 14.19 | D13 → SCK | `U3.12` | ✅ |
| 14.20–21 | 1 kΩ + LED on D7 | buzzer direct, no resistor | ❌ fault 3 |
| 14.22 | A3–A7, D0, D1, RST, DTR open | all unconnected | ✅ |

---

## PART COUNT

Every value the build sheet calls for is present, and nothing extra.

| Value | Wanted | In the schematic | |
|---|---|---|---|
| 100 nF | 9 | C1 C2 C3 C4 C5 C6 C10 C11 C12 | ✅ |
| 10 µF | 2 | C8 (radio), C9 (latch supply) | ✅ |
| 100 µF | 1 | C7 (radio) | ✅ |
| 100 Ω | 6 | R1 R2 R4 R5 R12 R13 | ✅ |
| 330 Ω | 1 | R3 | ✅ |
| 1 kΩ | 2 | R10 only | ⚠️ second one was the LED resistor — now needed for the buzzer, at 100 Ω |
| 4.7 kΩ | 1 | R7 | ✅ |
| 100 kΩ | 1 | R9 | ✅ |
| 470 kΩ | 1 | R14 | ✅ |
| 1 MΩ | 3 | R6 R8 R11 | ✅ |
| 1N5819 | 1 on board | D1 | ✅ |

The battery pack's **2 × 1N5819 and the 0.5 A fuse are not on this sheet**. That is per HW-003 — they live inside the sealed pack. The sheet should still carry a note saying so, or the pack will get built without them.

---

## WHAT THIS SCHEMATIC FIXED

| Issue | Was | Now |
|---|---|---|
| HW-009 / HW-034 | 2200 µF electrolytic, wrong part, wrong voltage rating | gone; 100 µF 10 V + 10 µF 50 V at the radio |
| HW-013 | two bypass caps, neither local | nine 100 nF, one per device, plus bulk at the radio |
| HW-014 / HW-043 | reed straight onto the clock pin, no debounce | 100 Ω series, 470 kΩ + 100 nF giving ~47 ms recovery |
| HW-016 | blue LED with no forward-voltage headroom | LED removed |
| HW-018 | echo on D7, no input capture | echo on D8 |
| HW-020 | flow input bare | 1 MΩ pull-up, 100 nF filter, 100 Ω and 330 Ω series |
| HW-021 | MCU blind to the latch, no commanded shutdown | A0 reads the reed line, A1 drives ~RD through 100 kΩ |
| HW-037 / HW-041 | CD4013BE, 3 V floor | 74HC74N, 2 V floor |
| HW-042 | latch supply straight off VBAT | D1 + 10 µF + 100 nF holding it up through a TX burst |

---

## FULL NETLIST

Rebuild any time with:

```
python3 tools/extract_netlist.py "Hydro Node Device/Hydro Node Parts & Schematic/Schematic/Hydro Node Schematic.SchDoc"
```

| Net | Pins | Members |
|---|---|---|
| **N01 `GND`** (switched, BLUE) | 14 | C1.2, C2.2, C3.1, C5.1, C6.2, C7.2(N), C8.2(N), J1.1, J2.2, J3.1, LS1.N, Q1.2(D), U1.GND_1, U3.9 |
| **N02 `BATT+`** (RED) | 13 | BATT.2, C2.1, C3.2, C4.2, C5.2, C6.1, C7.1(P), C8.1(P), D1.2(A), J3.2, R6.1, U1.VCC_1, U3.3 |
| **N03 `BATT-`** (BLACK) | 12 | BATT.1, C4.1, C9.2(N), C10.2, C11.2, C12.2, Q1.3(S), R8.1, R14.2, U2.7, U2.11, U2.12 |
| N04 (GREEN, unlabelled) | 9 | C9.1(P), C10.1, D1.1(K), R11.1, S1.2, U2.4, U2.10, U2.13, U2.14 |
| N05 (74HC74 pin 3) | 5 | C12.1, R12.2, R13.2, R14.1, U2.3 |
| N06 (flow sense) | 5 | C1.1, J1.2, R3.1, R5.2, R6.2 |
| N07 (74HC74 pin 1) | 4 | C11.1, R9.2, R11.2, U2.1 |
| N08 (probe DATA) | 3 | J2.1, R4.2, R7.2 |
| N09 (MOSFET gate) | 3 | Q1.1(G), R8.2, R10.2 |
| N10 | 2 | U2.2 (1D), U2.6 (1~Q) |
| N11 | 2 | R10.1, U2.5 (1Q) |
| N12 | 2 | R13.1, U1.A0 |
| N13 | 2 | R9.1, U1.A1 |
| N14 | 2 | R3.2, U1.A2 |
| N15 | 2 | U1.SCK, U3.12 |
| N16 | 2 | U1.MISO, U3.13 |
| N17 | 2 | U1.MOSI, U3.14 |
| N18 | 2 | U1.D10, U3.15 (NSS) |
| N19 | 2 | U1.D2, U3.5 (DIO0) |
| N20 | 2 | R4.1, U1.D4 |
| N21 | 2 | R5.1, U1.D5 |
| N22 | 2 | R2.2, U1.D6 |
| N23 | 2 | LS1.P, U1.D7 |
| N24 | 2 | R1.2, U1.D8 |
| N25 | 2 | U1.D9, U3.4 (RESET) |
| N26 | 2 | J3.3, R1.1 |
| N27 | 2 | J3.4, R2.1 |
| N28 | 2 | J2.3, R7.1 |
| N29 | 2 | R12.1, S1.1 |

### Pins with no wire

| Part | Pins | |
|---|---|---|
| S1 | 1, 2 (the stray off-sheet pair) | ❌ fault 5 |
| U1 | A3, A4, A5, A6, A7, RST_1, RST_2, RAW, DTR, TXO, RXI, TXO_2, RXI_2 | ✅ intended |
| U1 | **D3** | ❌ fault 1 |
| U1 | VCC (JP1_4), GND (JP1_5), GND (JP1_6), GND_2 (JP7_9) | ❌ fault 5 |
| U2 | 8 (2~Q), 9 (2Q) | ✅ intended |
| U3 | 6, 7, 8, 10, 11 (DIO1–DIO5) | ✅ intended |
| U3 | 1, 2, 16 (GND) | ❌ fault 4 |

---

## HOW THE NETLIST WAS READ

`.SchDoc` is an OLE compound file. The sheet has **3 net labels, no power ports, 72 wires and 46 junction dots**, so connectivity is geometry, not names.

One detail decides whether the answer is right or garbage. A pin record stores `Location.X/Y` — where the pin meets the component body — plus `PinLength` and an orientation. The end a wire attaches to is:

```
Location + PinLength × direction
```

Testing both ends instead merges nets that are not connected. On this sheet it shorted C1 and R7 across their own pins, because a wire runs past the far end of J1's and J2's pins. `tools/extract_netlist.py` now uses the correct end and self-checks by asserting that **no two-pin part has both pins on one net** — that check passes on this file.

---

## BEFORE THE PCB

Fix the five faults, then re-run the extractor and diff against this file. Also carry forward:

- **HW-004** — ground plane. Still the open blocker on the PCB side.
- **HW-013** — the nine 100 nF only work if each one is placed at the device it belongs to: C5 at the Pro Mini, C6 at the radio, C2 at J3, C10 at the 74HC74. The schematic cannot express that; the layout has to.
- **HW-038** — connector functions on the silkscreen. J2's order is DATA · GND · VCC, and J3's needs correcting to GND · RX · TX · +5V.
- **HW-040** — RF keep-out under the antenna feed.
- Give the green rail a net label so it is not confused with BATT+.
