# SCHEMATIC CHECK — Hydro Node Schematic.SchDoc

Second pass, 2026-08-22. Checked against `BUILD-SHEET.md`.
Netlist rebuilt from the file with `tools/extract_netlist.py`.

---

## VERDICT

**The wiring is correct. All 34 build-sheet connections check out.**

Nothing in the netlist is wrong any more. What is left are **library and part-choice problems**, and they are the kind that only show up after the boards arrive — so they are worth an hour now.

| | | Effect if ignored |
|---|---|---|
| ❌ | **Ra-02 footprint pitch is unverified** | The module is 2.0 mm pitch. Generic 2×8 through-hole footprints are 2.54 mm. If it is wrong, the boards are scrap. |
| ❌ | **C7, C8, C9 are aluminium electrolytics** | Rated 105 °C / 4000 h. At the 70–85 °C this enclosure reaches, that is 1.8–3.6 years — inside the 2-year target. |
| ❌ | **S1's part number is a surface-mount reed** | Its footprint is through-hole. The BOM would order a part that does not fit. |
| ❌ | **S1 has a stray second pin pair off-sheet** | Two pins also numbered 1 and 2, parked at (1060, −180). |
| ⚠️ | 8 parts have no manufacturer part number | BOM lines come out blank. |
| ⚠️ | Green rail has no net label, 3 wire stubs overshoot | Cosmetic. |

---

## WHAT YOU FIXED SINCE THE LAST PASS

| | Was | Now |
|---|---|---|
| ✅ **HW-055** | buzzer straight onto D7 | **R15 100 Ω** between D7 and `LS1.P`. Peak edge current now ~36 mA, inside the pin's 40 mA limit. |
| ✅ **HW-007** | one Ra-02 GND pin connected | all four — `U3.1`, `U3.2`, `U3.9`, `U3.16` |
| ✅ **HW-056** | 4 Pro Mini power/ground pads open | `JP1_4` on BATT+, `JP1_5`/`JP1_6`/`JP7_9` on the switched ground |
| ✅ **HW-053** | probes had no supply | `J2.3` tied to **BATT+**, `R7` pulls up to the same rail |
| ✅ **HW-036** | carbon film ½ W | every resistor is a Yageo **MFR-25, metal film, ¼ W, 1 %** |

### On the two decisions you made

**The buzzer replacing the LED — agreed, and it is the better choice.** A beep after the magnet tells you the device came on without having to be looking at the box, on a roof, in sunlight. Two things follow from it, both firmware, both now written into **HW-044**:

- ON and OFF need **distinguishable patterns** — one beep and two beeps. Otherwise a magnet pass that does nothing is indistinguishable from one that turned the device off.
- **The OFF beep has to finish before A1 pulls the latch down.** Once the latch drops, the switched ground is gone and the buzzer is dead mid-note.

**The ultrasonic pin order — accepted as intentional, closed WON'T FIX (HW-054).** It is off the fault list. One consequence is now permanent and is recorded in **HW-001**: the harness is a **cross-over cable**, forever.

| J3 pin on the board | must wire to module pad |
|---|---|
| 1 — GND | GND (pad 1) |
| 2 — +5V | **+5V (pad 4)** |
| 3 — TX | TX (pad 3) |
| 4 — RX | **RX (pad 2)** |

A straight 1:1 cable puts 3.6 V on the module's RX pin and leaves the module unpowered. Nobody can work the crossover out from looking at the board, so the harness drawing has to exist before anyone builds a cable.

### On how HW-053 was fixed

You tied `J2.3` to BATT+ rather than switching it from D3. That works, and the ground switch still removes the probes completely when the device is off. Two things for the record:

- The probes now draw standby current the whole time the device is **on**, not only during a reading — about **1.5 µA** for the pair at the DS18B20's typical 750 nA, roughly **26 mAh over two years**. Under 1 % of a 4.4 Ah pack. Not worth another wire.
- **D3 is now a spare pin.**

---

## THE FOUR THINGS LEFT

### 1. Verify the Ra-02 footprint pitch before you route anything — HW-060

`U3` uses `RA-02_BREAKOUT_THT_2X8`. The **Ai-Thinker Ra-02 is a 2.0 mm pitch part** — 16 pads, two rows of eight. Generic "2×8 through-hole" footprints are usually drawn at 2.54 mm, the ordinary header pitch.

The pitch lives in the PcbLib, not in the `.SchDoc`, so I cannot read it. **Open it and measure it against the module in your hand.** If it is 2.54 mm the module will not fit, and you will not find that out until the boards and the parts are both paid for.

While you are in there, check two more:

| Footprint | Should be |
|---|---|
| `MODULE_ARDUINO_PRO_MINI` | 2.54 mm |
| `FP-B2B/B3B/B4B-XH-A…` | **2.50 mm** — JST XH is a metric 2.5 mm series, not 2.54. Across a 4-way part the difference is 0.12 mm. |

`BUILD-SHEET.md` stage 13 has the Ra-02 in sockets. If that is still the production intent, the footprint must be the **socket's** pitch, and the socket has to accept a 2.0 mm module.

---

### 2. C7, C8 and C9 should not be aluminium electrolytics — HW-058

All three use `WCAP-ATLL_D5H11`. Their own parameters in your schematic say:

| | C7 | C8 | C9 |
|---|---|---|---|
| Value | 100 µF | 10 µF | 10 µF |
| Rating | 10 V | 50 V | 50 V |
| Leakage | 10 µA | 5 µA | 5 µA |
| Life | 4000 h at 105 °C | 4000 h at 105 °C | 4000 h at 105 °C |

**Problem one — they wear out inside the target life.** Aluminium electrolytics are the one capacitor family that dries out. Life doubles for every 10 °C below the rating, and **HW-027** puts the inside of this enclosure at **70–85 °C** on a Syrian roof in summer:

| Internal temperature | Life from 4000 h at 105 °C |
|---|---|
| 75 °C | ~32,000 h ≈ **3.6 years** |
| 85 °C | ~16,000 h ≈ **1.8 years** |

At the top of that range they are finished before the battery is. And the failure is not "no capacitance" — as they dry, ESR climbs, which is exactly the property the transmit burst depends on. The supply sags a little further every month until the latch drops out. That is **HW-042** returning through a different door.

**Problem two — C9 leaks on a rail the magnet cannot switch off.** C9 sits between the latch rail and BATT-, and both are live whether the device is on or off. Its leakage is a permanent drain against a 25 µA sleep target. At 3.6 V on a 50 V part the real figure will be well below the 5 µA maximum — but leakage climbs steeply with temperature, and nothing guarantees it hot.

**Fix:**

| | Change to | Why |
|---|---|---|
| **C9** | ceramic **10 µF 16 V X7R 1206** or tantalum | the one that matters most — always-live rail, and it is what holds the latch through a transmit |
| **C7** | **tantalum 100 µF**, or ceramic **1210 at 16 V or 25 V** | ceramic at 6.3 V loses 50–70 % of its value at 3.6 V to DC bias; tantalum does not derate |
| **C8** | ceramic **10 µF 16 V X7R 1206** | — |

Neither ceramic nor tantalum dries out, and both leak orders of magnitude less. `BUILD-SHEET.md` already called for a tantalum 100 µF for the DC-bias reason — this is a second, independent reason, and it applies to all three.

---

### 3. S1's part number does not match its footprint — HW-059

| Field | Says |
|---|---|
| Manufacturer Part Number | `MDSM-4R-12-18` — a Littelfuse **surface-mount** reed, glass 15.24 × 2.28 mm, SPST-NO, 12–18 AT |
| Footprint | `REEDSW-THT-D4L29-P35` — **through-hole**, 29 mm, 4 mm diameter |
| The part on your bench | axial through-hole glass reed with long wire leads (`Components Images/F2293658-01.jpg`) |

The BOM is generated from the schematic. Ordered as written, an SMD part arrives that does not fit the board.

**Fix:** measure the reed you are actually fitting — glass length, overall length, lead diameter — set the MPN to that part, then open `REEDSW-THT-D4L29-P35` and check the pad pitch against your measurement. Do it together with the next item, since both are the same symbol.

---

### 4. S1 still has a stray second pin pair off-sheet — HW-057

The symbol has `PartCount=2` and **four pin records, two of them also numbered 1 and 2**, parked at roughly **(1060, −180)** — outside the visible drawing, unconnected. The pins that matter are wired correctly at (470, 260) and (470, 320).

If the footprint has two pads and the symbol has two pins numbered 1 and two numbered 2, the PCB import has to guess. **Delete the stray pair** if the reed is a two-lead part, which the one in your photo is. If the footprint really has four pads, wire them or mark them no-connect explicitly.

---

## SMALLER THINGS

**8 parts have no manufacturer part number** — `BATT`, `J1`, `J2`, `J3`, `C7`, `C8`, `C9`, `Q1`. Those BOM lines come out blank. Fill the three capacitors in only *after* deciding HW-058, so the part number and the dielectric get chosen together.

**Give the green rail a net label.** N04 — the 74HC74's supply, off BATT+ through D1 — is the only significant net with no name. Call it `VLATCH`. On the PCB it looks like just another power net and it is the one that must never be confused with BATT+.

**Three wire stubs overshoot their last pin by 30 mil** — past `U3.3` at (220, 290), past `U3.4` at (220, 270), past `U3.9` at (310, 140). Electrically nothing; the connections are made. Tidy-up only.

**No ERC markers.** Put them on every pin meant to stay open — A3–A7, RAW, DTR, TXO, RXI, the duplicate RST/TXO/RXI, U2 pins 8 and 9, and the Ra-02's DIO1–DIO5 — so the DRC report goes to zero. A report with fourteen expected errors in it is a report nobody reads.

---

## WHAT WAS CHECKED AND IS CLEAN

| Check | Result |
|---|---|
| All 34 build-sheet connections | ✅ every one correct |
| Two-pin parts shorted across themselves | ✅ none |
| Nets with only one pin | ✅ none |
| Every component has a current PCB footprint | ✅ all 38 |
| Duplicate designators | ✅ none |
| Junction dots sitting on fewer than two wires | ✅ none |
| Wire crossings without a dot | 45 — all correct, none of them a missed connection |
| Unconnected pins | 22, all intended, except S1's stray pair |
| Component values against the build sheet | ✅ all match |

---

## LINE-BY-LINE AGAINST THE BUILD SHEET

Net names as they appear in the file: **`BATT+` = RED · `BATT-` = BLACK · `GND` = BLUE (switched) · N04 = GREEN (unlabelled).**

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
| 4.3 | 100 nF GREEN–BLACK | `C10` | ✅ |

### Stage 5 — MOSFET

| Step | Wanted | Schematic | |
|---|---|---|---|
| 5.2 | drain → BLUE | `Q1.2 (D)` → GND | ✅ |
| 5.3 | source → BLACK | `Q1.3 (S)` → BATT- | ✅ |
| 5.4 | 1 MΩ gate → BLACK | `R8` N09 ↔ BATT- | ✅ |
| 5.5 | 1 kΩ gate → pin 5 | `R10` N09 ↔ `U2.5 (1Q)` | ✅ |

### Stage 6 + 14.7 — 74HC74 pin 1

| Step | Wanted | Schematic | |
|---|---|---|---|
| 6.1 | 1 MΩ pin 1 → GREEN | `R11` N07 ↔ N04 | ✅ |
| 6.2 | 100 nF pin 1 → BLACK | `C11` N07 ↔ BATT- | ✅ |
| 14.7 | 100 kΩ A1 → pin 1 | `R9` A1 ↔ N07 | ✅ |

### Stage 7 + 14.6 — reed and 74HC74 pin 3

| Step | Wanted | Schematic | |
|---|---|---|---|
| 7.1 | reed leg 1 → GREEN | `S1.2` → N04 | ✅ |
| 7.2 | 100 Ω reed leg 2 → pin 3 | `R12` `S1.1` ↔ N05 | ✅ |
| 7.3 | 470 kΩ pin 3 → BLACK | `R14` N05 ↔ BATT- | ✅ |
| 7.4 | 100 nF pin 3 → BLACK | `C12` N05 ↔ BATT- | ✅ |
| 14.6 | 100 Ω A0 → pin 3 | `R13` A0 ↔ N05 | ✅ |

### Stage 9 — bulk decoupling

| Step | Wanted | Schematic | |
|---|---|---|---|
| 9.1 | 100 nF RED–BLACK | `C4` | ✅ |
| 9.2 | 100 nF RED–BLUE | `C2` `C3` `C5` `C6` — one per device | ✅ |

### Stage 10 — ultrasonic (J3)

| Step | Wanted | Schematic | |
|---|---|---|---|
| 10.2 | supply pin → RED | `J3.2` → BATT+ | ✅ *(position intentional — HW-054)* |
| 10.3 | GND pin → BLUE | `J3.1` → GND | ✅ |
| 10.4 | 100 nF across the connector | `C2`, beside J3 | ✅ |
| 14.12 | 100 Ω D6 → RX | `R2` D6 ↔ `J3.4` | ✅ |
| 14.13 | 100 Ω D8 → TX | `R1` D8 ↔ `J3.3` | ✅ |

Echo is on **D8**, so ICP1 is available.

### Stage 11 — temperature (J2)

| Step | Wanted | Schematic | |
|---|---|---|---|
| 11.2 | GND pin → BLUE | `J2.2` → GND | ✅ |
| 11.3 | 4.7 kΩ DATA → VCC | `R7` `J2.1` ↔ BATT+ | ✅ |
| 14.10 | probe supply | `J2.3` → BATT+ | ✅ *(BATT+ instead of D3)* |
| 14.11 | 100 Ω D4 → DATA | `R4` D4 ↔ `J2.1` | ✅ |

Connector order is **DATA · GND · VCC** — reverse of the build sheet's label. Update the build sheet and the silkscreen (**HW-038**) so they agree.

### Stage 12 — flow switch (J1)

| Step | Wanted | Schematic | |
|---|---|---|---|
| 12.2 | pin B → BLUE | `J1.1` → GND | ✅ |
| 12.3 | 1 MΩ pin A → RED | `R6` N06 ↔ BATT+ | ✅ |
| 12.4 | 100 nF pin A → BLUE | `C1` N06 ↔ GND | ✅ |
| 14.8 | 330 Ω A2 → pin A | `R3` A2 ↔ N06 | ✅ |
| 14.9 | 100 Ω D5 → pin A | `R5` D5 ↔ N06 | ✅ |

### Stage 13 — radio (U3)

| Step | Wanted | Schematic | |
|---|---|---|---|
| 13.2–13.5 | four GND wires | `U3.1` `U3.2` `U3.9` `U3.16` all on GND | ✅ |
| 13.6 | 3V3 → RED | `U3.3` → BATT+ | ✅ |
| 13.7–13.9 | 100 nF + 10 µF + 100 µF at the socket | `C6` `C8` `C7` | ✅ *(dielectric — HW-058)* |
| 13.10 | DIO1–DIO5 open | all five unconnected | ✅ |

### Stage 14 — Pro Mini (U1)

| Step | Wanted | Schematic | |
|---|---|---|---|
| 14.2 | VCC → RED | `JP6_4` and `JP1_4` → BATT+ | ✅ |
| 14.3 | GND → BLUE | `JP6_2` `JP1_5` `JP1_6` `JP7_9` → GND | ✅ |
| 14.4 | 100 nF VCC–GND | `C5` | ✅ |
| 14.5 | RAW open | unconnected | ✅ |
| 14.14–19 | D2→DIO0, D9→RESET, D10→NSS, D11→MOSI, D12→MISO, D13→SCK | all six | ✅ |
| 14.20–21 | indicator on D7 | `R15` 100 Ω → `LS1.P` | ✅ |
| 14.22 | A3–A7, D0, D1, RST, DTR open | all unconnected | ✅ |

---

## PART COUNT

| Value | In the schematic | |
|---|---|---|
| 100 nF | C1 C2 C3 C4 C5 C6 C10 C11 C12 — 9 | ✅ |
| 10 µF | C8 (radio), C9 (latch) — 2 | ✅ dielectric: HW-058 |
| 100 µF | C7 (radio) — 1 | ✅ dielectric: HW-058 |
| 100 Ω | R1 R2 R4 R5 R12 R13 **R15** — **7** | ✅ buy list was 6 |
| 330 Ω | R3 | ✅ |
| 1 kΩ | R10 — 1 | ✅ the second one is no longer needed |
| 4.7 kΩ | R7 | ✅ |
| 100 kΩ | R9 | ✅ |
| 470 kΩ | R14 | ✅ |
| 1 MΩ | R6 R8 R11 — 3 | ✅ |
| 1N5819 | D1 | ✅ |

The pack's **2 × 1N5819 and the 0.5 A fuse are not on this sheet** — they live inside the sealed pack per HW-003. Put a note on the sheet saying so, or the pack gets built without them.

---

## THE THREE PINS YOU ASKED ABOUT

### A0 — lets the MCU see the magnet

`A0` → **R13 100 Ω** → 74HC74 pin 3, which is the flip-flop's clock input and the node the reed drives.

The reed's job is to flip the latch. A0 is just watching that same wire, so firmware also knows a magnet is present — how long you held it, how many times you passed it. That is what a "hold the magnet 5 seconds to unpair" gesture would be built from (**HW-022**). The 100 Ω only protects the pin.

### A1 — lets the MCU turn itself off, and read whether it is on

`A1` → **R9 100 kΩ** → 74HC74 pin 1, the flip-flop's `1~RD` — its **reset, active low**.

Drive A1 low and the latch resets, Q drops, the MOSFET opens, the switched ground disconnects, and the whole board dies. That is the commanded shutdown from **HW-021** — for a critically low battery, or a "shut down" command from the Hub. Leave A1 as an input and it reads the same node, so firmware can also ask "am I on?".

**Why 100 kΩ and not a plain wire:** pin 1 is already held high by **R11, 1 MΩ**, from the latch rail. Wiring A1 straight to it would mean the pin has to sit high-impedance the whole time or it fights R11. With 100 kΩ against 1 MΩ, driving A1 low pulls the node to about **0.3 V** — a solid logic low for an HC part — and releasing A1 lets the 1 MΩ take it back high. One resistor turns one pin into both a reader and a driver.

### A2 and D5 — two pins on the flow switch, because they do two different jobs

The flow switch is just a dry contact. Its node (N06) carries four things:

| Part | Job |
|---|---|
| **R6 1 MΩ** to BATT+ | holds the node HIGH while the switch is open |
| **C1 100 nF** to ground | filters noise picked up on a long cable |
| **R5 100 Ω** to **D5** | the **driver** |
| **R3 330 Ω** to **A2** | the **reader** |

**D5 wakes the contact up.** Contacts that only ever carry microamps grow an oxide film and stop conducting even when closed. Just before a reading, firmware drives D5 high for a moment. Through 100 Ω that pushes about **36 mA** through the closed contact, which burns the film off. That is the wetting current in **HW-019**.

**A2 does the reading.** D5 then goes back to being an input, and A2 reads the node through 330 Ω. Switch closed → the node is pulled to ground → reads LOW. Switch open → the 1 MΩ holds it HIGH.

**Why not one pin for both?** Because the two jobs want opposite resistor values. The driver needs a *small* resistor to deliver real current. The reader wants a *bigger* one, so a fault out on the cable cannot damage the pin. One pin would force one compromise value that is bad at both. Splitting them also means the wetting pulse only happens when you ask for it — a few tens of microseconds every few minutes, which is why HW-019 put its cost at about **0.5 % of the energy budget**.

---

## FULL NETLIST

```
python3 tools/extract_netlist.py "Hydro Node Device/Hydro Node Parts & Schematic/Schematic/Hydro Node Schematic.SchDoc"
```

| Net | Pins | Members |
|---|---|---|
| **N01 `GND`** (switched, BLUE) | 20 | C1.2, C2.2, C3.1, C5.1, C6.2, C7.2(N), C8.2(N), J1.1, J2.2, J3.1, LS1.N, Q1.2(D), U1.JP1_5, U1.JP1_6, U1.GND_1, U1.GND_2, U3.1, U3.2, U3.9, U3.16 |
| **N02 `BATT+`** (RED) | 16 | BATT.2, C2.1, C3.2, C4.2, C5.2, C6.1, C7.1(P), C8.1(P), D1.2(A), J2.3, J3.2, R6.1, R7.1, U1.JP1_4, U1.VCC_1, U3.3 |
| **N03 `BATT-`** (BLACK) | 12 | BATT.1, C4.1, C9.2(N), C10.2, C11.2, C12.2, Q1.3(S), R8.1, R14.2, U2.7, U2.11, U2.12 |
| N04 (GREEN — needs a label) | 9 | C9.1(P), C10.1, D1.1(K), R11.1, S1.2, U2.4, U2.10, U2.13, U2.14 |
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
| N15–N18 | 2 each | SCK, MISO, MOSI, NSS to U3.12–15 |
| N19 | 2 | U1.D2, U3.5 (DIO0) |
| N20 | 2 | R4.1, U1.D4 |
| N21 | 2 | R5.1, U1.D5 |
| N22 | 2 | R2.2, U1.D6 |
| N23 | 2 | R15.2, U1.D7 |
| N24 | 2 | R1.2, U1.D8 |
| N25 | 2 | U1.D9, U3.4 (RESET) |
| N26 | 2 | J3.3, R1.1 |
| N27 | 2 | J3.4, R2.1 |
| N28 | 2 | R12.1, S1.1 |
| N29 | 2 | LS1.P, R15.1 |

---

## FOOTPRINTS

All 38 components have a current PCB model assigned.

| Ref | Footprint | |
|---|---|---|
| U1 | `MODULE_ARDUINO_PRO_MINI` | verify pitch |
| U2 | `DIP254P762X420-14` | ✅ DIP-14 |
| U3 | `RA-02_BREAKOUT_THT_2X8` | ❌ **verify pitch — HW-060** |
| Q1 | `FP-PG-TO220-3-MFG` | ✅ TO-220 (package choice is HW-017) |
| D1 | `ONSC-AXIAL_LEAD-2-59-10_P` | ✅ DO-41 |
| S1 | `REEDSW-THT-D4L29-P35` | ❌ **MPN mismatch — HW-059** |
| LS1 | `CUI_CPT-1255C-090` | ✅ |
| BATT | `JST_B2B-XH-A_(LF)(SN)` | ✅ |
| J1 / J2 / J3 | `FP-B2B/B3B/B4B-XH-A_LF_SN-MFG` | verify 2.50 mm |
| C1–C6, C10–C12 | `CAPRR508W50L508T317H508` | ✅ AVX SR215, 5.08 mm |
| C7, C8, C9 | `WCAP-ATLL_D5H11` | ❌ **dielectric — HW-058** |
| R1–R15 | `FP-MFR-25-MFG` | ✅ Yageo MFR-25 metal film ¼ W 1 % |

---

## HOW THE NETLIST WAS READ

`.SchDoc` is an OLE compound file. The sheet has **3 net labels, no power ports, 80 wires and 51 junction dots**, so connectivity is geometry, not names.

Two details decide whether the answer is right or garbage, and both bit me on the first pass:

1. **A pin's electrical end is `Location + PinLength × direction`**, not `Location`, and not both ends. Testing both ends falsely shorted C1 and R7 across their own pins.
2. **Child records reference their parent as `OwnerIndex + 1`.** Getting that wrong made every component look like it had no footprint.

`tools/extract_netlist.py` handles both and self-checks that no two-pin part has both pins on one net. That check passes on this file.

---

## BEFORE THE PCB

**Do first — these are cheap now and expensive later:**

1. **Measure the Ra-02 footprint pitch** against the module. (HW-060)
2. **Change C7, C8, C9** to tantalum or ceramic. (HW-058)
3. **Fix S1** — the part number, and the stray pin pair. (HW-059, HW-057)

**Do while you are in the schematic:**

4. Label the green rail `VLATCH`.
5. Fill in the 8 missing manufacturer part numbers. (HW-033)
6. No ERC markers on every intentionally open pin.
7. Trim the three overshooting wire stubs.

**Carry into the layout:**

- **HW-004** — ground plane. The remaining blocker on the PCB side.
- **HW-013** — the nine 100 nF only work if each is placed at the device it belongs to: C5 at the Pro Mini, C6 at the radio, C2 at J3, C10 at the 74HC74.
- **HW-007** — four Ra-02 GND pads means four short ties into the pour plus stitching vias, not one wire branching four ways.
- **HW-038** — silkscreen. J2 reads `DATA · GND · VCC`; J3 reads `GND · +5V · TX · RX`.
- **HW-040** — RF keep-out under the antenna feed.
- **HW-001** — the ultrasonic harness drawing, with the cross-over table, before anyone builds a cable.
