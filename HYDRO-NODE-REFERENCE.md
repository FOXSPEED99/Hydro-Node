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
