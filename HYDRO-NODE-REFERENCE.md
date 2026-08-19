# HYDRO NODE — STAGE 0 REVIEW EVIDENCE

Supporting material for `HYDRO-NODE-HARDWARE-ISSUES.md`. **The issue tracker is the source of truth for findings**; this file only holds the evidence those findings were derived from, so the tracker stays clean enough to copy-paste in one go.

Everything here was extracted programmatically from the Altium files, not read off the rendered image.

---

## 1. Extracted netlist (from `Hydro-Node.SchDoc`)

There are **no net labels and no power ports** anywhere in this schematic — every connection is made by drawn wire geometry. The netlist below was rebuilt from the wire vertices, junctions and pin coordinates in the binary file.

| Net | Function | Connections |
|---|---|---|
| NET01 | **GND_RAW** (battery −) | BATT.1, C1.1, C4.2, Q1.3 (S), R1.2, R3.2, R4.2, U1.6 (SET1), U1.7 (VSS), U1.8 (SET2), U1.9 (D2), U1.10 (RESET2), U1.11 (CLOCK2) |
| NET02 | **VBAT** (battery +, unswitched) | BATT.2, C2.1, C3.2 (+), C4.1, C5.2, J1.3 (LoRa 3V3), J5.2 (Ultrasonic VCC), S1.2, U1.14 (VDD), U2.VCC |
| NET03 | **GND_SW** (switched load ground) | C3.1 (−), C5.1, DS1.2 (K), J2.8 (LoRa GND), J3.2 (Temp GND), J4.2 (Flow GND), J5.1 (Ultrasonic GND), Q1.2 (D), U2.GND |
| NET04 | Reed / clock node | C1.2, R3.1, S1.1, U1.3 (CLOCK1) |
| NET05 | Temp VCC | J3.3, R6.2, U2.D3 |
| NET06 | 1-Wire data | J3.1, R6.1, U2.D4 |
| NET07 | Power-on reset | C2.2, R4.1, U1.4 (RESET1) |
| NET08 | MOSFET gate | Q1.1 (G), R1.1, R2.2 |
| NET09 | SPI SCK | J2.5, U2.D13 |
| NET10 | SPI MISO | J2.4, U2.D12 |
| NET11 | SPI MOSI | J2.3, U2.D11 |
| NET12 | LoRa NSS | J2.2, U2.D10 |
| NET13 | LoRa DIO0 | J1.5, U2.D2 |
| NET14 | Flow input | J4.1, U2.D5 |
| NET15 | Ultrasonic Trig | J5.4, U2.D6 |
| NET16 | Ultrasonic Echo | J5.3, U2.D7 |
| NET17 | LED drive | R5.2, U2.D8 |
| NET18 | LoRa RESET | J1.4, U2.D9 |
| NET19 | Toggle feedback | U1.2 (Q1̄), U1.5 (D1) |
| NET20 | Latch output | R2.1, U1.1 (Q1) |
| NET21 | LED anode | DS1.1 (A), R5.1 |

### Unconnected pins

| Pin | Comment |
|---|---|
| J1.1, J1.2, J2.1 (LoRa GND) | **Defect — see HW-007.** Only J2.8 returns the module's ground. |
| J1.6, J1.7, J1.8 (DIO1/2/3), J2.6, J2.7 (DIO5/4) | Acceptable; consider bringing DIO1 out for the Stage 6 pairing work. |
| U2 A0–A7, D0/D1, DTR, RST, second VCC and GND pins | Acceptable, but see HW-035 (floating inputs) and HW-029 (no test/programming interface). |
| U2 RAW | Correct — the on-board regulator is intentionally bypassed. |
| U1.12 (Q2̄), U1.13 (Q2) | Correct — unused flip-flop outputs, with all its inputs tied to VSS. |

---

## 2. PCB facts (from `Hydro-Node.PcbDoc`)

| Property | Value | Note |
|---|---|---|
| Copper extent | ~85.5 × 62.3 mm (board ~90 × 68 mm) | Plenty of free area for the recommended additions |
| Polygons / copper pours | **0** | HW-004 |
| Regions | **0** | |
| Vias | **0** | Nets change layer only through component through-holes |
| Tracks, bottom layer | 227 | |
| Tracks, top layer | 6 | Effectively single-sided routing |
| Track width | **1.0 mm, uniform on every net** | No distinction between power and signal |

---

## 3. Verified correct

Recording these explicitly so they do not get re-litigated later.

- **Reed latch topology.** U1 is correctly wired as a T-flip-flop (D1 ← Q1̄). One rising edge at CLOCK1 produces exactly one toggle. Q1 (pin 1) drives the MOSFET gate through R2 (1 kΩ) with R1 (1 MΩ) as the pulldown. Correct and conventional. Defects are in the debounce and contact protection only — HW-014.
- **Power-on reset.** C2 (100 nF from VBAT) with R4 (100 kΩ to VSS) gives an active-high reset pulse of ~10 ms when a cell is first fitted, so the device powers up **OFF**. Correct polarity for the CD4013's active-high RESET.
- **Unused CD4013 half.** SET1, SET2, RESET2, CLOCK2 and D2 are all tied to VSS; the unused outputs are left open. Correct.
- **OFF-state leakage path.** With Q1 off, R1, R3 and R4 all sit at 0 V across them, and C2/C4 are ceramic. The only OFF-state current is the MOSFET's leakage plus the CD4013's quiescent draw. The latch's OFF state is genuinely clean — this answers Section 6 item 6 in the affirmative.
- **Low-side switching choice.** Switching the load ground rather than the rail means the CD4013 stays powered from the raw battery and the latch state survives, which is what makes the whole scheme work.
- **MCU powered via VCC, not RAW.** Correct — it bypasses the Pro Mini's LDO drop. (The LDO still leaks when back-fed; that is HW-002, a different problem.)
- **DS18B20 powered from a GPIO (D3) with the 4.7 kΩ pull-up referenced to that same pin.** This is the right pattern: driving D3 low in sleep depowers the sensor *and* removes the pull-up's drain, so the whole 1-Wire subsystem costs zero in sleep. Good design decision.
- **The Node performs no interpretation.** Consistent with the Section 2 architecture. Keep it that way — see the recommendation in section 5 below.

---

## 4. Power budget model

Assumptions: 2 × LS14500 = 5.2 Ah nominal, derated 15 % for self-discharge, temperature and cut-off → **4.4 Ah usable**. Two years = 17,520 h → **allowable mean current 251 µA**.

### Per wake (SF7, 16-byte payload, 125 kHz BW)

| Phase | Time | Current | Charge |
|---|---|---|---|
| MCU wake + init | 65 ms | 4 mA | 0.26 mA·s |
| DS18B20 9-bit conversion | 94 ms | 5 mA | 0.47 mA·s |
| Ultrasonic, 5 × 50 ms cycles | 250 ms | 10 mA | 2.50 mA·s |
| LoRa config + PA ramp | 50 ms | 10 mA | 0.50 mA·s |
| LoRa TX | 60 ms | 100 mA | 6.00 mA·s |
| Housekeeping | 80 ms | 4 mA | 0.32 mA·s |
| **Total** | **~600 ms** | | **10.05 mA·s = 2.79 µAh** |

525,600 wakes over 2 years → **1.47 Ah active**, equivalent to an **84 µA** continuous average. That leaves ~167 µA of headroom for sleep current.

### Sleep current, as built vs. after the recommended fixes

| Contributor | As built | After fixes | Reference |
|---|---|---|---|
| Pro Mini power LED | ~2000 µA | 0 (removed) | HW-002 |
| MIC5205 LDO, back-fed | ~50 µA | 0 (removed) | HW-002 |
| ATmega328P power-down + WDT | ~4.5 µA | ~4.5 µA | — |
| ATmega328P BOD, if left enabled | ~20 µA | 0 (fuse) | HW-002 |
| C3 electrolytic leakage @ 25 °C / 60 °C | 5–30 / 50–200 µA | ~0 (ceramic) | HW-009 |
| RCWL-1670 standby | 1.5 µA | 1.5 µA | — |
| Ra-02 sleep | 0.2 µA | 0.2 µA | firmware must command SLEEP, not STANDBY (~1.5 mA) |
| CD4013B quiescent | <1 µA @ 25 °C | <1 µA | verify over temperature |
| DS18B20 subsystem | 0 (GPIO-powered) | 0 | correct as designed |
| Flow input pull-up, if left enabled | ~110 µA | ~3.6 µA (1 MΩ) | HW-020 |
| Floating unused I/O | tens of µA | 0 | HW-035 |
| **Total** | **≈2.1 mA** | **≈10–25 µA** | |
| **Resulting battery life** | **≈88 days** | **≈4.8 years** | |

The 2.4× margin in the right-hand column is what pays for retries, cold-temperature capacity loss, pairing traffic and the things this model has not counted. Do not spend it in advance.

### Spreading factor sensitivity (HW-031)

| SF | Airtime | Charge / TX | Active charge over 2 y | Verdict |
|---|---|---|---|---|
| SF7 | ~51 ms | 5.1 mA·s | ~1.5 Ah | Comfortable |
| SF9 | ~165 ms | 16.5 mA·s | ~3.0 Ah | Marginal (~1.3×) |
| SF10 | ~330 ms | 33 mA·s | ~5.4 Ah | Fails |
| SF12 | ~1.32 s | 132 mA·s | ~19 Ah | Fails by ~4× |

---

## 5. Measurement error budget (NFR-2)

At a 2.0 m measured range. Speed of sound c ≈ 331.3 + 0.606·T m/s, so a 1 °C error in the **path-average** air temperature is a 0.177 % distance error.

| Source | Error at 2 m | Reference |
|---|---|---|
| Headspace thermal gradient, single sensor (±8 °C path-average) | **±28 mm** | HW-023 — dominant |
| MCU timebase, ceramic resonator ±0.5 % | ±10 mm | HW-024 |
| Humidity (saturated vs dry air, uncorrected) | +7 to +12 mm bias | HW-023 note — correctable with a constant |
| Transducer ringdown / detection threshold | ±2–5 mm | measure |
| DS18B20 accuracy ±0.5 °C | ±1.8 mm | — |
| Echo edge timing, `pulseIn` at 8 MHz (±4 µs) | ±0.7 mm | HW-018 |
| MCU timebase, ±30 ppm crystal | ±0.06 mm | — |
| Mounting tilt, 3° | −2.7 mm | mechanical |
| Surface ripple while filling | ±5–20 mm | gate on the flow switch |

**Realistically achievable, as designed:** ±30–50 mm (1.5–2.5 %).
**After two temperature sensors + a crystal + ICP1 capture:** ±10–15 mm (0.5–0.8 %).
**Better than that** requires in-situ speed-of-sound calibration against a reference reflector, which the RCWL-1670 cannot support (HW-023).

Sidewall echoes (HW-030) are not in this table because they are not an error term — they are a wrong reading, and no averaging fixes them.

### Recommendation for the Node/Hub data split

Since the Hub owns all the maths (Section 2), have the Node transmit **raw values only**: echo time in microseconds, the raw DS18B20 reading, the raw flow-switch state, and the raw ADC count from the internal bandgap measurement (HW-025). Then the speed-of-sound formula, the humidity correction, the tank geometry and the volume curve all live on the Hub, where they can be updated over Wi-Fi without ever touching a sealed rooftop device. This is a firmware decision, recorded here now so it is not accidentally designed the other way in Stage 8.

---

## 6. Sources used for datasheet figures

Where a number in the tracker came from a published specification rather than from the design files:

- Ai-Thinker Ra-02 (SX1278): operating voltage 1.8–3.7 V, TX < 120 mA at +20 dBm, RX < 10.8 mA, sleep 0.2 µA. Module silkscreen: ISM 410–525 MHz, PA +18 dBm.
- RCWL-1670: 3–5 V, 6 mA operating, standby 1.5 µA @ 3.3 V, range 2 cm–400 cm, 50 ms measurement cycle, −25 °C to +85 °C, 5–95 % RH. Pinout GND / RX (= TRIG) / TX (= ECHO) / +5V.
- SAFT LS14500: 2.6 Ah, 3.6 V Li-SOCl₂, max recommended continuous current 50 mA, −60 °C to +85 °C.
- Arduino Pro Mini 3.3 V/8 MHz: MIC5205 regulator; ~4.5 µA power-down achievable after removing the power LED and regulator; swapping the MIC5205 for a low-Iq part saves ~50 µA.
- ATmega328P: power-down with WDT ≈ 4.2 µA typ; with WDT and BOD disabled ≈ 0.1 µA. ICP1 = PB0 = Arduino D8.
- CD4013B: 3–18 V, −55 °C to +125 °C.
- HT-60 flow switch: rated AC 220 V, 0.5 A (from the device label).

**Flagged as unverified**, per your rule 3 — please confirm these from the vendor datasheets before we close the related issues:
- The CD4013B's **maximum clock input rise/fall time** (HW-037). I could not retrieve the datasheet table during this review. I believe it is around 15 µs at V_DD = 5 V; the number matters as soon as HW-014's series resistor is added.
- The **IRLZ44N's R_DS(on) and I_DSS at V_GS = 3.6 V and V_DS = 3.6 V** (HW-017). These operating points are not in the datasheet at all, which is itself the finding.
- The **CD4013BE's quiescent current at 3.6 V over the full temperature range** (HW-037).
- **SAFT's specific guidance on paralleling LS14500 cells and on depassivation** (HW-003, HW-032). Do not act on my summary alone for the safety-related item — get it from SAFT.

---

## 7. U1 migration detail — CD4013BE → 74HC74 / SN74HCS74 (HW-041)

Both parts are 14-pin dual D flip-flops. **The pinouts are not compatible and the asynchronous input polarity is inverted.** This is not a socket swap.

### 7.1 Pinout comparison

| Pin | CD4013BE | 74HC74 / SN74HCS74 |
|---|---|---|
| 1 | Q1 | 1CLR (active **LOW**) |
| 2 | Q1 (inverted) | 1D |
| 3 | CLOCK1 | 1CLK |
| 4 | RESET1 (active **HIGH**) | 1PRE (active **LOW**) |
| 5 | D1 | 1Q |
| 6 | SET1 (active **HIGH**) | 1Q (inverted) |
| 7 | VSS | GND |
| 8 | SET2 (active HIGH) | 2Q (inverted) |
| 9 | D2 | 2Q |
| 10 | RESET2 (active HIGH) | 2PRE (active LOW) |
| 11 | CLOCK2 | 2CLK |
| 12 | Q2 (inverted) | 2D |
| 13 | Q2 | 2CLR (active LOW) |
| 14 | VDD | VCC |

Only pins **3, 7, 11 and 14** keep both their number and their connection. Everything else moves.

### 7.2 Net-by-net change list

| Function | CD4013 pin | Current connection | 74HC74 pin | New connection | Changed? |
|---|---|---|---|---|---|
| Supply + | 14 | VBAT | 14 | VBAT | no |
| Supply - | 7 | GND_RAW | 7 | GND_RAW | no |
| Clock in | 3 | reed node (NET04) | 3 | reed node (NET04) | no |
| Unused clock | 11 | GND_RAW | 11 | GND_RAW | no |
| Data in | 5 | from pin 2 | **2** | from pin **6** | **yes** |
| Inverted output | 2 | to pin 5 | **6** | to pin **2** | **yes** |
| Latch output | 1 | R2 (1k) to MOSFET gate | **5** | R2 (1k) to MOSFET gate | **yes** |
| Reset / clear | 4 | POR network, active HIGH | **1** | POR network, **active LOW** | **yes — see 7.3** |
| Set / preset | 6 | GND_RAW | **4** | **VBAT** | **yes — polarity flip** |
| Unused set/preset | 8 | GND_RAW | **10** | **VBAT** | **yes — polarity flip** |
| Unused reset/clear | 10 | GND_RAW | **13** | **VBAT** | **yes — polarity flip** |
| Unused data | 9 | GND_RAW | **12** | GND_RAW | **yes (pin moves)** |
| Unused outputs | 12, 13 | open | 8, 9 | open | pin moves |

The three polarity flips are the ones that will silently break the circuit if missed. On the CD4013 the unused SET/RESET pins are tied **low** to disable them; on the 74HC74 the equivalent PRE/CLR pins must be tied **high**. Tie them to VCC directly — if 1PRE and 1CLR are ever low at the same time the output state is invalid.

**Do not leave any unused 74HC74 input floating.** A floating HC input sits near its threshold and draws crossbar current continuously — tens of microamps, which is a direct hit on the NFR-1 budget. 2D and 2CLK must both be tied to a rail. (The CD4013 version already does this correctly.)

### 7.3 Power-on reset network inversion

The CD4013's RESET is active high; the 74HC74's CLR is active low. The POR network therefore has to invert. **Same two components, same values — swap their positions:**

| | Now (CD4013, active HIGH) | New (74HC74, active LOW) |
|---|---|---|
| C2 (100 nF) | VBAT to pin 4 | pin 1 to GND_RAW |
| R4 (100 kΩ) | pin 4 to GND_RAW | VBAT to pin 1 |
| At power-up | node jumps to VBAT, decays to 0 | node starts at 0, rises to VBAT |
| Time constant | 10 ms | 10 ms |
| Steady state | 0 V, no current | VBAT, no current |
| Result | RESET asserted then released | CLR asserted then released |

Both give the same behaviour: the device powers up **OFF** when a cell is first fitted.

One caveat for the 74HC74 (not the HCS74): the 10 ms RC edge on CLR is extremely slow for a non-Schmitt asynchronous input. It is a once-per-battery-change event with no clock present, so the practical risk is low — but it is another reason to prefer the SN74HCS74, whose CLR input is Schmitt-triggered.

### 7.4 Comparison summary

| Criterion | CD4013BE | 74HC74 | SN74HCS74 |
|---|---|---|---|
| Supply range | 3–18 V | 2–6 V | 2–5.5 V |
| Characterised at 3.0–3.6 V? | **No** (tables are 5/10/15 V only) | Yes (2.0 V and 4.5 V bracket it) | Yes |
| Schmitt-trigger inputs | No | No | **Yes, all inputs** |
| Slow input edge tolerance | Best of the three | **Worst of the three** | No transition-rate requirement at all |
| Separate Schmitt buffer needed (HW-014) | Yes | Yes, mandatory | **No — deleted** |
| Quiescent current spec | Best on paper | Slightly worse | Similar to HC |
| Output drive | ~0.5 mA | ±4 mA | ±4 mA |
| DIP available for bench work | Yes | Yes | **No** (TSSOP/SOIC only) |

**DECIDED (v3): fit the 74HC74** — the Toshiba TC74HC74AP already on the bench. Its 2 V minimum doubles the droop margin over the CD4013BE's 3 V, which under **HW-042** is the real reason for the change. The SN74HCS74 remains the better production part if a non-DIP package is acceptable, but under **HW-043** the latch no longer uses the CLOCK input, so its Schmitt-trigger advantage no longer buys anything here.

Note that §7.1–§7.3 below describe the **toggle** migration (clock-driven). If HW-043 is adopted, use **§8 instead** — it is simpler and supersedes most of this.

**Warning: 74HCT74 will not work.** TTL input thresholds, V_CC = 4.5–5.5 V. One letter apart from the correct part and physically identical. Check the marking.

### 7.5 Bench test to settle it (both parts are on hand)

Run all four steps on each candidate at **3.6 V** from a current-limited bench supply, then repeat at **3.0 V** to simulate end of battery life.

1. **Quiescent current.** IC alone, all unused inputs tied off, no load on Q. Measure I_CC at 25 °C, then soak to ~60 °C with a heat gun and measure again. Expect sub-microamp typical for both; record the actual numbers for the power model.
2. **Toggle correctness.** Wire the full latch. 200 magnet approaches, log Q. Count how many approaches produce anything other than exactly one state change.
3. **The decisive test — slow edge.** Disconnect the reed. Drive CLK from a function generator with a **1 ms linear ramp, 0 → 3.6 V**, 200 cycles. Count output transitions with a scope or a counter. One transition per ramp is a pass; anything more is the edge-rate problem, and this is where the CD4013BE and the 74HC74 should visibly differ.
4. **Same test with a Schmitt buffer in front.** Both parts should now be perfect. If they are, that confirms the buffer is the real fix and the flip-flop family is a secondary choice — which is what HW-041 assumes.

Record the results against HW-041 and HW-014, and I will close them out.

---

## 8. Power-latch circuit — SR variant (WITHDRAWN in v4, kept for history)

> **Withdrawn.** This variant makes OFF firmware-dependent, which contradicts HW-021's requirement that a hung MCU must still be switchable off with a magnet. **Use §9 instead.** Retained because its rail hold-up (§8.1 D1/C_hold) and its numbers are still correct and still required under HW-042.

Supersedes §7.1–§7.3. Built around the 74HC74 already on the bench. Low-side switching is retained — it works and was verified clean in §3.

### 8.1 Circuit

```
                    D1 (BAT54 Schottky)
   VBAT ─────┬────────►|────────┬──────── VLATCH ──── U1 pin 14 (VCC)
             │                  │
             │            C_hold 10 µF X7R
             │                  │
             │                 GND_RAW
             │
             │   R_pu 1 MΩ
   VLATCH ───┴───/\/\/\───┬──── U1 pin 4  (1PRE, active low)
                          │
                          ├──── C_f 100 nF ──── GND_RAW      (reuse C1)
                          │
                          ├──── R_sense 100 kΩ ──── MCU A0   (magnet sense, input)
                          │
                          └──── S1 reed ── R_s 1 kΩ ──── GND_RAW

   VLATCH ───/\/\/\───┬──── U1 pin 1  (1CLR, active low)
             R_por 1 MΩ │
                        ├──── C_por 100 nF ──── GND_RAW      (reuse C2)
                        │
                        └──── R_mcu 100 kΩ ──── MCU A1       (shutdown, normally hi-Z input)

   U1 pin 5  (1Q)  ──── R2 1 kΩ ──── Q1 gate     (R1 1 MΩ gate pulldown to GND_RAW, unchanged)
   U1 pin 3  (1CLK) ─── GND_RAW      unused, must not float
   U1 pin 2  (1D)   ─── GND_RAW      unused, must not float
   U1 pin 12 (2D)   ─── GND_RAW      unused, must not float
   U1 pin 11 (2CLK) ─── GND_RAW      unused, must not float
   U1 pin 10 (2PRE) ─── VLATCH       active low, tie HIGH to disable
   U1 pin 13 (2CLR) ─── VLATCH       active low, tie HIGH to disable
   U1 pin 8, 9      ─── open         unused outputs
   U1 pin 7  (GND)  ─── GND_RAW
```

### 8.2 Operation

| Event | PRE | CLR | Q | Device |
|---|---|---|---|---|
| Battery first fitted, no magnet | H | **L** (C_por discharged, 100 ms) | L | OFF |
| Magnet approaches | **L** | H | H | **ON** |
| Magnet removed | H | H | H (held) | ON |
| Reed bounces during approach | L, H, L, H, L | H | H | ON — bounce is harmless |
| MCU asserts shutdown (reed open) | H | **L** | L | OFF |
| MCU resets or crashes | H | H (pin reverts to hi-Z input) | unchanged | stays ON |
| Fault: PRE and CLR both low | L | L | **H** | stays ON — fails safe |

### 8.3 Why this removes three open issues

- **Bounce (HW-014).** PRE is a level input. Repeated assertions all mean "ON". Schmitt buffer deleted, R3 deleted.
- **Transition rate (HW-037, part of HW-041).** Asynchronous inputs have no transition-rate requirement, so the CD4013B-vs-74HC74 edge-tolerance argument is moot.
- **Reed standby current.** A level input tolerates a 1 MΩ source. Magnet held on the enclosure: **3.6 µA**, down from 360 µA with the old 10 kΩ pull-down.

### 8.4 Numbers

| Quantity | Value | Working |
|---|---|---|
| Latch hold-up sag over a 60 ms TX burst | ~6 mV | ΔV = I·t/C = 1 µA × 0.06 s / 10 µF |
| D1 forward drop at ~1 µA | ~0.15 V | Schottky, low-current region — **verify on the bench** |
| VLATCH at a 3.0 V end-of-life cell | ~2.85 V | vs 74HC74 minimum 2.0 V → 0.85 V margin |
| Reed closed, standby current | 3.6 µA | 3.6 V / 1 MΩ |
| PRE node rise after magnet removal | ~300 ms | 3 × R_pu · C_f = 3 × 1 MΩ × 100 nF |
| PRE node fall on magnet contact | ~300 µs | 3 × R_s · C_f = 3 × 1 kΩ × 100 nF |
| Power-on reset width | ~100 ms | R_por · C_por = 1 MΩ × 100 nF |
| MCU pulling CLR low, divider result | ~0.33 V | 3.6 V × 100 kΩ / (1 MΩ + 100 kΩ), vs V_IL = 0.3 × V_CC = 1.08 V |

The ~300 ms PRE recovery is not a defect — it is a free interlock. Firmware cannot assert CLR until PRE has returned high, which is exactly the required sequencing.

### 8.5 Parts delta from the current board

**Add:** D1 Schottky, C_hold 10 µF X7R, R_pu 1 MΩ, R_s 1 kΩ, R_sense 100 kΩ, R_mcu 100 kΩ.
**Delete:** R3 (10 kΩ), and the Schmitt-trigger buffer that HW-014 would otherwise have required.
**Reuse unchanged:** R1 1 MΩ, R2 1 kΩ, C1 100 nF (now C_f), C2 100 nF (now C_por), Q1, S1.
**Change value:** R4 100 kΩ → 1 MΩ as R_por (or keep 100 kΩ and use 1 µF for C_por).
**Replace:** CD4013BE → TC74HC74AP.
**MCU pins:** A0 (magnet sense, input), A1 (shutdown, hi-Z input except during a commanded shutdown).

### 8.6 Bench validation before this is accepted

1. **Droop measurement (HW-042, mandatory).** Scope VBAT at U1 pin 14 during a real +18 dBm transmission, on cells left idle at least a week so passivation is present, at room temperature and at the coldest expected temperature. Record the minimum. Below the fitted logic's minimum supply, HW-042 escalates to BLOCKER.
2. **Turn-on reliability.** 200 magnet approaches through the production enclosure wall at the production standoff. Every one must turn the device on. Zero failures required.
3. **Accidental turn-off.** 200 magnet approaches with the device already on. It must remain on every time. Zero turn-offs required.
4. **Commanded turn-off.** 50 full gestures. Each must shut down cleanly, and releasing the magnet early must do nothing.
5. **Reset immunity.** Pull the MCU's RESET line low while the device is on. The latch must hold and the device must come back up.
6. **Standby current.** Measure with the device off, and again with a magnet held on the reed. Expect sub-µA and ~3.6 µA respectively.

---

## 9. Recommended power-latch circuit — toggle with proper debounce (HW-041 + HW-042 + HW-043)

**This is the current recommendation.** It is the existing design, corrected. The magnet retains full control of both on and off, with no firmware involvement. Low-side switching is retained.

### 9.1 Circuit

```
                    D1 (BAT54 Schottky)                       ── HW-042 ──
   VBAT ─────┬────────►|────────┬──────── VLATCH ──┬── U1 pin 14 (VCC, 74HC74)
             │                  │                  │
             │            C_hold 10 µF X7R         └── U2 pin 5 (Schmitt buffer VCC)
             │                  │
             │                 GND_RAW
             │
             │   R_pu 1 MΩ                                    ── HW-043 ──
   VLATCH ───┴───/\/\/\───┬──── U2 in   (74LVC1G14, INVERTING Schmitt)
                           │
                           ├──── C_f 1 µF ──── GND_RAW
                           │
                           └──── S1 reed ── R_s 1 kΩ ──── GND_RAW

   U2 out ──┬──────────────────── U1 pin 3  (1CLK)   fast edge, always
            └──── R_sn 100 Ω ──── MCU A0             magnet sense, HIGH = magnet present

   U1 pin 2  (1D)   ──── U1 pin 6 (1Q)     toggle feedback: D <- Q
   U1 pin 5  (1Q)   ──── R2 1 kΩ ──── Q1 gate        (R1 1 MΩ gate pulldown, unchanged)
   U1 pin 1  (1CLR) ──── power-on reset, active LOW  (see §7.3 — swap C2 and R4)
   U1 pin 4  (1PRE) ──── VLATCH            active low, tie HIGH to disable
   U1 pin 10 (2PRE) ──── VLATCH            active low, tie HIGH to disable
   U1 pin 13 (2CLR) ──── VLATCH            active low, tie HIGH to disable
   U1 pin 11 (2CLK) ──── GND_RAW           unused, must not float
   U1 pin 12 (2D)   ──── GND_RAW           unused, must not float
   U1 pin 8, 9      ──── open              unused outputs
   U1 pin 7  (GND)  ──── GND_RAW
```

Optional, for **automatic low-battery shutdown only** (HW-025) — never the user-facing off path: MCU A1 → 100 kΩ → U1 pin 1 (1CLR), normally held as a hi-Z input so a reset or crash can never assert it.

### 9.2 Why the buffer must be INVERTING

The reed now pulls the node **low** on magnet approach (that inversion is what allows the 1 MΩ pull-up and the 100× standby-current saving). The 74HC74 clocks on a **rising** edge. An inverting buffer therefore restores "toggle when the magnet arrives" rather than "toggle when the magnet leaves".

| Event | Reed node | U2 out (inverting) | CLK edge | Result |
|---|---|---|---|---|
| Magnet approaches | falls to ~4 mV in ~3 ms | rises to VLATCH | **rising** | **toggle** |
| Contact bounces during approach | stays low (1 s release τ) | stays high | none | ignored |
| Magnet removed | rises over ~3 s | falls | falling | ignored |

### 9.3 Numbers

| Quantity | Value | Working |
|---|---|---|
| Attack time constant | 1 ms | R_s · C_f = 1 kΩ × 1 µF |
| Release time constant | 1 s | R_pu · C_f = 1 MΩ × 1 µF |
| Minimum separation for two distinct toggles | ~3 s | 3 × release τ — a visible double-tap, not bounce |
| Reed node level, magnet present | ~3.6 mV | 3.6 V × 1 kΩ / (1 MΩ + 1 kΩ) |
| Standby current, magnet resting on enclosure | **3.6 µA** | 3.6 V / 1 MΩ — was 360 µA with the old 10 kΩ |
| Standby current, magnet absent | 0 | reed open, no path |
| Latch hold-up sag over a 60 ms TX burst | ~6 mV | ΔV = I·t/C = 1 µA × 0.06 s / 10 µF |
| VLATCH at a 3.0 V end-of-life cell | ~2.85 V | vs 74HC74 minimum 2.0 V → 0.85 V margin |

### 9.4 Parts delta from the current board

**Add:** D1 Schottky, C_hold 10 µF X7R, U2 inverting Schmitt buffer, R_pu 1 MΩ, R_s 1 kΩ, R_sn 100 Ω. C_f: change C1 from 100 nF to 1 µF.
**Delete:** R3 (10 kΩ).
**Reuse unchanged:** R1 1 MΩ, R2 1 kΩ, C2 100 nF, Q1, S1.
**Rework:** swap C2 and R4 positions for the active-low CLR power-on reset (§7.3).
**Replace:** CD4013BE → TC74HC74AP (HW-041).
**Reverse:** the reed now switches to GND, not to VBAT.
**MCU pins:** A0 (magnet sense, input). A1 optional, low-battery shutdown only.

### 9.5 Buffer selection

Select U2 on quiescent current, not on speed. 74LVC1G14 covers 1.65–5.5 V and is typically well under a microamp, but its datasheet maximum is around 10 µA over temperature — 4 % of the 251 µA budget. Compare against an ultra-low-power alternative such as the 74AUP1G14 (note its narrower supply range, which is tight against a fresh 3.67 V cell). **Confirm I_CC at 3.6 V and at 60 °C from the vendor datasheet before committing.**

### 9.6 Rejected alternative — hardware-timed off

Tap = ON via PRE, hold ~3 s = OFF via an RC timer into CLR. Keeps both directions firmware-independent *and* makes bounce structurally impossible rather than filtered. It works, and the timing is straightforward (a ~1 MΩ / 4.7 µF network into a second Schmitt threshold, with a steering diode for fast recovery on magnet removal).

**Not recommended.** It costs roughly five extra parts plus a tuning exercise, needs a low-leakage timing capacitor and a Schmitt input whose leakage is small against a sub-microamp timing current, for a control used about five times in the product's life. §9 achieves the same practical outcome with two parts. Revisit only if field data shows spurious toggling.

### 9.7 Bench validation

1. **Droop measurement (HW-042, mandatory).** Scope VBAT at U1 pin 14 during a real +18 dBm transmission, on cells left idle at least a week so passivation is present, at room temperature and at the coldest expected temperature. Record the minimum. Below the fitted logic's minimum supply, HW-042 escalates to BLOCKER.
2. **Toggle determinism.** 200 magnet approaches through the production enclosure wall at the production standoff, alternating on and off. Every approach must produce exactly one state change. Zero double-toggles required — this is the test the present 1 ms filter fails.
3. **Reset immunity.** Pull the MCU's RESET line low while the device is on. The latch must hold and the device must come back up.
4. **Standby current.** Measure with the device off, and again with a magnet held on the reed. Expect sub-µA and ~3.6 µA respectively.
5. **Indicator legibility (HW-016, HW-044).** Confirm the startup blink is clearly visible through the production enclosure wall, in daylight, at **3.0 V** — not just at 3.6 V.

---

## 10. Battery configuration analysis (HW-003, HW-009, HW-042)

Constraint set as of v6: the only primary cells available are **LS14500 (2.6 Ah)** and **LS14250 (1.2 Ah)**. Rechargeables of any form factor are available. LS26500 and larger are not sourceable.

### 10.1 The two problems are separate

They get conflated, and the confusion is why "add a big capacitor" keeps looking attractive:

| Problem | What it is | What fixes it |
|---|---|---|
| **Energy** | ~1.8 Ah must come out of the pack over 2 years | More Ah — a second cell, or less consumption |
| **Peak current** | The Ra-02 wants 120 mA; an LS14500 is rated 50 mA continuous | A second cell, a supercapacitor, or lower TX power |

Paralleling two cells happens to solve **both**, which is why the original instinct was sound. Only the *direct* connection is unsafe — the parallel topology itself is fine once the cells are isolated from each other.

### 10.2 Energy budget vs configuration

Non-TX portion of each wake is 4.05 mA·s (MCU wake 0.26, DS18B20 9-bit 0.47, five ultrasonic cycles 2.50, LoRa config/ramp 0.50, housekeeping 0.32). TX adds 60 ms at the PA current. 525,600 wakes over 2 years at a 120 s interval. Sleep assumed 20 µA = 0.35 Ah. Usable capacity derated 15 % from nameplate for self-discharge, cut-off and pulse-load losses.

| TX power | PA current | Per wake | 2-yr active | 2-yr total |
|---|---|---|---|---|
| +20 dBm | 120 mA | 11.25 mA·s | 1.64 Ah | **1.99 Ah** |
| +18 dBm | ~100 mA | 10.05 mA·s | 1.47 Ah | **1.82 Ah** |
| +14 dBm | ~45 mA | 6.75 mA·s | 0.99 Ah | **1.34 Ah** |
| +10 dBm | ~30 mA | 5.85 mA·s | 0.85 Ah | **1.20 Ah** |

| Configuration | Usable | Margin @ +18 dBm | Margin @ +14 dBm |
|---|---|---|---|
| 1 × LS14500 | 2.2 Ah | **1.21×** | 1.65× |
| 2 × LS14500 | 4.4 Ah | **2.42×** | **3.29×** |
| 1 × LS14250 | 1.0 Ah | 0.55× — fails | 0.75× — fails |

A single LS14500 at 1.21× is not a production margin for a 2-year claim. **Two cells, or one cell plus a TX power cut and an accepted 1.65×.**

PA current figures other than +20 dBm are interpolated from the SX1276/78 family datasheet anchors (120 mA at +20 dBm on PA_BOOST, ~87 mA at +17 dBm, ~29 mA at +13 dBm on RFO). **Measure the actual Ra-02 current at your chosen power setting before locking the budget.**

### 10.3 Why a bulk capacitor cannot carry the burst

One TX burst at SF7 moves **Q = I·t = 0.120 A × 0.060 s = 7.2 mC**.

| Capacitor | Resulting sag ΔV = Q/C |
|---|---|
| 470 µF | 15.3 V |
| 1000 µF | 7.2 V |
| 2200 µF | **3.3 V** — fully collapsed |
| Required for ΔV ≤ 0.2 V | **36,000 µF** |
| 0.1 F supercapacitor | 0.072 V |

Even the softer framing — "let the cell supply its rated 50 mA and the cap covers the excess 70 mA" — needs 4.2 mC, which is still a **1.9 V** sag into 2200 µF. A bulk capacitor of any size that fits this enclosure is off by more than an order of magnitude. This is also why the fitted C3 does not do what it appears to (HW-009); its only real function is smoothing the first few milliseconds of the current edge.

**A supercapacitor does work**, in this topology:

```
   cell ──/\/\/\── ● ──┬── supercap 0.1 F, ESR <= 1 ohm
          R 100 Ω     │
                      └── load (Ra-02 etc.)
```

- During TX the load draws from the supercap directly — R is not in that path. Sag ≈ 72 mV.
- R limits the cell to 3.6 V / 100 Ω = **36 mA** worst case, inside the 50 mA rating even with an empty supercap at first power-up.
- Recharge after a burst: ~0.7 mA through R, restoring 7.2 mC in about 10 s — comfortably inside a 120 s cycle.
- Quiescent current through R costs 2 mV at 20 µA. Negligible.

Costs: supercap leakage is typically 1–10 µA continuous and must be budgeted; ESR above ~1 Ω defeats the purpose (10 Ω would drop 0.7 V at 70 mA); and **most small supercaps are rated 2.7 V, below the 3.6 V rail** — you need a 3.8 V or 5.5 V part, and the 5.5 V ones are two cells in series, halving capacitance and doubling ESR.

This solves peak current but **not energy** — see §10.2. It does not rescue a single-cell design.

### 10.4 Worst-case rail with per-cell isolation

At +18 dBm, 100 mA total, each cell and each diode carries ~50 mA. End-of-life cell at 3.2 V with elevated internal impedance from passivation (~3 Ω per cell → 0.15 V sag).

| Isolation method | Drop at 50 mA | Rail at end of life | Verdict |
|---|---|---|---|
| Plain Schottky (BAT54 class) | ~0.32 V | **2.73 V** | Works, but only 0.33 V over the MCU floor |
| Low-Vf Schottky (PMEG class) | ~0.22 V | 2.83 V | Better |
| Ideal-diode controller | 0.01–0.03 V | **3.03 V** | Preferred |

Load minimums to clear: ATmega328P at 8 MHz **2.4 V**; Ra-02 **1.8 V**; 74HC74 **2.0 V** (and it sits behind its own hold-up diode from HW-042, so it sees another ~0.15 V less).

The ATmega is the binding constraint. Cutting TX power to +14 dBm halves the diode current and moves every row in this table up by roughly 0.1 V, on top of the energy saving.

### 10.5 Rechargeable options — assessed, not recommended without a charging source

| Chemistry | Voltage | Blocker for this design |
|---|---|---|
| Li-ion / LiPo | 3.0–4.2 V | **4.2 V exceeds the Ra-02's 3.7 V maximum** — needs a regulator. Self-discharge ~2–3 %/month loses over half the pack across 2 years with nothing charging it. |
| LiFePO₄ | 2.5–3.65 V | Voltage range is nearly a drop-in for the 3.6 V rail, and it handles amps without complaint. But self-discharge ~3 %/month still loses most of the pack over 2 years unpowered. |

**The disqualifier for every rechargeable here is self-discharge over an unattended 2-year life**, not the chemistry. Secondary problem: a lithium-ion pack sealed in a rooftop enclosure that reaches 70–80 °C (HW-027) is a worse safety proposition than the one HW-003 is trying to avoid. LiFePO₄ tolerates heat far better but is still typically rated to ~60 °C.

**Solar + LiFePO₄ is a genuinely credible alternative** and deserves a real look, because this device sits in direct sunlight by definition. A ~1 W panel covers a 100 µA average many times over, and it would delete HW-003 entirely along with HW-032 (passivation) and most of HW-042 (droop) — LiFePO₄ has milliohm-class impedance and does not sag under 120 mA.

Its own scope, if you want it costed: panel and mounting, a charge controller with µA-class quiescent, a **sub-zero charge lockout** (LiFePO₄ must not be charged below 0 °C), another enclosure penetration, and a panel-soiling/shading maintenance story.

### 10.6 Recommendation

**Two LS14500 in parallel with per-cell isolation, plus a TX power cut.**

1. Ideal-diode controller per cell (low-Vf Schottky as the fallback), plus a PTC or fuse in the pack lead.
2. Drop TX power from +18 dBm. It is the single largest lever available: it nearly halves the dominant energy term, halves the peak current so the cells sit inside their rating, reduces the HW-042 droop, and is very likely forced by **HW-006** anyway.

That gives a **3.3× margin** on the 2-year target at +14 dBm, which is the kind of headroom a production claim needs.
