# HYDRO NODE — HARDWARE ISSUE TRACKER
Version: v26   |   Last updated: 2026-08-26   |   Status: **Stage 1 bring-up.** Sleep current measured at 100 µA against a 25 µA budget — margin 2.43× → 1.41×; the measurement is not yet valid and the attribution is owed

## STATUS SUMMARY
Total issues: 71   |   Open: 46   |   Resolved: 23   |   Won't fix: 2
Blockers remaining: 1
Production-ready: NO — the two Li-SOCl₂ cells are hard-paralleled without blocking diodes. That is the only blocker.
Schematic: `Hydro Node Schematic.SchDoc` — all 34 build-sheet connections verified. See `SCHEMATIC-CHECK.md`.
PCB: `Hydro_Node_PCB.PcbDoc`, 90 × 70 mm — every net connected, clearance 0.351 mm, ground pour over the whole board, all footprints correct. See `PCB-CHECK.md` and `PCB-FIXES.md`.
Bring-up: **the board switches on and the magnet behaves.** HW-067 fixed by removing R13; **HW-069's R14 → 2.2 MΩ is fitted and tested good**. First sleep-current reading is **100 µA** — see **HW-070** for what that costs and how to attribute it, **HW-071** for the ultrasonic sitting on the permanently-powered rail, and **HW-046** for the D13 LED, which can draw 40 µA while looking off. Rework list in `WORKSHOP-TODAY.md`; five Stage-17 measurements still owed.

---

## OPEN ISSUES

### HW-003 — Two LS14500 Li-SOCl₂ cells hard-paralleled with no blocking diodes
- Severity: BLOCKER
- Status: OPEN — decision made in v14, pending implementation
- Component / net: Battery connector, VBAT / GND_RAW
- Problem: The two cells are wired directly in parallel. Lithium-thionyl-chloride primary cells must never be charged, and two directly-paralleled cells will push current into one another as they diverge. Cell manufacturers require a **series blocking diode per cell** whenever primary lithium cells are paralleled.
- Impact: Forcing current into a Li-SOCl₂ cell grows **lithium dendrites** on the anode — there is no reversible plating mechanism in this chemistry. A dendrite bridging to the cathode is an internal short and thermal runaway. The cell contains thionyl chloride; venting releases SO₂ and HCl, and the electrolyte reacts violently with water. This is a sealed enclosure on the roof of an occupied building, heading for a production line.
- Your argument (v5): *"Both cells measured ~3.65 V when we bought them. If they are at the same voltage, no current flows between them, so where is the danger? Nothing bad has happened."* That is correct at the moment of assembly and it is why nothing has happened yet. It does not hold over the life of the pack, for four reasons:
  1. **They do not stay matched.** No two cells discharge identically. Internal impedance spread, a temperature gradient across the holder (one cell nearer the sun-facing wall runs hotter), differing passivation film thickness and differing self-discharge all mean one cell delivers more current than the other. The divergence compounds over 24 months.
  2. **The exposure is after each pulse, not at rest.** During a 120 mA TX burst the lower-impedance cell supplies most of the current and therefore sags further. The instant the pulse ends, the two cells sit at different voltages with **zero resistance between them**, and current flows from the higher into the lower. That is a charging current. It is small early in life and grows as they diverge.
  3. **End of life is the dangerous window.** Li-SOCl₂ has a famously flat discharge curve that falls off a cliff at the end. When one cell reaches its knee and the other has not, the healthy cell drives the **full voltage difference** into a nearly-exhausted cell through essentially zero resistance, continuously. This is exactly the condition the manufacturer warnings are written about, and it happens at 18–24 months — the end of your target life.
  4. **"Nothing bad happened" is the expected observation right now.** The pack is new and matched; the failure mode is an aging phenomenon. Weeks of bench time carries no information about month 20. This is the same age-correlation that makes HW-042 dangerous.
- **DECISION (v14): two LS14500 in parallel, isolated, supplied as a sealed pack. Keep the 2-year target.** The v13 single-cell recommendation is withdrawn — the 50 m / thick-concrete link distance, given in v14, makes the radio settings uncertain enough that a one-cell design is not safe to commit to.

  | Item | Part | Purpose |
  |---|---|---|
  | Cells | 2 × LS14500 | 4.4 Ah usable; ~50 mA per cell at +18 dBm, inside the 50 mA rating |
  | Isolation | 2 × **`1N5819`** (40 V, 1 A, DO-41), one in each cell's **+** leg | Blocks inter-cell charging. ~0.25 V at 50 mA, ~0.1 V at sleep currents |
  | Protection | 1 × **0.5 A fast-blow fuse** in the pack lead (`0451.500` SMD or 5×20 mm glass) | **Not a PTC** — an MF-R025's ~1 Ω costs 120 mV at 120 mA, the same headroom as a diode, for no benefit |
  | Assembly | **Sealed, non-serviceable pack, single JST connector** | Makes single-cell replacement physically impossible — that is the realistic hazard, not slow aging divergence |

  Added cost: roughly five cents of diodes plus a fuse.
- Why two cells and not one — what the link distance did to the analysis: at 50 m through thick concrete the required spreading factor is genuinely uncertain, and SF drives energy far harder than TX power does. Two-year margins:

  | | ONE cell (2.2 Ah) | | | TWO cells (4.4 Ah) | | |
  |---|---|---|---|---|---|---|
  | | +14 dBm | +18 dBm | +20 dBm | +14 dBm | +18 dBm | +20 dBm |
  | SF7 | 1.57× | 1.21× | 1.12× | 3.14× | 2.43× | 2.24× |
  | SF9 | **1.02×** | FAILS | FAILS | **2.04×** | 1.27× | 1.11× |
  | SF10 | FAILS | FAILS | FAILS | 1.36× | FAILS | FAILS |

  One cell survives only if the link turns out easy. Two cells cover every plausible outcome. **And two cells is the reversible choice**: if Stage 5 measures an easy link you simply fit one cell and ship, with no board or mechanical change — whereas committing to one cell and then discovering you need SF9 means a respin. Design for the case you cannot yet measure.
- Bench verification before this is closed:
  1. **Measure the actual `1N5819` forward drop at 50 mA** on your parts. I have assumed 0.25 V, and the worst-case rail figures in `HYDRO-NODE-REFERENCE.md` §10.4 and the HW-042 margin both depend on it.
  2. Confirm the Ra-02 still starts and transmits cleanly with the diodes fitted, on cells left idle a week (passivation, HW-032).
  3. Confirm the sealed pack assembly cannot be opened to replace a single cell without obvious destruction.
- Notes: This issue moves to RESOLVED once the diodes and fuse are fitted and the pack is built as a sealed unit. The severity stays BLOCKER until then, because the hazard exists on the current hardware.
- Notes: The risk reframing from v13 stands and is why the sealed pack matters as much as the diodes: with matched cells discharged together in one enclosure, aging divergence is modest; the realistic hazard is a user putting a fresh 3.67 V cell across a depleted one.

---


### HW-070 — Measured sleep current is 100 µA against a 25 µA budget
- Severity: MAJOR
- Status: OPEN — measured on the bench 2026-08-26
- Component / net: whole node, sleeping, magnet away, reed at rest
- Measurement: **~0.10 mA (100 µA)** on the built board, after R13 was removed (HW-067) and R14 raised to 2.2 MΩ (HW-069).
- Problem: `BUILD-SHEET.md` budgets **25 µA** for sleep. The measured figure is **4× that**, and sleep is the term that dominates a two-year life because it runs for all 17,520 hours while the active work runs for minutes.
- Impact on the two-year target, against the 4400 mAh pack and the 1373 mAh of active energy already budgeted:

  | Sleep current | Sleep energy over 2 years | Total | Margin on 4400 mAh |
  |---|---|---|---|
  | 25 µA (target) | 438 mAh | 1811 mAh | **2.43×** |
  | 50 µA | 876 mAh | 2249 mAh | 1.96× |
  | 75 µA | 1314 mAh | 2687 mAh | 1.64× |
  | **100 µA (measured)** | **1752 mAh** | **3125 mAh** | **1.41×** |
  | 150 µA | 2628 mAh | 4001 mAh | 1.10× |

  So 100 µA **does not fail the two-year target** — it eats the margin. 1.41× has to absorb Li-SOCl₂ capacity loss at rooftop temperature, passivation (HW-032), the diode drops of HW-003, and whatever spreading factor HW-047's link measurement forces. That is not enough headroom to ship against.
- **The measurement is not yet valid, for two reasons that must be cleared first:**
  1. **A0 is a bare floating input** (HW-035, v25 update). R13 was removed and A0 went nowhere else. A floating CMOS input oscillates around its threshold and its input stage draws crossbar current continuously. Nothing measured on this board means anything until every unused pin has a defined state in `setup()`.
  2. **Were the three sensor cables plugged in?** See HW-071 — J3.2 puts the ultrasonic module on the switched rail permanently, so if J3 was connected the module was fully powered for the whole measurement. A reading of 0.10 mA strongly implies J3 was **unplugged**, which means the real sleep current of the assembled product is higher than this number, not equal to it.
- Ranked suspects, with the magnitude each would contribute:

  | Suspect | Order of magnitude | How to clear it |
  |---|---|---|
  | **Brown-out detector left enabled in power-down** | **~20 µA** | `sleep_bod_disable()` immediately before `sleep_cpu()`, or clear the BODLEVEL fuse |
  | **D13 LED lit by an `INPUT_PULLUP`** — see HW-046 | **~40 µA** | Leave D13 an output driven **low** after `SPI.end()`, or desolder the LED |
  | ADC left enabled | 200–300 µA | `ADCSRA = 0;` before sleeping. At 100 µA total it is probably already off — confirm anyway |
  | Analog comparator left enabled | tens of µA | `ACSR \|= (1 << ACD);` |
  | Floating inputs (A0 now, plus A3–A7, D0, D1) | tens of µA, unpredictable | `INPUT_PULLUP` or output-low on every one |
  | Watchdog timer | ~5 µA | **Required** — this is the 2-minute wake source. Budget it, do not remove it |
  | Two DS18B20 in standby on BATT+ | ~1.5 µA | Accepted in v17 when HW-053 was closed |
  | 74HC74 quiescent | <1 µA typ, 8 µA package max at 25 °C | Nothing to do |
  | C9 electrolytic leakage | <1 µA at 3.5 V on a 50 V part | Measure before swapping — see HW-058 |
  | R6, R7, R9, R11, R14 | 0 µA at rest | All five sit with both ends at the same potential when idle |
- Meter caveat: **0.10 mA on a milliamp range is one count.** The resolution is ~10 µA and the shunt's burden voltage at that range can drop enough to change the circuit's own behaviour. Re-read it on a **microamp range**, and note that a µA range's burden voltage is *worse* — if the rail sags the reading is not the sleeping current either. The clean method is a 10 Ω shunt in the BATT− lead with a millivolt reading across it, or a purpose-built low-burden meter.
- Method to attribute the current rather than guess at it — subtract one thing at a time and record each number:
  1. Sensors unplugged, firmware as-is → baseline.
  2. Add `ADCSRA = 0` and the analog-comparator disable → Δ.
  3. Give every unused pin a defined state, A0 first → Δ.
  4. Add `sleep_bod_disable()` → Δ. Expect the biggest single step here.
  5. Drive D13 low (or lift the LED) → Δ.
  6. Plug J2 back in → Δ (expect ~1.5 µA).
  7. Plug J3 back in → Δ (expect **milliamps** — that is HW-071, not a firmware problem).
- Notes: this issue stays open until the attributed measurement exists. A single number with no breakdown cannot be designed against, and every one of the steps above is free.

---

### HW-071 — The ultrasonic module is permanently powered whenever the node is on
- Severity: MAJOR
- Status: OPEN — raised 2026-08-26 out of the HW-070 sleep-current measurement
- Component / net: `J3.2` → **BATT+**, `J3.1` → **GND** (switched); RCWL-1670
- Problem: the low-side MOSFET switches the whole board's ground, so "off" is genuinely off — but between wakes the node is **on**, merely asleep. J3.2 sits on BATT+ and J3.1 on the switched ground, so the ultrasonic module has **full power for every one of the 120 seconds between readings**, and for the 2-year life of the product. Nothing in the design turns it off.
- Impact: a ranging module carries its own microcontroller and a receiver amplifier chain. Modules in this class draw **milliamps** at idle, not microamps. If that is true of the RCWL-1670 then the sleep term is not 25 µA or 100 µA — it is thousands of microamps, and the two-year target fails outright by more than an order of magnitude. This single connection can be larger than every other item in the power budget combined.
- **Unverified and must be measured, not looked up:** I cannot reach a datasheet for the RCWL-1670 from this environment (every datasheet host is blocked by the egress proxy), and low-cost module quiescent figures are unreliable even when published. **Measure it:** power the module alone off a bench supply at 3.6 V, leave it idle with no trigger, and read the current. That number decides the severity of this issue and possibly the architecture.
- Fix options, in order of preference:
  1. **High-side switch the module's supply from a GPIO** — a P-channel MOSFET (or a load switch) in the J3.2 feed, gate driven from a spare pin. **D3 is free** since HW-053 was closed by tying J2.3 to BATT+, so the pin exists. This is the correct answer and it is one transistor and two resistors.
  2. Do **not** drive J3.2 straight off a GPIO. The module's inrush and its operating current are well beyond a 40 mA pin.
  3. If the measured idle current turns out to be genuinely microamps, close this issue with the measurement recorded — but the measurement has to exist first.
- Related: the same question applies to the flow switch, and there the answer is already good — J1 draws current only while the contacts are closed, through R6's 1 MΩ, i.e. 3.6 µA while water is actually flowing.
- Notes: this was missed at the schematic stage because HW-053 asked the right question about the **temperature probes** (should J2.3 be switched?) and it was answered on the numbers — 1.5 µA of DS18B20 standby is not worth a transistor. **The same question was never asked about J3**, where the answer is almost certainly the opposite. The lesson is that "which loads are on the always-powered rail, and what does each of them draw at idle" is a checklist item, not a per-part judgement call.

---

### HW-058 — C7, C8 and C9 are aluminium electrolytics on a two-year rooftop product
- Severity: MAJOR
- Status: OPEN — found in the 2026-08-22 schematic check
- Component / net: C7 100 µF (radio bulk), C8 10 µF (radio bulk), C9 10 µF (latch hold-up). All three use footprint `WCAP-ATLL_D5H11`
- Problem: all three carry the same parameters in the schematic — **aluminium, radial 5 × 11 mm, 105 °C, 4000 hours**, leakage **10 µA** for C7 and **5 µA** each for C8 and C9. Aluminium electrolytics are the one capacitor family that *wears out*: the electrolyte evaporates, and the rated life is quoted at the rated temperature.
- Impact 1 — **wear-out inside the target life.** The standard rule is that life doubles for every 10 °C below the rating. **HW-027** puts the internal enclosure temperature at **70–85 °C** on a Syrian roof in summer:

  | Internal temperature | Life from 4000 h at 105 °C |
  |---|---|
  | 75 °C | ~32,000 h ≈ **3.6 years** |
  | 85 °C | ~16,000 h ≈ **1.8 years** |

  At the top of that range the capacitors are worn out **before the two-year battery target is reached**. As they dry out their ESR rises, which is exactly the property the radio's transmit burst depends on — so the failure mode is not "no capacitance", it is a supply that sags further every month until the latch drops out. That is **HW-042** coming back through a different door.
- Impact 2 — **C9 leaks on the always-live rail.** C9 sits between the latch rail (N04) and BATT-. Both are powered whether the device is on or off, so its leakage is a **24/7 drain that the magnet cannot switch off**. The part's own parameter says up to 5 µA; the sleep-current target in the build sheet is 25 µA total. At 3.6 V on a 50 V part the real figure will be far below the specified maximum — but leakage rises steeply with temperature, and nothing guarantees it on a hot roof.
- Fix:
  - **C9 → ceramic 10 µF 16 V X7R (1206) or tantalum.** This is the one that matters most: it is on the rail that never switches off, and it is the rail that holds the latch up through a transmit burst.
  - **C7 → tantalum 100 µF, or ceramic 1210 rated 16 V or 25 V.** Ceramic at 6.3 V loses 50–70 % of its value at 3.6 V through DC bias derating; tantalum does not derate and is what `BUILD-SHEET.md` already specifies.
  - **C8 → ceramic 10 µF 16 V X7R (1206).**
  - Neither ceramic nor tantalum dries out, and both have leakage orders of magnitude below an electrolytic.
- Notes: the build sheet already called for a tantalum 100 µF for the DC-bias reason. This issue is the second, independent reason to move off aluminium, and it applies to all three parts rather than just the 100 µF. Surface-mount 1206/1210 is already accepted for this build.

---

### HW-059 — S1's part number is a surface-mount reed; its footprint is through-hole
- Severity: MAJOR
- Status: OPEN — found in the 2026-08-22 schematic check
- Component / net: S1, `Manufacturer Part Number = MDSM-4R-12-18`, footprint `REEDSW-THT-D4L29-P35`
- Problem: the two do not describe the same part. **MDSM-4R-12-18 is a Littelfuse surface-mount reed**, SPST-NO, glass envelope **15.24 × 2.28 mm**, 12–18 AT sensitivity. The footprint name says through-hole, 29 mm long, 4 mm diameter. The reed actually on the bench — `Components Images/F2293658-01.jpg`, and the one `BUILD-SHEET.md` stage 7 describes bending the legs of — is an axial through-hole glass reed with long wire leads.
- Impact: the BOM is generated from the schematic. Order this line as written and an SMD part arrives that does not fit the board. This is the fifth documentation-versus-hardware mismatch on the project (**HW-033**) and the second to be found in a library part rather than a document.
- Fix: decide which part is real, then make both fields agree.
  1. Measure the reed you are actually fitting — glass length, overall length, lead diameter.
  2. Set the MPN to that part.
  3. Open `REEDSW-THT-D4L29-P35` and check the pad pitch against the measurement. `D4L29-P35` reads as a 4 mm diameter, 29 mm body, 3.5 mm pitch — confirm which of those is the lead spacing, because a reed's leads are usually formed to whatever pitch you choose.
- Notes: do this at the same time as **HW-057**, since both are the same symbol needing a clean-up. Also relevant to **HW-039** — a bare glass body with long unsupported leads needs its lead-forming and strain relief specified, and that depends on the same measurement.

---


### HW-067 — The half-powered Pro Mini holds the latch clock at 2.4 V, so the magnet can never switch the device on
- Severity: **BLOCKER** *(raised from MAJOR — confirmed on hardware)*
- Status: OPEN — **CONFIRMED by measurement on the first built board, 2026-08-24**
- Component / net: R9 100 kΩ from `U1.A1` to `U2.1 (1~RD)`; R11 1 MΩ from the latch rail to the same pin
- Problem: `U2.1` is the flip-flop's **active-low reset**. It is held high by **R11, 1 MΩ**. The Pro Mini's **A1** reaches the same pin through **R9, 100 kΩ**. R9 is **ten times stronger than R11**, so whenever A1 sits low the reset is held asserted, Q stays at 0, the gate stays at 0 V and the MOSFET can never turn on — no matter how many times the magnet is applied.
- Why this is not just a firmware question: when the device is **off**, the Pro Mini still has BATT+ on its VCC while its ground floats, so it sits half-powered in an undefined state with its pins in an undefined condition. The design assumes A1 is high-impedance in that state. Nothing on the board enforces it.
- Impact: a device that can never be switched on, with every connection on the board correct. This is a candidate cause of the 2026-08-24 bring-up failure and it is being tested by lifting one leg of R9 — see `BRINGUP-DEBUG.md`.
- Fix, in order of preference:
  1. **Firmware:** A1 stays an **input with the pull-up disabled** at all times, and is driven low only for the few milliseconds that command a shutdown, then returned to an input. This has to be written into the firmware specification, not left to be remembered.
  2. **Hardware, if a respin happens:** raise R9 to **1 MΩ** so it can no longer overpower R11, or drop R11 to 100 kΩ so the two are matched and A1 can still win when it is genuinely driven. Raising R9 is the safer of the two — it costs nothing and removes the failure mode rather than balancing it.
  3. A series diode from A1 into R9, so the MCU can only ever pull the pin **down** deliberately, is available if the firmware route proves unreliable.
- Notes: also check **R13**, the 100 Ω from `U1.A0` to `U2.3`, the clock. That one is 100 Ω against R14's 470 kΩ pull-down — nearly 5000 times stronger — so a stuck-low A0 clamps the clock line and the reed can never produce an edge. The same firmware rule covers both: **A0 and A1 are inputs unless deliberately driving.**
- Cross-reference: **HW-021** put both pins there on purpose and that decision stands. This issue is about the resistor ratio and the off-state, not about whether the MCU should be able to read and reset the latch.
- **CONFIRMED (v21) — this is why the first board does not work, and it is a design fault, not a build fault.** Six measurements on the built board, all referenced to BATT−:

  | Point | Measured | Expected | |
  |---|---|---|---|
  | U2 pin 14 (latch rail) | 3.5 V | 3.3–3.5 V | ✅ D1 and C9 correct |
  | U2 pin 1 (`1~RD`) | 3.2 V | 3.3–3.5 V | ✅ reset released |
  | U2 pin 4 (`1~SD`) | 3.5 V | 3.3–3.5 V | ✅ set released |
  | **U2 pin 3 (`1CP`), no magnet** | **2.4 V** | **0 V** | ❌ **the fault** |
  | U2 pin 3, magnet applied | 3.6 V | 3.5 V | ✅ the reed works |
  | U2 pin 5 (`1Q`) | 0 V, never changes | toggles | consequence |
  | Q1 gate | 0 V, never changes | follows pin 5 | consequence |

- **Mechanism.** Pin 3 is the clock and the 74HC74 triggers on a **rising edge**. Held at 2.4 V the input already reads as high (V<sub>IH</sub> is 0.7 × 3.5 = 2.45 V, and the real switching threshold is near 1.75 V), so the magnet's move to 3.6 V produces **no edge at all**. Q never toggles, the gate never rises, the MOSFET never conducts. Every one of the seven readings follows from that.
- **Source of the 2.4 V.** Pin 3 is pulled to BATT− by **R14, 470 kΩ**. The Pro Mini's **A0** reaches the same node through **R13, 100 Ω** — R13 is **4,700× stronger**. When the device is off the Pro Mini has BATT+ on its VCC and a floating ground, so it is half-powered and leaks current out of A0. Working back from the measurement, the internal path looks like about **235 kΩ** to BATT+: 3.6 × 470/(470+235) = **2.40 V**, exactly what was read. 235 kΩ is an ordinary figure for a CMOS pin on a chip that is powered without a ground.
- **Immediate proof and workaround:** lift one leg of **R13**. Pin 3 should fall to ~0 V and the magnet should then work. If it stays at 2.4 V, R14 is open instead — measure pin 3 to BATT− at ~470 kΩ with power off.
- **Proper fix — three resistor/capacitor changes:**

  | Part | Now | Change to | Effect |
  |---|---|---|---|
  | R13 | 100 Ω | **1 MΩ** | the MCU can no longer overpower the pull-down |
  | R14 | 470 kΩ | **100 kΩ** | the pull-down wins with margin |
  | C12 | 100 nF | **470 nF** | keeps R14 × C12 at 47 ms, so HW-014's debounce is unchanged |

  | | Pin 3 when off | Valid low needs | Debounce |
  |---|---|---|---|
  | as built | **2.40 V** | < 1.05 V | 47 ms |
  | R13 → 1 MΩ alone | 0.99 V | < 1.05 V | 47 ms — 60 mV margin, too tight |
  | **all three** | **0.27 V** | < 1.05 V | **47 ms** ✅ |

  Cost: with a magnet left sitting on the reed, current rises from ~7.7 µA to ~35 µA. That only flows while a magnet is physically present, so it does not touch the two-year budget.
- **CORRECTION (v23) — do NOT change R9. The earlier advice in this entry to raise it to 1 MΩ was wrong and would have broken the commanded shutdown.** R9 is how A1 pulls the flip-flop's active-low reset down against R11's 1 MΩ pull-up; the two form a divider, so R9 must be the *stronger* side or the MCU can never assert the reset at all:

  | R9 | Pin 1 when A1 drives low | |
  |---|---|---|
  | **100 kΩ (as built)** | **0.32 V** | ✅ resets |
  | 220 kΩ | 0.63 V | ✅ resets |
  | 470 kΩ | 1.12 V | ❌ will not reset |
  | 1 MΩ | 1.75 V | ❌ will not reset |

  V<sub>IL</sub> max is 1.05 V on a 3.5 V rail. **R9 stays at 100 kΩ.** The A1 path is not the same case as the A0 path: on A1 the off-state leak pushes pin 1 *up*, which is the inactive direction for an active-low reset, and the measured 3.2 V is still a solid high. Only the A0 path needed fixing, and it is fixed by removing R13. The firmware rule below still applies to both pins.
- **Firmware rule, now mandatory:** **A0 and A1 are inputs with pull-ups disabled at all times**, except the few milliseconds A1 spends deliberately commanding a shutdown, after which it returns to an input immediately. Neither pin may ever be left as an output.
- **The general lesson for this design, worth carrying into any respin:** low-side switching means the Pro Mini is *always* half-powered when the device is off. **Any connection from an MCU pin into the always-on latch domain must be high-impedance enough to lose to that domain's own pull-up or pull-down.** R13 at 100 Ω against 470 kΩ was never going to work; the resistor was sized for pin protection without anyone asking what it does in the off state.

---

### HW-068 — R11's 1 MΩ pull-up is marginal against the 74HC74's worst-case input leakage
- Severity: MINOR
- Status: OPEN — raised 2026-08-24 during bring-up analysis
- Component / net: R11 1 MΩ, `U2.1 (1~RD)`; same argument applies to the 1 MΩ on the flow input
- Problem: the 74HC family specifies input leakage of up to **±1 µA** over temperature. Through 1 MΩ that is a **1 V** drop. On a 3.4 V rail the reset pin could sit as low as **2.4 V** against a V<sub>IH</sub> of 0.7 × 3.4 = **2.38 V**. That is a margin of about 20 mV in the worst case.
- Impact: in practice HC leakage is nanoamps at room temperature and this works fine — which is what makes it dangerous. It is a margin that shrinks as the board heats up on a roof, so the failure would appear months into deployment, in summer, in the field, and would look random.
- Fix: **220 kΩ instead of 1 MΩ.** Worst-case drop becomes 0.22 V, leaving the pin above 3.1 V. The cost is the standing current when the pin is pulled low — but that only happens while A1 is deliberately commanding a shutdown, so it is microamps for milliseconds, not a continuous drain.
- Notes: raised as MINOR because it has not been observed failing and there is no evidence it is behind the current bring-up problem. Worth folding into the same respin as **HW-067**, since both are about this one pin.

---

### HW-069 — Slow magnet approach makes the reed chatter, and the RC filter is too short to catch it
- Severity: MAJOR
- Status: OPEN — observed on the first working board, 2026-08-24
- Component / net: S1, R12, R14, C12, `U2.3 (1CP)`
- Problem, as reported from the bench: *"sometimes when i put the magnet close to the reed switch it's turn on and off and on — like you should be fast or something. it's not happen always but sometimes."*
- Mechanism: bringing the magnet in **slowly** parks it at the distance where the field is only just above the reed's pull-in threshold. There the blades are barely closed, and ordinary hand tremor opens and closes them several times. **Every closure is a rising edge, and every rising edge is a toggle**, so the latch lands wherever the last wobble left it. Moving the magnet in quickly crosses that zone too fast to wobble, which is exactly the behaviour reported.
- Why the existing filter does not catch it: R14 × C12 = 470 kΩ × 100 nF gives a **47 ms** recovery. That was sized in **HW-014** against *contact bounce*, which lasts a few hundred microseconds, and it does that job — bounce has never been a problem on this board. Hand chatter is two orders of magnitude slower, perhaps 100 ms between wobbles, so the node falls back below threshold between them and each closure produces a fresh edge.
- **This means my v16 decision to drop the Schmitt trigger was wrong.** HW-014 originally required one; I withdrew it on the grounds that R14 + C12 gave sufficient debounce. That reasoning covered bounce and not chatter, and the two need very different time constants.
- Fix, in two stages:

  **1. Bench experiment, one part:** **C12 100 nF → 1 µF**, giving a **470 ms** window. If the chatter stops and toggling stays reliable, that may be enough. If toggling becomes unreliable, revert — the rising edge into the clock has gone from ~10 µs to ~100 µs and the flip-flop is objecting. *This is genuinely an experiment: the 74HC74's maximum input transition rate could not be checked, because every datasheet host is blocked from this environment.*

  **2. Rev B, the proper fix:** a **74LVC1G14** Schmitt-trigger inverter (SOT-23-5, <1 µA, a few cents) between the RC node and `U2.3`:

  ```
  reed ──R12──┬──────────────> Schmitt input
              │
       R14 ───┴─── C12 ─── BATT−

       Schmitt output ──┬──> U2 pin 3  (clock)
                        └──R13──> A0
  ```

  This settles three things at once:
  - **The window can be as long as wanted** — a Schmitt accepts arbitrarily slow inputs and emits a clean fast edge, so C12 is free to be 1 µF or 10 µF.
  - **HW-067 is fixed structurally rather than by resistor ratio.** A0 hangs off the Schmitt's *driven output*, which the Pro Mini's ~15 µA leak cannot move. R13 can stay 100 Ω, and the "MCU can see the magnet" feature survives intact.
  - The toggle lands on magnet **removal**, a more definite gesture than an approach.
- **Mechanical fix, free, worth doing regardless:** a shallow pocket or dimple in the enclosure at the magnet spot, sized so the magnet drops into one defined position. The field at the reed then goes from nearly nothing to well past threshold as the magnet seats — it cannot hover at the edge because there is nowhere to hover. This also fixes the "where exactly do I put the magnet" problem for the installer, and ties into **HW-015**'s marking requirement.
- Notes: contact wear is not a concern here even with a larger C12 charging through R12 — **HW-014** established that the device is switched roughly five times in two years, so inrush per closure is irrelevant.
- **Bench fix chosen for the prototype (v23) — change R14, not C12.** The two ways to lengthen the ignore-window are not equally safe:

  | Change | Ignore window | Rising edge into the clock | Magnet-held current |
  |---|---|---|---|
  | as built, R14 470 kΩ / C12 100 nF | 57 ms | 10 µs | 7.4 µA |
  | C12 → 1 µF | 566 ms | **100 µs — the flip-flop may object** | 7.4 µA |
  | **R14 → 2.2 MΩ** | **265 ms** | **10 µs — unchanged** | **1.6 µA** |

  Raising R14 leaves the clock edge completely alone, gives nearly five times the window and *reduces* the magnet-held current. It is the better prototype experiment, and it is instantly checkable: measure U2 pin 3 with no magnet and it must sit under 0.3 V. Superseded the C12 experiment.
- **Why 2.2 MΩ is a prototype fix and not the production answer.** The 74HC family allows up to **±1 µA** of input leakage, which through 2.2 MΩ would lift the pin by 2.2 V — enough to hold the clock high permanently, which is the exact failure of HW-067 arriving by another road. Real HC parts leak nanoamps at room temperature, so this works on the bench; it works because the chip is better than its specification. That is acceptable for a prototype and not acceptable for a sealed box on a roof for two years. **1 MΩ is the highest value defensible from the datasheet alone** (worst case 1.0 V against a 1.05 V limit), and it only buys a 120 ms window.
- Consequence: the **Schmitt trigger is required for production**, not optional. It removes the dependency on leakage entirely.
- Flux matters at these values: a hand-built board with residue between pads can conduct enough at 2.2 MΩ to hold the pin up. Clean around U2 pins 1 and 3 with IPA before judging the result.

---

### HW-061 — C6, the radio's 100 nF, sits 12.2 mm from U3
- Severity: MINOR  *(was MAJOR in v18 — corrected, see below)*
- Status: OPEN — found in the 2026-08-24 PCB check
- Component / net: C6, C7, C8 against U3; C9 against U2
- Problem: this is the placement condition that was attached to **HW-013** when it was closed at the schematic stage — a capacitor only works where it is put. Measured nearest pad to nearest pad on the board:

  | Cap | Serves | Distance | |
  |---|---|---|---|
  | C1 → J1 | flow filter | 3.6 mm | fine |
  | C5 → U1 | Pro Mini 100 nF | 4.0 mm | fine |
  | C4 → BATT | 100 nF | 4.9 mm | fine |
  | C10, C11, C12 → U2 | 100 nF × 3 | 5.1–5.3 mm | fine |
  | C2 → J3 | ultrasonic 100 nF | 5.7 mm | acceptable |
  | **C8 → U3** | **radio 10 µF bulk** | **8.3 mm** | **too far** |
  | **C7 → U3** | **radio 100 µF bulk** | **12.1 mm** | **too far** |
  | **C6 → U3** | **radio 100 nF** | **12.2 mm** | **too far** |
  | **C9 → U2** | **latch hold-up 10 µF** | **15.0 mm** | **too far** |

  The four that fail are the four that carry the whole argument of HW-042 and HW-013.
- Impact: C6, C7 and C8 exist because the SX1278 pulls up to 120 mA for a few milliseconds while the cells can supply about 50 mA. The capacitors have to hand that current over immediately. A 12 mm trace is roughly 12 nH in series, and series inductance is precisely what stops current changing fast — so at that distance the capacitor delivers its burst *through* the thing it was fitted to bypass. C9 is worse in kind: it is the part that holds the 74HC74's supply up through a transmit so the latch does not forget "on" and shut the device down on a roof. At 15 mm it is holding up the wrong end of a wire, which re-opens **HW-042** by the back door.
- Fix: placement only, no schematic change. **C6 hard against U3's 3.3 V and GND pads, target under 3 mm**, with C8 then C7 just outside it in that order. **C9 against U2 pins 14 and 7.** Route each one's ground straight into the pour with its own via once **HW-062** is done.
- Notes: C3 sits 11.8 mm from BATT, but C3 is the spare general-purpose 100 nF rather than a device bypass, so it is not counted as a failure here. Give it a job — put it at whichever device ends up furthest from its own cap after the moves above.
- **CORRECTION (v19) — this issue was overstated in v18, and it was overstated in a way that would have wasted your time.** It listed C6, C7, C8 and C9 as all being too far away, on the general rule that decoupling belongs next to its chip. Working the actual numbers instead of the rule of thumb, **only C6 is degraded**:

  | Cap | Distance | Verdict |
  |---|---|---|
  | **C6** 100 nF at U3 | 12.2 mm | **move it** |
  | C7 100 µF at U3 | 12.1 mm | fine where it is |
  | C8 10 µF at U3 | 8.3 mm | fine where it is |
  | C9 10 µF at U2 | 15.0 mm | fine where it is |

- **Why C6 genuinely matters.** A capacitor stops acting like a capacitor above its self-resonant frequency, where the inductance of the loop reaching it takes over. C6's loop is out and back, roughly 24 mm, which at ~1 nH/mm is about 20 nH:

  | C6 position | Loop inductance | 100 nF useful up to |
  |---|---|---|
  | 12.2 mm (now) | ~20 nH | **3.6 MHz** |
  | 3 mm | ~5 nH | **7.1 MHz** |
  | pads touching | ~1.2 nH | 14.5 MHz |

  Moving it doubles its useful range and costs nothing.
- **Why C7 and C8 are fine.** They are bulk capacitors — they supply 120 mA for the few **milliseconds** a transmission lasts, and on that timescale inductance is irrelevant. What matters is resistance: 12 mm of 0.5 mm-wide 1 oz copper is about **12 mΩ**, which at 120 mA is **1.4 mV**. Out of 3600 mV.
- **Why C9 is fine.** It holds the 74HC74's rail up during a burst, and the 74HC74 draws **microamps**. Over a 5 ms burst, 10 µF supplying 10 µA sags **5 mV**. Trace resistance at those currents is not measurable. **HW-042 is not re-opened by this** — the v18 entry claimed it was, and that claim is withdrawn.
- Fix, revised: **move C6 only.** Target under 3 mm from U3's 3.3 V and nearest GND pad, with a via from C6's ground pad straight into the pour. Leave C7, C8 and C9 alone. Step-by-step in `PCB-FIXES.md`.

---

### HW-062 — One via on the entire board, and it is on BATT+
- Severity: MINOR  *(was MAJOR in v18 — corrected, see below)*
- Status: OPEN — found in the 2026-08-24 PCB check
- Component / net: whole board; the single via is on BATT+
- Problem: the board has **exactly one via**. There are **zero ground stitching vias**. The ground pour is on the Top layer; 1305 mm of routing across 305 segments is on the Bottom layer. The pour reaches bottom-layer ground only through the 20 through-hole GND pads that happen to exist.
- Impact: 20 through-hole pads is a real connection, but it is not stitching. Return current from a bottom-layer signal has to travel sideways to the nearest ground pad before it can reach the pour above — and that sideways detour *is* the loop area **HW-004** was written about. The pour was supposed to remove the detour, not relocate it. **HW-007** asked specifically for stitching around the Ra-02 footprint, and there is none.
- Fix: stitch the pour. A loose grid of vias across the board, plus a tight ring around U3, around U1, and where GND_SW and GND_RAW come together at Q1. A dozen well-placed vias cost nothing at fabrication and are the entire reason for having a pour.
- Notes: do this **after** HW-060 is settled, because fixing the Ra-02 footprint moves U3 and everything routed to it.
- **CORRECTION (v19) — downgraded, because the v18 entry undercounted what is already there.** This board is **entirely through-hole**. Every component lead sits in a plated hole, and a plated hole *is* a via. There are **20 ground pads spread across the board**, and every one of them already ties the top pour to the bottom-layer ground. The layers are not connected in one place; they are connected in twenty.
- **What stitching still buys, and why it is worth an hour anyway:**
  1. **It repairs the slots from HW-063.** Where a top-layer trace cuts the pour, vias on both sides keep the separated pieces joined through the bottom layer.
  2. **The radio.** A ring of vias around U3 gives the transmit return current somewhere short to go, which is what **HW-007** asked for and is the one place on this board where it measurably matters.
- Fix, unchanged in substance but now correctly prioritised — do it **after** HW-063, not before, since moving the top-layer traces changes where the slots are. `Tools → Via Stitching/Shielding → Add Stitching to Net`, net GND, 5 mm grid, then 6–8 by hand around U3. Step-by-step in `PCB-FIXES.md`.

---

### HW-063 — 295 mm of top-layer routing cuts slots through the ground pour
- Severity: MAJOR
- Status: OPEN — found in the 2026-08-24 PCB check
- Component / net: 68 tracks on the Top layer
- Problem: the Top layer carries the ground pour *and* 68 signal tracks totalling 295 mm. Every one of those tracks is a slot through the pour:

  | Net | Segments | Length on Top |
  |---|---|---|
  | BATT+ | 27 | **75 mm** |
  | NetC9_1 (latch rail) | 5 | 64 mm |
  | NetC12_1 (74HC74 pin 3) | 5 | 62 mm |
  | GND | 16 | 50 mm |
  | BATT- | 8 | 31 mm |
  | NetU2_2 | 7 | 13 mm |

- Impact: a pour is only a ground plane where it is continuous. Where a trace crosses it the return current underneath must detour around the slot, and around a 75 mm trace is a long way. This is the slotted-plane trap written up in `HYDRO-NODE-REFERENCE.md` §11 — the failure mode where a board has a pour, passes visual inspection, and still behaves as though it has no plane.
- Fix: the Bottom layer already carries 1305 mm and clearly has room. Move as much of the 295 mm to Bottom as will go, **BATT+ first**. Whatever must stay on Top should be short and kept away from U3.
- Notes: the 50 mm of **GND** on Top is a different case — that is pour-net copper and harmless. It also becomes unnecessary once **HW-062** stitches the pour properly.
- **Priority note (v19): this is now the first PCB job, not the third.** With HW-060 withdrawn and HW-061 and HW-062 downgraded, this is the only remaining MAJOR on the layout and the only one that changes how the board behaves. The 75 mm BATT+ run on Top is the single biggest thing standing between this board and the ground plane HW-004 was closed on. Do it before the stitching vias, because moving these traces changes where the slots are.
- Because the board is entirely through-hole, **moving a trace from Top to Bottom needs no extra vias** as long as both ends land on component pads — the pads already go through the board. That makes this a much smaller job than it looks. Step-by-step in `PCB-FIXES.md`.

---

### HW-064 — Remove Dead Copper is disabled on the ground pour
- Severity: MINOR
- Status: OPEN — found in the 2026-08-24 PCB check
- Component / net: the GND polygon, `REMOVEDEAD=FALSE`
- Problem: the pour keeps isolated copper islands that connect to nothing.
- Impact: at 433 MHz an unconnected piece of metal is a piece of metal that can resonate and re-radiate. The effect is modest, but the setting costs nothing to change and there is no reason to keep dead copper on an RF board.
- Fix: set **Remove Dead Copper = true** on the polygon and repour. Do it last, after the placement and stitching changes, so it only has to be done once.
- Notes: `POUROVER=FALSE` is correct as it stands — same-net objects get thermal reliefs rather than being flooded over, which keeps hand soldering possible on a through-hole board.

---

### HW-065 — J1's boss pad has zero annular ring, and two silkscreen violations sit on S1's pads
- Severity: MINOR
- Status: OPEN — found in the 2026-08-24 PCB check
- Component / net: J1 mounting boss; S1 pads
- Problem, part 1: J1's third pad is **43.3 mil across with a 43.3 mil hole** — the drill removes the entire pad, leaving no copper ring at all. It is the mounting boss of the `B2B-XH-AM` connector and carries no signal, so nothing fails electrically, but a plated pad with no ring is something a fab will query or silently change.
- Problem, part 2: two DRC violations are **already stored in the board file** from your own run:

  ```
  [Top Overlay] to [Top Solder] clearance [0.25mm]  at (2127.95, 1151.57) mil
  [Top Overlay] to [Top Solder] clearance [0.25mm]  at (3423.23, 1151.57) mil
  ```

  That y-coordinate is exactly **S1's pad row** — silkscreen ink printed across the reed switch's exposed pads.
- Impact: ink on a pad gives a bad solder joint, and on a hand-built board it is the kind of fault that reads as a cold joint and gets chased for an hour. Every other DRC error in the report is hidden behind noise like this.
- Fix: change the J1 boss to a **non-plated hole**. Trim the S1 body outline back off its pads. Then re-run DRC and get the report to zero, so a real error cannot hide.
- Notes: the ECO will also report unmatched pins for S1's stray symbol pair (**HW-057**) and, if the Pro Mini symbol still has 34 pins against a 30-pad footprint, for A4–A7. Clear those with explicit No ERC markers rather than by ignoring the report.

---

### HW-066 — M3 mounting holes sit 2.5 mm from the board edge
- Severity: MINOR
- Status: OPEN — found in the 2026-08-24 PCB check
- Component / net: four mechanical pads, 3.0 mm holes with 4.0 mm pads
- Problem: the board is **90 × 70 mm** (x 25.5–115.5, y 25.5–95.5 mm). The holes are at x = 28.0 and 113.0, y = 28.0 and 93.25 — leaving **1.0 mm of board material between the hole wall and the edge**, and only 0.5 mm between the pad edge and the board edge.
- Impact: an M3 washer is about 7 mm across and will overhang the board on two sides, so it cannot sit flat. 1 mm of remaining material next to a mounting hole is also where a board cracks if the enclosure is ever over-tightened — and this one lives on a roof through a Syrian summer, expanding and contracting against whatever it is screwed to.
- Fix: move the holes in to roughly 3.5 mm from each edge, if the enclosure's boss positions allow. If they do not, the enclosure needs the standoffs moving instead.
- Notes: the top pair is at y = 93.25 mm and the bottom pair at y = 28.00 mm — 2.25 mm from one edge against 2.5 mm from the other. Almost certainly not deliberate; worth squaring up while the holes are being moved.

---

### HW-057 — The reed symbol has a stray second pin pair parked off-sheet
- Severity: MINOR
- Status: OPEN — found in the 2026-08-22 schematic check
- Component / net: `S1`, `LibReference=REED-SWITCH-NO-D4L29`, `PartCount=2`
- Problem: S1 carries **four pin records, two of them also numbered 1 and 2**, sitting at roughly **(1060, −180)** — far below and to the right of the drawing, outside the visible sheet, with no wires. The pins that matter are wired correctly at (470, 260) and (470, 320): `S1.2` to the latch supply, `S1.1` through R12 to the flip-flop clock.
- Impact: two more unconnected-pin errors, and a footprint ambiguity. If the reed footprint has two pads, the netlist has two pin-number collisions and the PCB import may map the wrong one. If it genuinely has four, two pads are unused and nobody has said so.
- Fix: open the S1 symbol and settle it. If the reed is a two-lead part — which the one in `Components Images/F2293658-01.jpg` is — **delete the stray pair**. If the footprint has four pads, wire them or mark them no-connect explicitly.
- Notes: the same check is worth running on LS1, which also reports `PartCount=2` but has only one part placed.

---

### HW-001 — Ultrasonic harness is a cross-over cable with no controlled drawing
- Severity: MAJOR *(reduced from BLOCKER in v5)*
- Status: OPEN
- Component / net: J5 (4-pin), sensor harness, RCWL-1670
- Problem: The schematic defines J5 as **1=GND, 2=VCC, 3=Echo, 4=Trig**; the RCWL-1670's pads are **GND, RX (=TRIG), TX (=ECHO), +5V**. Position-for-position, pins 2 and 4 are swapped. You have confirmed the module is never plugged directly into the header — it lives in its own enclosure inside the tank and is reached by a 4-wire cable, and you know the correct order. So this is no longer a functional defect on the built article. What remains is that **the harness is a cross-over cable, and nothing in the design documents says so.**
- Impact: On a production line an operator building a harness from the schematic, or from an ordinary pin-1-to-pin-1 convention, will build it straight-through. That harness puts VBAT on the module's TRIG input and the MCU's D6 output onto the module's supply pin. It looks identical to a correct one, so it passes visual inspection and fails at functional test — or worse, damages the MCU pin. Fails the "repeatable assembly" half of NFR-6.
- Recommended fix: Make a straight-through harness correct, so the cross-over cannot be built wrong:
  1. **Re-order J5 in the schematic and PCB to 1=GND, 2=Trig, 3=Echo, 4=VCC**, matching the module's physical pad order. Then a 1:1 cable is the right cable and there is nothing to get wrong.
  2. Print the four signal names on the silkscreen beside J5 (HW-038).
  3. Whatever order you settle on, issue the harness as a **controlled drawing** — wire colours, both connector pinouts, length — and put it in the BOM (HW-033). A cross-over cable that exists only in someone's head is not a production document.
- Notes (v5): Downgraded because you confirmed the assembly method and that the pinout is known. Not closed, because the schematic still disagrees with the hardware and the harness is undocumented. It closes when the schematic is corrected or the harness drawing exists.
- **Update (v17) — the cross-over is now permanent, and this issue is the only thing standing between it and a wrong cable.** HW-054 is closed WON'T FIX: J3 stays `GND · +5V · TX · RX` while the RCWL-1670's pads are `GND · RX · TX · +5V`. That is a deliberate decision, so the harness **must** swap pins 2 and 4:

  | J3 pin on the board | Wire goes to module pad |
  |---|---|
  | 1 — GND | GND |
  | 2 — +5V | **+5V (pad 4)** |
  | 3 — TX | TX (pad 3) |
  | 4 — RX | **RX (pad 2)** |

  A straight 1:1 cable puts 3.6 V on the module's RX pin and leaves the module unpowered. Nobody can infer the crossover from the board, so **the harness drawing is now a blocking prerequisite for building any cable**, not a documentation nicety. It needs: wire colours, the pin-to-pad table above, overall length, and a continuity test step. Silkscreen must read `GND · +5V · TX · RX` to match the board (**HW-038**).

---

### HW-005 — Ultrasonic sensor enclosure, transducer feedthrough and condensation management
- Severity: MAJOR *(reduced from BLOCKER in v5)*
- Status: NEEDS INFO
- Component / net: RCWL-1670, its enclosure, J5 harness
- Problem: The module is a bare, uncoated PCB and the tank headspace is at or near 100 % RH — outside the module's own 5–95 % RH rating. You have confirmed the intended arrangement: **the ultrasonic gets its own waterproof enclosure mounted inside the tank at the top, and the Hydro Node sits outside the tank nearby.** That addresses the main concern in principle. What is not yet defined is the part that actually decides whether it survives: how the transducers get out of that enclosure, and what happens to condensate.
- Impact: The transducer faces must be exposed to the headspace air to work, so the enclosure necessarily has an opening or a feedthrough at exactly the point that is hardest to seal — and it is the coldest surface in a saturated space, so it is where water condenses first. Condensate on a transducer face attenuates the echo and causes dropouts that read as bad level data rather than as a fault. Threatens NFR-2 and NFR-3.
- Recommended fix:
  - **Pot the transducer feedthrough**, do not gasket it. Two-part polyurethane or silicone potting around the transducer barrels, with the PCB in the dry side of the enclosure.
  - **Conformally coat the module PCB anyway**, as a second barrier. It costs almost nothing and the enclosure will eventually breathe.
  - Add a **drip shield** above the transducer faces so condensate running down the underside of the tank lid cannot fall onto or pool on them.
  - Orient the assembly so the transducer faces point straight down and nothing can sit on them.
  - The enclosure needs its own **breather membrane** for the same reason as the main one (HW-028) — a sealed box that swings 40 °C daily will pump moisture in.
- Notes: **Questions still open:** (1) Is your RCWL-1670 the variant with the transducers soldered to the board, or on flying leads? Flying leads make the potted feedthrough much easier. (2) What is the tank material and lid construction — this decides how both the sensor enclosure and the cable penetration (HW-045) get mounted. (3) Roughly what temperature range does the headspace see at your site?

---


### HW-012 — No ESD or surge protection on the three external sensor cables
- Severity: MAJOR
- Status: OPEN
- Component / net: J3 (Temp), J4 (Flow), J5 (Ultrasonic) — U2 pins D3, D4, D5, D6, D7
- Problem: Three cables leave a sealed enclosure and run across a rooftop to a tank and a fill pipe. Nothing on the board protects the pins they land on.
- Your question, answered: *"Protection from what? The device runs on 3.6 V, there is no 220 V mains here."* This is a common and reasonable assumption, but **ESD and induced surge have nothing to do with the circuit's own supply voltage.** The energy comes from outside:
  1. **The installer is the source.** A person walking on a roof in dry, dusty air routinely carries 10–25 kV of static. The standard human-body model is 2 kV through 1.5 kΩ, which is a **~1.3 A peak for about 150 ns**. When that person touches a connector pin or a bare cable end, all of it goes into the MCU pin. Your 3.6 V rail neither causes nor limits it. If anything a 3.6 V CMOS part is *more* fragile than a 5 V one — thinner gate oxide, less headroom.
  2. **Syria's climate makes this worse, not better.** Dry, dusty air is precisely the condition that generates the highest static potentials. Low humidity means charge does not bleed away. This is a harsher ESD environment than a humid coastal one, not a gentler one.
  3. **Induced surge from nearby lightning.** A cable running across a roof and down into a tank forms a loop. A strike a few hundred metres away couples into that loop magnetically — no direct hit required — and induces tens to hundreds of volts. Again, independent of your supply voltage.
  4. **The cable is the collector, not the supply.** The exposure is created by having several metres of wire outside the box, which is a design property you cannot remove.
- Impact: A dead GPIO on a sealed device on a roof. Not repairable in the field, so every occurrence is a site visit and a replacement unit. The **DS18B20 1-Wire line (D4) is the most vulnerable** — long, high impedance, pulled up, and connected straight to a pin with nothing in between.
- Recommended fix, tiered so you can take the cheap 80 % now:
  1. **Nearly free — do this regardless.** A **100 Ω series resistor** at the MCU end of every externally-exposed signal: D4 (1-Wire), D5 (flow), D6 (Trig), D7 (Echo). Six cents of resistors. It limits current into the ATmega's internal clamp diodes, which are good for roughly 1 mA of continuous injection, and it also limits fault current if a cable shorts to a rail. At 100 Ω the effect on 1-Wire timing and on the echo edge is negligible.
  2. **Full protection.** A **low-capacitance ESD/TVS array to ground** on each of those lines — choose < 5 pF so the echo edge and the 1-Wire timing are not slowed — plus one across the supply feeding the ultrasonic module. Place them at the connectors with a short path to the ground pour, so the transient never travels across the board.
- Notes: This is one of the cheapest items on the whole list relative to what it prevents, and unlike most of the others it cannot be retrofitted — the pads have to exist in the respin. Take at least tier 1.
- Notes: Also relevant to **HW-050** — a metal tank is a large conductor that the sensor cable enters, which gives an induced transient somewhere convenient to couple into.

---




### HW-017 — IRLZ44N is the wrong MOSFET, in the wrong package, in the wrong place
- Severity: MAJOR
- Status: OPEN
- Component / net: Q1
- Problem: Three issues:
  1. **Unspecified at this gate drive.** The IRLZ44N's R_DS(on) is characterised at V_GS = 5 V and 10 V. Here it is driven at 3.6 V falling to 3.0 V, against a V_GS(th) of 1.0–2.0 V max. It will conduct, and at ~150 mA the resistance is irrelevant — but it is an uncharacterised operating point and the worst-case margin at end of life is only 1.0 V over threshold.
  2. **Wrong size by three orders of magnitude.** A 47 A / 55 V TO-220 is switching about 150 mA. Its off-state leakage is specified as 25 µA at 55 V and 250 µA at 125 °C — nowhere near the 3.6 V, 60 °C point you actually care about for the OFF-state budget.
  3. **Mechanically unsound.** From the 3D view, the TO-220 body **overhangs the board edge** with the tab unsupported and unrestrained. In a TO-220 the tab is electrically the **drain**, i.e. the switched-ground net — an exposed live metal tab flapping inside a sealed enclosure.
- Impact: Unquantifiable OFF-state leakage (NFR-1), lead fatigue and a potential internal short (NFR-3, NFR-6).
- Recommended fix: Replace with a small **SOT-23 logic-level N-channel MOSFET explicitly specified at V_GS = 2.5 V**, with a datasheet I_DSS in the nanoamp range at low V_DS. Mount it flat on the board inside the outline. Keep R1 (1 MΩ pulldown) and R2 (1 kΩ gate series) as they are — that part is correct.
- Notes: While you are here, decide whether you still want a master hardware latch at all. See the note under HW-021.

---




### HW-022 — No local unpair or recovery path on a sealed Node
- Severity: MAJOR
- Status: OPEN
- Component / net: System-level; U2, S1
- Problem: FR-7 states that unpairing is initiated from the Hub only. If a Hub is lost, destroyed or replaced, the Node is permanently bound to a Hub that no longer exists — inside a sealed enclosure with no ports, no buttons and (per HW-021) no way for the magnet to say anything except "toggle power".
- Impact: A dead Hub bricks every Node on the site. That is a support and warranty problem, not just an engineering one.
- Recommended fix: Implement HW-021's reed-sense line, then define a **magnet gesture** for local unpair — e.g. hold the magnet on the target for 10 seconds *after* power-on, confirmed by a distinctive LED pattern. This preserves the intent of FR-7 (no accidental unpairing, Hub is the normal path) while giving a documented field-recovery route. Firmware-only once the sense wire exists.
- Notes: Raised now because it costs one PCB net; retrofitting it after the respin is expensive. The protocol details belong in Stage 6.

---

### HW-023 — A single air-temperature sensor cannot represent the headspace thermal gradient
- Severity: MAJOR
- Status: OPEN
- Component / net: J3, DS18B20
- Problem: FR-2 corrects the speed of sound using one DS18B20 in the headspace. But on a sunlit rooftop tank, the air touching the hot tank roof can be 15–25 °C warmer than the air just above the cool water. The ultrasonic pulse travels through that entire gradient, so what matters is the **path-average** temperature — and a single sensor mounted near the sensor (i.e. at the hot end) systematically over-estimates it.
- Impact: **This is the dominant error source in the whole measurement.** The speed of sound changes by about 0.606 m/s per °C, so a 1 °C path-average error is a 0.177 % distance error. A realistic 8–10 °C path-average error gives **28–35 mm of error at a 2 m range** — an order of magnitude worse than every other term in the budget. Directly threatens NFR-2.
- Impact (v9, rescaled — the true range is 0.05–0.15 m full to 0.70–1.00 m empty, much shorter than v8 assumed):

  | Source | @0.15 m (full) | @0.50 m | @1.00 m (empty) |
  |---|---|---|---|
  | Thermal gradient ±8 °C, one sensor | 2.1 mm | 7.1 mm | 14.1 mm |
  | Thermal gradient ±3 °C, two sensors | 0.8 mm | 2.7 mm | 5.3 mm |
  | Ceramic resonator ±0.5 % | 0.8 mm | 2.5 mm | 5.0 mm |
  | Humidity, uncorrected | 0.5 mm | 1.8 mm | 3.5 mm |
  | Parallax, s = 40 mm, uncorrected (HW-052) | 1.3 mm | 0.4 mm | 0.2 mm |

  **The short range is good news.** Every percentage error scales with distance, so at 1.00 m the worst single-sensor term is 14 mm and at 0.15 m it is 2 mm. Note the useful inversion: the temperature and clock errors are worst when the tank is **empty**, while parallax is worst when it is **full** — so no single range is bad for everything, and the two dominant terms never peak together. With two temperature sensors, a crystal and the HW-052 correction, **total error stays under about 7 mm across the whole range**, which is well inside 1 % of tank volume everywhere.
- Recommended fix, in increasing order of effectiveness:
  1. **Two DS18B20s on the same 1-Wire bus** — one at the transducer, one on a lead reaching down near the low-water line — and average them. This is the highest value-for-money fix on this whole list: one extra part, **zero extra pins** (that is the point of 1-Wire), and it turns the worst error term into one of the smaller ones. Do this.
  2. Shade the sensor head and the tank lid so the gradient is smaller to begin with.
  3. Longer term, if you need better than ~1 %: a **fixed reference reflector** at a precisely known distance in the beam, so the firmware measures the actual speed of sound every cycle and cancels temperature, humidity and clock error in one step. Honest caveat: HC-SR04-compatible modules report only the *first* echo, so this needs a module that exposes the raw echo envelope or supports multi-echo. Not achievable with the RCWL-1670.
- Notes: **Humidity is a second, smaller term.** Saturated air raises the speed of sound by roughly 0.35–0.6 % versus dry air at 30–40 °C, which is +7 to +12 mm at 2 m. A tank headspace is essentially always saturated, so this one is easy — apply a fixed saturated-air correction constant on the Hub and most of it disappears. Note that this correction lives on the **Hub**, consistent with your Section 2 split.

---

### HW-024 — Pro Mini 8 MHz clock source is unknown (crystal vs ceramic resonator)
- Severity: MAJOR
- Status: NEEDS INFO
- Component / net: U2 clock source
- Problem: The echo pulse is timed by the MCU, so **the MCU's clock accuracy is the distance measurement's scale factor.** A quartz crystal is ±30 ppm and irrelevant. A ceramic resonator is typically ±0.5 % initial plus a few tenths of a percent over temperature.
- Impact: With a resonator, ±0.5 % is **±10 mm at 2 m**, and it drifts with the rooftop temperature swing, so it is not even a fixed offset you could calibrate out once. That would make the clock the second-largest error term after HW-023.
- Recommended fix: Identify the part on your actual modules. If it is a resonator, put a **±30 ppm crystal (or a TCXO) on the Node PCB** when you move the MCU onto the board (HW-026). If you must ship with a resonator, calibrate each unit's timebase at production test against a known reference distance and store the correction factor in EEPROM — which is a real production step with real cost, and is a good argument for just fitting a crystal.
- Notes: **Please photograph the clock component on one of your modules, or tell me the exact board variant.** SparkFun and the various clone vendors do not all use the same part.

---

### HW-025 — No battery voltage or health telemetry
- Severity: MAJOR
- Status: OPEN
- Component / net: U2, VBAT
- Problem: Nothing measures the battery. There is no divider to an ADC pin, and no other mechanism.
- Impact: For a 2-year sealed field device this is a serious operational gap. You cannot schedule replacement, cannot distinguish "Node is dead" from "Node is out of range", and cannot detect a bad cell batch before it becomes a field campaign.
- Recommended fix: **Use the ATmega328P's internal bandgap reference measured against VCC.** This needs **zero extra components and zero leakage** — you set the ADC mux to the 1.1 V bandgap with AVcc as the reference, read it, and compute VCC = 1.1 × 1024 / ADC. Send the raw ADC count to the Hub and let the Hub do the conversion, per your Section 2 split. Do not use a resistive divider: any divider across VBAT leaks continuously unless you MOSFET-gate it, which is more parts for a worse answer.
- Notes: For Li-SOCl₂ the open-circuit voltage is famously flat, so absolute voltage tells you little about remaining capacity. The genuinely useful signal is the **loaded voltage dip during the LoRa TX burst** — sample VCC mid-transmission. A dip that grows over months is the real end-of-life and passivation indicator. Log both.

---

### HW-026 — Entirely through-hole, socketed modules, hand assembly — not viable at production volume
- Severity: MAJOR
- Status: OPEN
- Component / net: Whole board
- Problem: Every part is through-hole: ½ W axial resistors, DIP-14 CD4013, TO-220 MOSFET, a radial electrolytic, a glass reed, and two 8-pin **sockets** into which a Pro Mini and an Ra-02 are plugged. Plus, per HW-002, every Pro Mini needs manual rework (LED and LDO removal, fuse programming) before it can meet the power target.
- Impact: Fails NFR-6 outright. Hand assembly at volume is slow, expensive and inconsistent; socketed modules oxidise and back out under vibration and thermal cycling; per-unit module rework is the kind of step that gets skipped on a busy line and produces a batch of 88-day products.
- Recommended fix: Respin for automated assembly:
  - **Put the ATmega328P directly on the board** (or move to a modern low-power MCU — an STM32L0 or nRF52 gives you 1–2 µA sleep with a real RTC instead of the WDT, native ADC references, and a supply chain with actual lifecycle guarantees). Fit an ICSP/SWD header for programming.
  - **Solder the Ra-02 down** rather than socketing it, or use a castellated module footprint.
  - Move to **SMD 0402/0603 1 % metal-film resistors and X7R ceramics**, SOT-23 for Q1.
  - Keep the reed (or Hall device, HW-039) and the connectors as the only through-hole parts, for a single selective-solder or hand-solder step.
- Notes: This is the largest single change on the list, and it is the one that most directly serves "a full manufacturing line will start once the design is finalized". It also resolves HW-002 permanently rather than by rework. Worth deciding early, because it changes the schematic substantially and there is no point fixing the small things twice.

---

### HW-027 — PETG-CF enclosure vs rooftop UV and temperature; FDM prints are not watertight
- Severity: MAJOR
- Status: OPEN
- Component / net: Enclosure (NFR-4)
- Problem: Three distinct problems with the specified enclosure:
  1. **Temperature.** PETG's glass transition is around 80 °C. A dark enclosure in direct rooftop sun can reach 70–80 °C internally in a hot climate. At those temperatures PETG creeps under load — screw bosses relax, gaskets lose compression, and a mounted enclosure can sag.
  2. **UV.** PETG has poor UV resistance. It yellows and embrittles under sustained direct sunlight, and a brittle enclosure cracks at the mounting points. Carbon fill helps the stiffness and the colour but does not make the polymer UV-stable.
  3. **Watertightness.** FDM prints are **porous by construction** — layer lines are leak paths regardless of wall thickness. A printed box is not IP-rated no matter how good the gasket is.
- Impact: Directly threatens NFR-3, and given that everything else is sealed inside it, an enclosure failure is a total product failure.
- Recommended fix:
  - Change material to **ASA-CF or PC-CF**. ASA is the standard UV-stable outdoor choice (it is what exterior automotive trim is made of) and it holds up far better in sun. PC gives a much higher service temperature if you need it.
  - Whatever the material, use a **light colour** and add a separate **ventilated sun shield** over the enclosure so the box itself never sees direct sun. This is worth more than any material change — it can drop the internal temperature by 20 °C and it also helps the battery, the electrolytic and the ultrasonic accuracy.
  - For watertightness, either **pot or conformally coat the PCB** and treat the enclosure as splash protection only, or **use an off-the-shelf IP66/67 polycarbonate enclosure** and print only the internal carrier and the tank-mount bracket. At production volume the off-the-shelf enclosure is almost certainly cheaper, more reliable, and already certified.
- Notes (v7): **Syria makes this materially worse, and it moves from a theoretical concern to a likely failure.** Summer air temperatures across most of the country reach 35–45 °C, with intense direct sun and very high solar irradiance. A dark, sealed enclosure in that environment will reach **70–85 °C internally** — at or above PETG's ~80 °C glass transition. The enclosure will creep: screw bosses relax, gasket compression is lost, and a wall-mounted box can sag on its fixings. Combined with strong UV, PETG-CF is the wrong material for this specific site. **The sun shield is no longer optional, and I would move to ASA-CF regardless.** The high ambient also raises the electrolytic leakage in HW-009, the CD4013/74HC74 quiescent current, and MOSFET leakage — every one of the temperature-sensitive terms in the power budget sits at the bad end of its range here.
- Notes (v7): Dust is the other Syria-specific factor. It affects the breather membrane choice in HW-028 (must be dust-tolerant, not just water-tolerant), and it is part of the fouling picture in HW-048.
- Notes: I know NFR-4 specifies PETG-CF and I am not overriding that — this is the recommendation and the reasoning; the decision is yours. If you want to stay with PETG-CF, the sun shield becomes mandatory rather than optional, and I would want the internal temperature logged over a full summer before sign-off.

---

### HW-028 — No condensation management inside the sealed enclosure
- Severity: MAJOR
- Status: OPEN
- Component / net: Enclosure, PCB
- Problem: A fully sealed box that swings from 10 °C at night to 70 °C in the afternoon will breathe air in and out through any imperfection, carry moisture in, and condense it on the coldest surface inside — which is the PCB.
- Impact: Corrosion, leakage currents between adjacent pads (which shows up as mysterious extra sleep current, i.e. NFR-1), and eventual failure. This is the standard failure mode for sealed outdoor electronics and it is entirely preventable.
- Recommended fix: Three things, all cheap:
  1. A **Gore-type breather vent** (a PTFE membrane) in the enclosure wall. It equalises pressure while blocking liquid water — it is still IP67, so it does not violate NFR-5's "no openings for switches or ports"; it is a sealed membrane, not an opening.
  2. **Conformal coating** on the assembled PCB (acrylic or silicone), masking the connectors.
  3. A **desiccant pack** inside, sized for the enclosure volume — cheap insurance and it belongs in the BOM.
- Notes: The breather vent is the important one. Without it, a sealed box on a roof is a condensation pump.

---

### HW-029 — No test points and no production programming or test interface
- Severity: MAJOR
- Status: OPEN
- Component / net: Whole board
- Problem: There are no test points on the board, and the Pro Mini's DTR/TXO/RXI/RST pins are broken out on the schematic symbol but connected to nothing. Programming and functional test would have to happen on the bare module before assembly.
- Impact: Fails the "testable, serviceable" half of NFR-6. Without an in-circuit test interface you cannot verify a finished board before you seal it, which means defects are found by the customer.
- Recommended fix: Add to the respin:
  - **Test points** on VBAT, GND_RAW, GND_SW, the gate node, the reed node and the 1-Wire line — pads sized for a bed-of-nails fixture.
  - A **programming/test header** (ICSP, or SWD if you move to an ARM part) with UART TX/RX brought out, on a footprint that can be probed by a fixture rather than a plugged connector.
  - A **production self-test firmware mode** that exercises every sensor, reports over the UART, and measures its own sleep current draw against the test fixture — run before the enclosure is closed.
- Notes: Also give each board a **serial number** (a QR/DataMatrix label plus a value in EEPROM). You will need it for the pairing protocol in Stage 6, for field support on a sealed device, and for traceability if a batch goes bad.

---

### HW-030 — Beam containment: sidewall and obstruction clearance
- Severity: MAJOR *(reduced from BLOCKER — my v8 escalation was wrong, see notes)*
- Status: OPEN
- Component / net: RCWL-1670, mechanical mounting
- Problem: The measurement range is now pinned down: **0.05–0.15 m to the full water line, 0.70–1.00 m to the tank floor.** That is a far shorter range than the v8 analysis assumed, and it changes the conclusion. Recomputing where a beam of width D = 2·d·tan(θ/2) reaches the sidewall, against a range that now ends at 1.00 m:

  | Tank | Diameter | 60° beam reaches wall at | 75° beam reaches wall at |
  |---|---|---|---|
  | 500 L | 0.80 m | 0.69 m — clear for 69 % of range | 0.52 m — clear for 52 % |
  | 1000 L | 1.00 m | 0.87 m — clear for 87 % of range | 0.65 m — clear for 65 % |
  | 2000 L | 1.30 m | **never within range** | 0.85 m — clear for 85 % |

  So the sidewall is only in the beam over the last part of the range, i.e. only when the tank is nearly empty — and in a 2000 L tank with a 60° beam, never.
- Impact: Reduced but not zero. Sidewall returns at grazing incidence are weak, so the practical risk is an internal obstruction rather than the wall. The exclusion rule is simple: an object at horizontal offset r only enters the cone once the beam has descended to d = r / tan(θ/2).

  | Object offset from axis | Enters a 60° cone at | Enters a 75° cone at |
  |---|---|---|
  | 0.10 m | 0.17 m | 0.13 m |
  | 0.15 m | 0.26 m | 0.20 m |
  | 0.20 m | 0.35 m | 0.26 m |
  | 0.30 m | 0.52 m | 0.39 m |

  **Keeping the sensor axis ≥ 0.20 m clear of the fill pipe, float valve and outlet is sufficient** for anything shallower than 0.35 m, and objects deeper than that are below the water in a reasonably full tank anyway.
- Recommended fix:
  1. **Make the clearance an installation specification**, not installer judgement: sensor centred on the tank lid where possible, and a stated minimum 200 mm horizontal clearance from the fill pipe, float valve, outlet and any internal structure. Put the table above in the installation guide so a fitter can reason about an awkward tank.
  2. **Gate readings on the flow switch.** A stream of water falling from the fill pipe is a target at every depth it passes through, and if it is near the axis it will return echoes across the whole range. You already have the flow switch (FR-3) — use it: do not trust level readings while flow is detected, and take the post-fill reading a minute after flow stops so the surface has settled. This costs nothing and it solves the one obstruction case that clearance cannot.
  3. **Stilling well — now optional, not required.** Reserve a 110 mm PVC well for 500 L tanks or awkward installations where the 200 mm clearance cannot be met. At 40 kHz the wavelength is 8.6 mm, so 110 mm bore is 12.8 λ and guides cleanly; 75 mm (8.7 λ) also works; 50 mm (5.8 λ) is marginal. Given HW-048's scum problem, do not fit one by default.
- Notes (v9): **I raised this to BLOCKER in v8 and that was wrong.** I had assumed a range extending to 1.5–2.7 m and claimed the float valve would sit inside the beam and capture the reading. With the real range, the geometry says the opposite: objects near the lid are at *small* depth, where the cone is narrow, so they are automatically excluded unless they are almost on-axis. A float sitting at the water surface returns an echo at the water's own distance, which is harmless. Downgraded to MAJOR, and the fix is a clearance spec rather than a stilling well.
- Notes: The dominant near-range risks are now **HW-051** (blind zone against a 5 cm minimum distance) and **HW-052** (split-transducer parallax), not beam containment.

---

### HW-031 — LoRa airtime versus the 2-minute wake interval: the budget only closes at SF7–SF9
- Severity: MAJOR
- Status: OPEN
- Component / net: System-level (Ra-02, FR-4, FR-5, NFR-1)
- Problem: This came out of building the power model and it is a genuine coupling between two requirements that are currently specified independently. TX current dominates the per-wake energy, and LoRa airtime grows roughly 2× per spreading factor step. With a 2-minute interval (525,600 wakes over 2 years) and a 16-byte payload at 125 kHz bandwidth:

  | SF | Airtime | Charge per TX | Active charge over 2 years | Verdict |
  |---|---|---|---|---|
  | SF7 | ~51 ms | 5.1 mA·s | ~1.5 Ah | OK — comfortable |
  | SF9 | ~165 ms | 16.5 mA·s | ~3.0 Ah | Marginal — ~1.3× margin |
  | SF10 | ~330 ms | 33 mA·s | ~5.4 Ah | **Fails** |
  | SF12 | ~1.32 s | 132 mA·s | ~19 Ah | **Fails by ~4×** |

  Against 4.4 Ah of usable capacity (5.2 Ah nominal, 15 % derated for self-discharge, temperature and cut-off).
- Impact: The spreading factor is normally chosen for range. Here it is constrained by the battery, which means **the achievable range is set by NFR-1, not by the radio.** If the Node-to-Hub link needs SF10 or above to close, the 2-year target cannot be met at a 2-minute interval, and one of the two requirements has to move.
- Recommended fix: Fix SF ≤ 9 as a hard design constraint and design the link budget (antenna, mounting, Hub placement) to close within it. Then, to buy margin back, consider an **adaptive interval**: a water tank's level changes slowly, so sample every 5 minutes when idle and every 60 seconds when the flow switch says the tank is filling. That cuts the active budget by roughly 2.5× while *improving* the data where it actually matters. It is a change to FR-5, so it is your call — I am flagging it, not assuming it.
- Notes: Full per-wake model for SF7: MCU wake/init 65 ms @ 4 mA; DS18B20 9-bit conversion 94 ms @ 5 mA; 5× ultrasonic cycles 250 ms @ 10 mA; LoRa config/ramp 50 ms @ 10 mA; TX 60 ms @ 100 mA; housekeeping 80 ms @ 4 mA. Total ≈ 600 ms and 10.05 mA·s = 2.79 µAh per wake, i.e. an 84 µA equivalent average. Add ~20 µA of sleep current after the fixes above and you land at ~104 µA average, which is **4.8 years** — a healthy 2.4× margin on NFR-1. That margin is what pays for the things this model has not counted: retries, cold-temperature capacity loss, and pairing traffic.

---

### HW-032 — Li-SOCl₂ passivation: no depassivation strategy
- Severity: MAJOR
- Status: OPEN
- Component / net: Battery, firmware
- Problem: Lithium-thionyl-chloride cells form a passivating film on the anode during storage and during long periods of very low current draw. The film is what gives these cells their extremely low self-discharge, but it also means the first significant current pulse after a quiet period sees a much higher internal impedance and the terminal voltage dips hard — potentially below the MCU's brown-out threshold or the Ra-02's minimum. This design's duty cycle (µA for 2 minutes, then a 120 mA burst) is precisely the pattern that provokes it.
- Impact: Resets or failed transmissions after storage or after the device sits unpowered, and progressively worse dips as the installation ages. Reads as a random field failure.
- Recommended fix:
  1. Specify a **storage and depassivation procedure** for production: before or at first power-up, apply a controlled load pulse for a defined period to break down the film, and verify the recovered voltage as a test-station pass/fail.
  2. In firmware, on the first wake after a long idle, do a short **dummy load pulse** before the real transmission.
  3. Log the loaded voltage during TX (HW-025) so passivation is visible in the telemetry rather than being inferred after a failure.
  4. Confirm the exact procedure and timings against the cell manufacturer's application note — do not invent these numbers.
- Notes: The HLC/supercapacitor option under HW-009 largely sidesteps this problem, because the capacitor rather than the cell supplies the pulse. That is another point in its favour and another reason to measure before committing.

---



### HW-045 — Tank-wall penetration and in-tank connector for the sensor harness
- Severity: MAJOR
- Status: OPEN
- Component / net: J5 harness, tank wall/lid, ultrasonic enclosure
- Problem: The confirmed mechanical arrangement — Node outside the tank, ultrasonic inside — means a 4-wire cable must cross the tank wall or lid. Nothing in the design defines that penetration, the connector at the in-tank end, or the materials in contact with the water or its headspace. This is new information from v5 and had no issue before now.
- Impact: Three separate risks:
  1. **Water ingress and contamination.** A hole in a water tank is a leak path both ways — water out, and dirt, insects and light in. Light entering a tank promotes algae growth.
  2. **Potable water compliance.** If this tank supplies drinking water, every material in contact with the water or the headspace air — cable jacket, gland, potting compound, enclosure — must be rated for potable contact. This is a regulatory matter in most markets, not a preference.
  3. **The in-tank connector.** Any connector inside the headspace sits in saturated air. A JST-XH will corrode. The cable must either run unbroken into the potted sensor enclosure, or terminate in a genuinely IP68 connector.
- Recommended fix:
  - Use a proper **cable gland rated IP68** through the lid, with the correct cable diameter, and a drip loop below it on the inside so water runs off rather than tracking along the jacket.
  - Prefer a **single unbroken cable** from the Node enclosure to the potted sensor assembly — no connector inside the tank at all. Terminate only at the Node end, where it is dry and serviceable.
  - Specify a **potable-rated cable jacket and gland** if the tank is for drinking water, and record the certification in the BOM.
  - Where possible penetrate the **lid, not the wall** — a lid penetration is above the waterline and any leak is a drip, not a drain.
- Notes (v7, answered): The tank is **rated potable and filled with potable water**, though in practice it is used for washing rather than drinking because the water fouls over time. **Decision: specify potable-rated materials anyway** — cable jacket, gland, potting compound and enclosure. The reasoning is practical, not regulatory: potable-rated cable and glands are commodity parts with almost no cost delta, you cannot control what a future customer does with a tank that is *rated* potable, and specifying it once removes the question permanently instead of leaving it to be re-argued at production. Record the certification in the BOM (HW-033).
- Notes (v7): Your description of the tank fouling — surface going brown, sediment and growth building up because nobody cleans it — is directly relevant to the measurement and had not been captured anywhere. Raised as **HW-048**.
- Notes: The cable also runs within centimetres of a 433 MHz PA. Keep it away from the antenna, and see HW-012 — the ESD and series-resistor protection on D6/D7 matters more now that the run is longer and passes through a wall.

---

### HW-047 — 433 MHz channel plan, co-existence and multi-node collisions
- Severity: MAJOR
- Status: OPEN
- Component / net: Ra-02, antenna, system
- Problem: HW-006 removed the *regulatory* constraint on the band, but not the physical one. 433 MHz is a shared ISM band everywhere, and with no regulator assigning channels **you have to do the channel planning yourself**. Two distinct sources of interference: other equipment in the band, and your own Nodes interfering with each other.
- Impact: Lost packets look like a dead Node to the Hub, and every retry costs battery. In a dense deployment — several buildings, several Nodes each — self-interference becomes the dominant failure mode, and it gets worse the higher you set the PA.
- Recommended fix:
  1. **Avoid 433.92 MHz.** That is the default frequency for car key fobs, garage remotes, doorbells and weather stations across the region, and it is the busiest slice of the band. Pick something well clear of it — 434.4 MHz or 433.3 MHz — and confirm with a quick spectrum sweep at a real installation site before locking it in.
  2. **Randomise the wake offset per Node.** Nodes that power up together will transmit together forever, and two Nodes on the same channel transmitting simultaneously both fail. Add a per-Node pseudo-random jitter of ±5–10 s to the 120 s interval, seeded from the Node's serial number (HW-029). Costs nothing and removes the systematic collision.
  3. **Acknowledge every packet.** The Node needs to know whether the Hub heard it, both for retries and for the backoff in **HW-049**. Budget ~80 ms of RX at 10.8 mA per wake — that is 0.86 mA·s, about 8 % of the per-wake energy, and it takes the two-cell margin from 2.42× to 2.26×. Worth it.
  4. **Fix the antenna before raising the PA.** Going from +14 to +20 dBm is 6 dB and costs roughly 2.7× the transmit energy on every single packet, forever. Going from a whip lying against a metal tank to a properly mounted vertical whip with a ground plane, clear of the tank body, is easily 6–10 dB **and costs nothing**. Spend the link budget on the antenna first, then decide how much PA you actually need.
- Notes: Also relevant to the Stage 6 pairing protocol — proximity-gated pairing depends on RSSI, and a congested channel makes RSSI gating less reliable. Worth choosing the channel before designing the pairing handshake.
- Notes: If you eventually deploy many Nodes per Hub, consider putting different Hubs on different channels rather than relying on address filtering alone. Decide this before the first production run, because changing it later means touching every installed device.
- Notes (v14): **Link distance received — up to 50 m with thick concrete walls in between. This is now the highest-uncertainty item in the design**, because it sets the spreading factor, which drives the energy budget harder than TX power does (see the table in HW-003). Modelled scenarios, free-space at 50 m being 59 dB:

  | Scenario | Total path loss | Result |
  |---|---|---|
  | 2 walls × 12 dB | 83 dB | Closes at SF7 with ~44 dB spare |
  | 3 walls × 18 dB | 113 dB | Closes at SF7 with ~14 dB spare |
  | 4 walls × 25 dB | 159 dB | **Fails at every power and every SF, including +20 dBm at SF12** |

  If a real installation lands near the third row, no radio setting fixes it — it is a siting problem, and the answers are a better antenna, Hub relocation or a repeater. **This must be measured in a real building during Stage 5, before the design is frozen.**
- Notes (v14): **Put the antenna gain at the Hub, not the Node.** The Hub is mains powered, so a 3–5 dBi antenna there costs zero Node battery and adds margin in *both* directions — the Node's transmissions and its acknowledgements. Every dB gained at the Hub is a dB the Node does not have to pay for on every packet for two years. Do this before considering any increase in Node TX power.
- Notes (v14): 433 MHz was the right band choice for this link. Lower frequencies penetrate concrete considerably better than 868 MHz or 2.4 GHz would, so the earlier suggestion to consider 868 MHz (raised under HW-006 for EU compliance) is **withdrawn** now that Syria imposes no limit — stay at 433 MHz.

---

### HW-048 — Transducer and water-surface fouling in an uncleaned tank
- Severity: MAJOR
- Status: OPEN
- Component / net: RCWL-1670 transducers, mechanical
- Problem: You described the tank as fouling over time — the surface going brown, sediment and growth accumulating, and nobody ever cleaning it. That is the normal state of an unmaintained rooftop tank, and it has two direct consequences for an ultrasonic level sensor that nothing in the design accounts for.
- Impact:
  1. **Fouling on the transducer faces.** Dust entering the tank, condensation, and airborne growth will build a film on the two transducer faces over months. Ultrasonic transducers are very sensitive to anything on the radiating surface — a film attenuates both the transmitted pulse and the returning echo. The failure is gradual: range shortens, then readings become intermittent, then they stop. It reads as a flaky sensor, not as a dirty one.
  2. **Scum on the water surface.** A thick layer of surface scum or biofilm is acoustically soft — it absorbs rather than reflects. Depending on thickness you get a weak echo, no echo, or an echo from the top of the scum layer rather than the water itself. The last case is the dangerous one: a plausible-looking reading that is systematically wrong by the scum depth.
- Recommended fix:
  - **Make the sensor assembly serviceable without breaking the main enclosure seal.** The transducers will need wiping at some interval. Design the in-tank sensor as a separate potted module on its cable that can be unclipped, cleaned and refitted from the tank hatch. This is a mechanical requirement, and it needs to be decided now — retrofitting serviceability is not possible.
  - **Angle or shield the transducer faces** so falling dust and condensate do not settle directly on them. Facing straight down is the best case; a small shroud helps further.
  - **Detect degradation rather than waiting for failure.** Have the Node report echo quality alongside distance — pulse width, or the number of consecutive failed readings out of the sample set. A slow trend in that number is the fouling signal and gives the Hub something to raise a maintenance alert on, months before readings are lost.
  - **Reject implausible readings in firmware**: median of N samples, and discard readings that jump more than physically possible between 2-minute cycles.
- Notes (v8): **HW-030 has been raised to BLOCKER and now recommends a stilling well**, which collides head-on with this issue. Decide them together. The compromise is a 110 mm bore with generous bottom perforations, mounted so the whole well can be lifted out for cleaning — but that only works if the tank hatch is reachable, which is still the open question below.
- Notes: This interacts with **HW-030** — a stilling well would shield the beam from sidewall echoes but would also collect scum inside it, which could be worse than no well at all in this tank. Decide the two together once tank dimensions are known.
- Notes: **Question for you: is the tank hatch accessible for a person to reach the sensor?** If not, the serviceability requirement above changes into a "design for zero maintenance" requirement, which is a much harder problem and would push toward a non-contact alternative such as a pressure sensor at the tank base.

---

### HW-049 — Hub power outages waste Node battery and lose data
- Severity: MAJOR
- Status: OPEN
- Component / net: System — Node firmware, Hub power
- Problem: The Hub is mains powered. In Syria, extended and frequent grid outages are normal. Nothing in the current architecture says what the Node does when the Hub is not there — and with no acknowledgement in the design (**HW-047**), the Node cannot even tell.
- Impact: Two costs, one of which is large.
  1. **Battery.** A Node that keeps transmitting on schedule into a dead Hub spends full transmit energy for nothing. At 8 hours of outage per day, that is 240 wasted transmissions daily. Over two years it burns **~0.50 Ah — about 11 % of the whole 4.4 Ah pack** thrown away.
  2. **Data loss.** Every reading taken while the Hub is down is gone. For a system whose purpose is a continuous level record, an 8-hour hole every day is a serious gap.
- Recommended fix:
  1. **Acknowledge and back off.** Once acknowledgements exist (HW-047), a Node that misses several in a row should stretch its interval — 2 min → 10 min → 30 min — and return to normal on the first successful ack. Backing off to 30 minutes during outages recovers essentially all of that 0.50 Ah. This is the single highest-value firmware behaviour in the whole design after sleep current.
  2. **Buffer in RAM during the outage.** The Node stays powered in deep sleep, so SRAM survives. Roughly 1 KB of spare SRAM at ~8 bytes per reading holds about 125 readings — around 4 hours at the normal interval, or much longer once backed off. Send the backlog when the Hub returns. **Do not buffer to the ATmega's EEPROM**: at 720 writes a day its 100,000-cycle endurance is consumed in well under a year without wear levelling.
  3. **Give the Hub a battery backup.** The Hub is mains powered and has a screen, Wi-Fi and a database link — a small UPS or an internal cell keeps the whole system alive through an outage and makes points 1 and 2 rarely needed. This is a Hub-side change and belongs in the Hub design, but it is recorded here because the Node's battery budget depends on it.
- Notes: This also affects the pairing protocol in Stage 6 — a Node that cannot reach its Hub for a day must not conclude it has been unpaired and drop back to pairing mode.
- Notes: The backoff interacts with **HW-031**. Both point the same way: the fixed 2-minute interval is the most expensive assumption in the design, and making the interval adaptive — faster while filling, slower when idle, much slower when the Hub is unreachable — buys back more battery than any component change on this list.

---

### HW-050 — Metal tanks are worse than plastic on every axis, and both must be supported
- Severity: MAJOR
- Status: OPEN
- Component / net: System — antenna, enclosure, ultrasonic, grounding
- Problem: Installations include both plastic and metal tanks. The design currently makes no distinction, but a metal tank changes four separate things and makes each of them worse.
- Impact:
  1. **RF — this is about proximity, not enclosure.** To be clear, since it came up: the Node and its antenna are outside the tank, and nothing here assumes otherwise. The problem is that at 433 MHz the wavelength is 69 cm and a quarter-wave whip is 17 cm long, so a large metal surface within a fraction of a wavelength detunes the antenna and distorts its pattern — and a Node bracketed to a tank wall puts the whip a few centimetres from a metal sheet the size of a door. That easily costs more link margin than the entire difference between +14 and +20 dBm, which undercuts the "fix the antenna before raising the PA" point in **HW-047**. **Specify a minimum 17 cm (λ/4) standoff from any large metal surface**, or deliberately mount the antenna above the tank rim where the metal is behind it and acts as a ground plane rather than a detuning object.
  2. **Thermal.** Metal in Syrian sun runs far hotter than plastic and radiates into the headspace, producing a **larger vertical temperature gradient** — which is the dominant accuracy error in **HW-023**. A Node enclosure bracketed to a hot metal tank also conducts that heat straight into the electronics, worsening **HW-027**.
  3. **Acoustic.** The ultrasonic sensor *is* inside the tank, and a metal wall is a much better acoustic reflector than plastic, so sidewall and corner echoes are **stronger** in a metal tank. That mostly affects the last part of the range where HW-030 shows the beam reaching the wall — worst in a 500 L metal tank, where the wall is in the beam from 0.69 m onward.
  4. **Electrical.** A metal tank may be bonded to building earth, or floating. The Node uses low-side switching, so its load ground is a switched node, and the sensor cable runs from that node into the tank. A fault or a bonded tank creates a path around the MOSFET that the OFF-state analysis in `HYDRO-NODE-REFERENCE.md` §3 does not account for.
- Recommended fix:
  - **Mount the antenna ≥ 17 cm (λ/4 at 433 MHz) clear of the tank wall**, ideally on a short bracket that puts it above the tank rim rather than beside it. Make this a specified installation dimension, not installer judgement.
  - **Thermally isolate the Node enclosure from the tank** — standoffs or a non-metallic bracket rather than direct contact, plus the sun shield from HW-027.
  - **Use the two-sensor temperature arrangement from HW-023** as standard, not optional. On a metal tank the single-sensor gradient error is worse than the numbers in that issue assume.
  - **Keep the sensor cable galvanically simple**: the ESD/series-resistor protection from **HW-012** is now required, not advisable, and consider whether the in-tank sensor assembly should be fully isolated from the tank structure.
- Notes: If field data later shows metal tanks behave acceptably, this can be reduced to an installation-guide note. Until then, size the design for the metal-tank case, because it is the worst case on all four axes.

---

### HW-051 — Blind zone versus a 5 cm minimum measured distance
- Severity: MAJOR
- Status: OPEN
- Component / net: RCWL-1670, mechanical mounting
- Problem: The gap between the transducers and the highest water level is **5–15 cm**. The RCWL-1670 is advertised with a 2 cm minimum, which is plausible for a split TX/RX design — a separate receiver does not have to wait out the transmitter's ringdown the way a single-transducer module does. But that is a marketing figure for a low-cost module, and blind-zone claims of this kind are routinely optimistic. At the 5 cm end you have only 2.5× margin over the claim.
- Impact: If the real blind zone is 4–6 cm, a full tank returns no echo, a timeout, or a stuck value — **exactly when the reading matters most**, since "is it full" and "has filling finished" are the two questions the product exists to answer. The failure is also silent: a timeout looks like a sensor fault, and a stuck value looks like a working sensor.
- Recommended fix:
  1. **Measure the real blind zone before anything else.** Flat target, perpendicular, moved from 2 cm outward in 5 mm steps, 20 readings at each position, at room temperature and at 50 °C. Record the closest distance that returns a stable, correct reading. This is an afternoon on the bench and it is the single most informative test on the ultrasonic.
  2. **Then set the mounting standoff from the measured number, not the datasheet.** Specify the minimum sensor-to-full-water gap as at least **3× the measured blind zone**. If the measured blind zone is 4 cm, the 5 cm installations are not acceptable and the sensor needs a riser.
  3. A riser is a trivial mechanical fix — a short spacer or a deeper sensor enclosure lifts the transducers above the lid line. Decide it once, from data.
- Notes: This is now the top ultrasonic risk, ahead of HW-030. It is cheap to settle and everything downstream — mounting spec, enclosure depth, installation guide — depends on the answer.
- Notes: Near-field (Fresnel) effects are **not** a concern here. For a transducer of radius a at λ = 8.6 mm, the near field extends a²/λ, which is roughly 7–12 mm for a typical 16–20 mm transducer. Well inside 5 cm. The risk is ringdown and receiver recovery, not near-field.

---

### HW-052 — Split-transducer parallax adds a systematic error at close range
- Severity: MAJOR
- Status: OPEN
- Component / net: RCWL-1670, Hub-side maths
- Problem: The RCWL-1670 has **separate transmit and receive transducers, physically offset** by roughly 30–50 mm. The sound path is therefore a triangle, not a straight line down and back: the measured round trip is L = 2·√(d² + (s/2)²), where d is the true perpendicular distance and s the centre-to-centre transducer spacing. Any firmware that assumes distance = L/2 over-reads, and the error grows sharply as the target gets closer.

  | True distance | s = 30 mm | s = 40 mm | s = 50 mm |
  |---|---|---|---|
  | 50 mm | +2.2 mm (+4.4 %) | **+3.9 mm (+7.7 %)** | +5.9 mm (+11.8 %) |
  | 100 mm | +1.1 mm | +2.0 mm | +3.1 mm |
  | 150 mm | +0.7 mm | +1.3 mm | +2.1 mm |
  | 500 mm | +0.2 mm | +0.4 mm | +0.6 mm |
  | 1000 mm | +0.1 mm | +0.2 mm | +0.3 mm |

- Impact: A **systematic over-reading that is largest exactly when the tank is full** — the region this product cares about most. At the 5 cm minimum with 40 mm spacing it is nearly 4 mm, comparable to every other error term combined at that range, and unlike the others it is a fixed bias rather than noise, so averaging will not remove it. It also biases the "tank full" threshold in the direction of under-reporting fullness.
- Recommended fix: Correct it on the Hub, which is where all the maths lives (Section 2 of the project brief). Given the measured round-trip path length L and the transducer spacing s:

  **d = √( (L/2)² − (s/2)² )**

  Measure s once with callipers on a production module — centre to centre of the two transducer barrels — and treat it as a build constant. The correction is exact, costs one square root on the Hub, and disappears into rounding beyond about 300 mm.
- Notes: This is another reason the Node should transmit **raw echo microseconds** rather than a converted distance (see `HYDRO-NODE-REFERENCE.md` §5). With the raw time of flight on the Hub, the parallax correction, the speed-of-sound correction and the tank geometry are all applied in one place and can be revised over Wi-Fi without touching a sealed rooftop device.
- Notes: Verify the model empirically during the HW-051 blind-zone sweep — the same measurement run gives you the data to confirm the formula and to pin down s.

---

### HW-010 — Reverse polarity: the risk is a mis-crimped pack, not a reversed connector
- Severity: MINOR *(reduced from MAJOR in v10)*
- Status: OPEN
- Component / net: Battery connector, VBAT
- Problem: My original wording said the battery could be connected backwards. **You are right that it cannot be mis-mated** — the JST-XH housing is keyed and only enters one way, so an assembled pack cannot be plugged in reversed. That half of the issue is withdrawn. What survives is a different failure: the two wires being **crimped into the wrong positions in the housing** when the pack is assembled. The connector then mates perfectly, looks correct, and the polarity is reversed.
- Impact: Reduced. A reversed pack forward-biases the ESD structures in the CD4013/74HC74, the ATmega, the Ra-02 and the ultrasonic module simultaneously, and would reverse-bias C3 if it survives HW-009. But the failure is caught at first power-on — the device simply does not work — so the cost is a scrapped board at the factory, not a field failure. It is a yield problem, not a reliability problem.
- Recommended fix, cheapest first:
  1. **Free:** put a polarity convention in the pack assembly drawing (red always to housing position 1), and add a polarity check to the production functional test (**HW-029**). This catches it before the board is powered.
  2. **~$0.10:** a series **P-channel MOSFET ideal-diode** in the VBAT line. Roughly 20 mΩ, negligible quiescent current, and it does not eat the 0.2–0.3 V a Schottky would — which matters when the plateau is only 3.6 V and **HW-042** is already short of headroom.
  I would take option 1 for now and option 2 only if the respin has room, since a reversed pack is caught at test either way.
- Notes (v10): I checked whether the per-cell blocking diodes from **HW-003** would cover this. **They do not.** Those diodes sit in each cell's positive leg *inside* the pack, so with the whole pack reversed at the connector they are still forward-biased in the fault path. Worth stating explicitly so nobody assumes the HW-003 fix covers reverse polarity — it does not.

---

### HW-019 — Flow switch contact wetting current at 3.6 V
- Severity: MINOR *(reduced from MAJOR in v11 — I reviewed the wrong part)*
- Status: OPEN
- Component / net: WY-90 flow switch, J4, U2.D5
- Problem: **Correction first.** I based the original issue on the photo in `Components Images/71ZoYQ6sslL.jpg`, which shows an **HT-60 rated AC 220 V 0.5 A** — a mains pump-control switch. Your actual part is a **WY-90, DC 12–24 V**. That is a completely different and much better proposition, and the mains-contact argument is withdrawn. See HW-033 for the BOM consequence.
- Your question, answered: *"It's just a switch — when water flows the two wires connect. What has voltage got to do with it?"* A mechanical contact is not a perfect short, and that is the whole reason contact ratings exist:
  1. **Contacts only touch at a few microscopic points.** The apparent contact area is large; the real conducting area is a handful of asperities.
  2. **Those surfaces grow insulating films** — oxides, sulphides, adsorbed organics — a few nanometres to a few hundred nanometres thick. A closed contact can be mechanically closed and electrically open.
  3. **Voltage is what breaks the film down.** The effect is called *fritting*: enough field across a thin film punches a conductive channel through it. The threshold is roughly **0.3–0.5 V**, so your 3.6 V is comfortably sufficient. **Voltage is not your problem.**
  4. **Current is what maintains the connection.** After fritting, current has to melt and hold a metallic bridge at that channel. With the ATmega's internal pull-up (20–50 kΩ) you have only about **110 µA**, which is far below the current this switch was designed around. Too little current and the bridge is small, unstable and can re-oxidise.
  So the "DC 12~24 V" on the label is a statement about the **maximum load the contact can switch without arc damage** — it is not a minimum needed to work, and running at 3.6 V does not violate it. The spec that actually matters here is the **minimum switching current / dry-circuit capability**, which cheap switches do not publish.
- Impact: Much reduced. A WY-90-class in-line flow switch is almost always a spring-loaded magnet piston acting on a **hermetically sealed reed switch** — contacts in an inert atmosphere, usually rhodium or ruthenium plated. That construction is exactly what makes reed switches the standard choice for low-level signal switching, and sealed reeds routinely work at microamp levels. The residual risk is that you are still well below the part's designed operating point and cheap reeds vary batch to batch, so an occasional missed fill event over a 2-year life is possible.
- Recommended fix — cheap insurance, take it:
  - **Wetting pulse.** Spare GPIO → **330 Ω** → the flow-switch node. Before sampling, drive that GPIO high for ~5 ms: if the switch is closed, ~10 mA flows through the contact, which fritts and cleans any film. Then set the pin back to a hi-Z input and read D5 normally.
  - **Cost: 0.05 mA·s per wake out of ~10 mA·s — about 0.5 % of the energy budget**, and one resistor plus one pin (A0–A5 are free). It removes the whole class of problem for essentially nothing, whatever the contact turns out to be.
- Recommended fix — bench test to size the residual: let the switch sit unused for a week or two so any film can form, then run water and measure the node voltage with the real 30 kΩ pull-up at 3.6 V. Below ~0.4 V when flowing is a healthy contact. Repeat 50 cycles and count misses. Do this with and without the wetting pulse so you can see what it buys.
- Notes (v12, confirmed from a photo of the physical part): Label reads **WY-90 DC 12–24 V**, brand 万阳 (Wanyang), described as 水控自动开关 — "water-controlled automatic switch". Brass body, plastic cap, two flying leads (blue and black), bare-stripped.
- Notes (v12): **The switch is directional and this is an installation requirement, not a preference.** The cap is marked 水流方向 ("water flow direction") with a yellow arrow. Fitted backwards the piston will not lift and the contact will never close — and the failure is **silent and indistinguishable from "no water was used"**, so it would never be diagnosed from the data. Put the arrow orientation in the installation guide as a marked step, and add a commissioning check: run water and confirm the Hub sees the flow flag change state before the installer leaves the roof.
- Notes (v12): It is a **dry contact, so there is no polarity** — either wire to either connector pin. Worth stating explicitly in the assembly drawing so nobody wastes time on it. The leads are supplied bare-stripped and need proper crimped terminations into the JST housing, not tinned-and-stuffed copper; that belongs in the harness drawing under HW-033.
- Notes: **Still open, and I need these before firmware.** (1) Is the WY-90 **normally open** (closes on flow) or normally closed? (2) What is its minimum actuation flow rate, and is your tank fill rate above it? A flow switch that never trips because the fill is slower than its threshold is a silent FR-3 failure that no amount of contact conditioning fixes.
- Notes: **HW-020** is unaffected by this correction — the external pull-up, RC filter and series resistor are still needed, and the internal pull-up must still be disabled in sleep or it costs ~110 µA continuously while the switch is closed.

---

### HW-033 — BOM omissions
- Severity: MINOR
- Status: OPEN
- Component / net: BOM
- Problem: Comparing the BOM against the schematic and the assembly, the following are used but not listed: the **two 8-pin headers/sockets for the Ra-02** (J1, J2 — the largest single omission), the **battery holder or cell tabs** (the LS14500s in the photo are bare button-top cells with no tabs), a **DIP-14 socket** if one is used for U1, the **PCB itself**, cable glands, enclosure hardware and fasteners, conformal coating, desiccant, and the enclosure gasket.
- Impact: An incomplete BOM means an incomplete kit at the production line and unbudgeted cost.
- Recommended fix: Rebuild the BOM from the schematic's designator list rather than by hand, add a mechanical/consumables section, and add a **manufacturer part number and a lifecycle status** column for every line. Cross-check every designator appears exactly once.
- Notes (v11): **Third documentation mismatch found, and this one changed a review conclusion.** The BOM lists only "Water Flow switch" with no part number; the component photo shows an **HT-60, AC 220 V 0.5 A**; the part actually fitted is a **WY-90, DC 12–24 V**. Reviewing against the photo produced a wrong severity on HW-019. Together with the LED colour (blue vs red, HW-016), the R5 value (220 Ω vs 330 Ω) and the C3 rating (16 V vs 25 V), that is four places where the documents and the built article disagree. **A full reconciliation pass is now overdue** — every line needs a manufacturer part number, and every component photo needs to be of the part actually fitted, or the next review will make the same class of error.
- Notes: Also add: LoRa antenna, IPEX-to-SMA pigtail (listed), SMA bulkhead gasket, and the actuating magnet (listed, but with no grade or dimensions specified — see HW-015).
- **Update (v17) — the schematic can now generate most of a BOM, but six lines have no manufacturer part number** and will come out blank: **BATT, J1, J2, J3, C7, C8, C9, Q1**. The three electrolytics are the ones that matter, because **HW-058** says they should not be electrolytics at all — fill those lines in only after that decision is made, so the part number and the dielectric are chosen together.
- **Update (v17) — fifth documentation-versus-hardware mismatch, and the first inside a library part.** S1 carries `MDSM-4R-12-18`, a surface-mount reed, against a through-hole footprint and a through-hole part on the bench. Written up as **HW-059**. The pattern is now consistent enough to be a process problem rather than a series of slips: every part number in the schematic should be checked against the physical part before the BOM is released, not after. That check is cheap now and expensive once boards are ordered.

---


### HW-035 — Unused MCU I/O left floating will add sleep current if not configured
- Severity: MINOR
- Status: OPEN
- Component / net: U2 — A0–A7, D0, D1, DTR, and the unconnected Ra-02 DIO lines
- Problem: Nine analogue pins plus the UART pins are unconnected. A floating CMOS input sits near its switching threshold and its input stage draws crossbar current; several floating pins can add tens of microamps.
- Impact: Silent addition to sleep current — precisely the kind of thing that makes a measured power budget disagree with the calculated one.
- Recommended fix: Firmware must explicitly configure **every** unused pin before sleeping — either as an input with the internal pull-up enabled, or as an output driven low. Add this to the Stage 7 checklist and make it a measured pass/fail. It is a firmware fix, but it is recorded here because it is a power-path issue and it will be forgotten otherwise.
- Notes: In the respin, tie genuinely unused pins to ground through pads so the state is defined by hardware rather than by remembering.
- **Update (v25) — A0 is now one of these, and it is live on the bench right now.** R13 has been removed from the built board to clear **HW-067**, and A0 went nowhere else, so **A0 is a bare floating input**. A floating CMOS input drifts around its switching threshold and the input stage oscillates, drawing current continuously — which lands directly in the sleep-current measurement that is about to be taken.
- **Required before that measurement means anything:** `pinMode(A0, INPUT_PULLUP)` in `setup()`, or drive it as an output low. This is not optional housekeeping; it is the difference between measuring the circuit and measuring an oscillating pin.
- The full list for this board is A0 (now), A3, A4, A5, A6, A7, D0, D1, and the duplicate RST/TXO/RXI. Every one needs a defined state in `setup()` before the 25 µA target can be tested at all.
- **Update (v26) — the sleep current has now been measured at 100 µA (HW-070) with A0 still floating, so that measurement does not yet mean anything.** Widening this issue from "unused pins" to the full low-power teardown, because the pins are only one of five things the firmware has to switch off and they get done together or not at all. The complete sleep routine:

  ```
  // before sleeping
  SPI.end();                        // release SCK/MOSI/MISO
  pinMode(13, OUTPUT);              // D13 LED off for real — see HW-046
  digitalWrite(13, LOW);
  ADCSRA = 0;                       // ADC off: 200-300 uA if left on
  ACSR  |= (1 << ACD);              // analog comparator off: tens of uA
  power_all_disable();              // PRR: timers, TWI, USART
  // every unused pin gets a defined state
  for (uint8_t p : {A0, A3, A4, A5, A6, A7, 0, 1}) pinMode(p, INPUT_PULLUP);
  // then, and only then:
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli(); sleep_enable(); sleep_bod_disable(); sei();
  sleep_cpu();                      // BOD off is worth ~20 uA on its own
  sleep_disable();
  ```

  `sleep_bod_disable()` must sit inside the same interrupt-disabled window as `sleep_enable()` and be followed immediately by `sleep_cpu()` — the ATmega328P only holds the BOD-disable bit for four clock cycles, so anything between them loses it silently.
- Note that `INPUT_PULLUP` on an unused pin is correct only because the pin connects to nothing. **Never apply it to a pin that reaches the always-on latch domain** — that is exactly how HW-067 happened.

---

### HW-038 — Connector functions are not on the silkscreen; DS18B20 wire order is undocumented
- Severity: MINOR
- Status: OPEN
- Component / net: J3, J4, J5, battery connector
- Problem: The silkscreen shows reference designators (J4, J5, …) but not what plugs into them or what each pin does. The DS18B20 waterproof probe ships as three flying wires with no connector, so the crimp order is a build instruction that exists nowhere in the documentation.
- Impact: Assembly errors, and field-service errors. Combined with HW-011's interchangeable 2-pin connectors, this is how a battery ends up in the flow-switch socket.
- Recommended fix: Silkscreen every connector with its function and its pin-1 signal name — `TEMP  1:DATA 2:GND 3:VCC`, `FLOW`, `ULTRASONIC  1:GND 2:TRIG 3:ECHO 4:VCC`, `BATTERY +/−`. Add a wire-colour table to the assembly drawing (the DS18B20 probe's usual convention is red = VDD, black or blue = GND, yellow or white = DATA, but **verify it on your actual probes** — clone probes are not consistent, and a swapped VDD/DATA will destroy the sensor).
- Notes: Free to fix in the respin, and it prevents several of the more expensive mistakes on this list.

---

### HW-039 — Reed switch is a bare glass body with long unsupported leads
- Severity: MINOR
- Status: OPEN
- Component / net: S1
- Problem: The reed in the photo is a bare glass envelope (4 × 29 mm) with long thin leads. From the 3D view it stands vertically off the board with nothing supporting the body.
- Impact: Glass reeds crack from shock, and unsupported leads fatigue under vibration. This is the device's only user control and it is inside a sealed enclosure, so a failure is unrecoverable in the field.
- Recommended fix: At minimum, use a **plastic-encapsulated or moulded reed**, mount it lying flat against the board, and secure the body with a dab of adhesive. Better: replace it with a **micropower Hall-effect sensor** (for example a device sampling at ~20 Hz for around 1.5 µA average). Solid-state, tiny, no glass, no bounce, and a clean digital output — which also removes the debounce problem in HW-014 and makes the placement problem in HW-015 much easier. The cost is ~1.5 µA of continuous current, which is negligible against the 251 µA budget, and it only applies in the ON state.
- Notes: A latching Hall device would let you keep the toggle behaviour in the sensor itself. If you go with the "delete the CD4013" architecture from HW-021, an omnipolar Hall sensor driving an MCU interrupt is the natural pairing.

---

### HW-040 — Antenna: no RF keep-out, and the SMA bulkhead is an unmanaged sealing penetration
- Severity: MINOR
- Status: OPEN
- Component / net: Ra-02 IPEX connector, SMA pigtail, enclosure
- Problem: The PCB has no RF keep-out region around the module, and the antenna leaves the enclosure through an SMA bulkhead — a metal penetration in a sealed wall that nothing in the design specifies how to seal. There is also no defined antenna position relative to the tank, which for a metal tank matters a great deal.
- Impact: Reduced range (which, per HW-031, you cannot compensate for by raising the spreading factor), and a water-ingress path straight into the electronics.
- Recommended fix: Keep copper and metal away from the module's antenna feed area; specify a **gasketed/O-ring SMA bulkhead** with a defined torque, and add thread sealant to the assembly instructions. Define the antenna's mounting position and orientation in the installation guide — vertical, clear of the tank body, and if the tank is metal, mounted off it rather than against it. Verify the installed link budget on a real roof before locking the SF choice from HW-031.
- Notes: A cheaper and more reliable alternative for a sealed product is an **external antenna on a short pigtail mounted through a single gland**, or an internal antenna if the enclosure and the range allow it. Worth evaluating once HW-006 fixes the band.
- **Update (v24) — the counterpoise is the missing half of the antenna, and it is now the highest-value range item on the project.** A quarter-wave whip is only half an antenna; the other half is the metal it works against. λ at 433 MHz is **69 cm**, so a quarter wave is **17.3 cm** and a proper counterpoise wants a radius of roughly that. Screwed into a plastic wall with nothing behind it, the counterpoise becomes whatever the coax braid happens to be — which is why range on builds like this is not repeatable between two identical enclosures, and why **HW-047**'s link measurement would be measuring the wrong thing until this is fixed.
- **Requirement:** a conductive plate inside the enclosure wall at the antenna end, with the **SMA bulkhead bolted through it metal-to-metal** and bonded to the coax shield at that point. Centre it on the antenna and make it as large as the box allows.

  | Plate | Size in wavelengths | |
  |---|---|---|
  | 7 × 9 cm (the sheet on the bench) | **0.13 λ** | partial, but far better than nothing |
  | 17 cm radius | 0.25 λ | a proper counterpoise |

- This changes the enclosure design, so it belongs with **HW-027** and **HW-028** rather than being left to assembly. Note that the plate is also a sealing surface and a thermal path, and it must not foul the RF keep-out this issue already asks for.
- **Recorded so it is not lost:** the 7 × 9 cm copper sheet on the bench was originally intended as a ground plane under the perfboard. It is worth far more at the antenna. See the scope note added to **HW-004**.

---

### HW-044 — The LED cannot confirm a power-off, and carries no state information
- Severity: MINOR
- Status: OPEN
- Component / net: DS1, R5, U2.D8
- Problem: DS1's cathode is on GND_SW, so the LED only has power while the device is on. The instant the latch drops, the MCU loses power mid-indication. From the user's side, **"I just turned it off" and "it was already off" look identical** — in both cases nothing happens. The indicator can confirm a power-on but not a power-off, which is half of the control it is being relied on for. Separately, a single generic blink carries no information: the Node enters pairing mode automatically when it has no stored Hub (FR-7), and there is currently no way for an installer to tell a paired Node from an unpaired one without the Hub in hand.
- Impact: Weak user feedback on the only control the device has. Compounds HW-043 — the LED is now load-bearing for the on/off UX, not decorative.
- Recommended fix:
  - With the MCU sense line from HW-043, firmware can watch the magnet and blink a distinct "you are about to change state" pattern **while the magnet is still held**, i.e. before the latch drops. That gives a real off confirmation without needing the LED to survive the power cut.
  - Encode state in the startup pattern: for example a short double-blink for "paired, entering normal cycle" and a slow repeating blink for "unpaired, in pairing mode". Free, and it saves an installer a trip.
  - Consider also blinking a coarse battery-health indication at power-on, once HW-025's internal-bandgap measurement exists.
- Notes: Firmware-only once the HW-043 sense line is in place. Cost is negligible — the startup blink is about 1.7 µAh per power-on at ~2 mA for 3 s, and the device is switched roughly five times in two years.
- **Update (v17) — the indicator is now a buzzer, and this issue becomes a firmware specification.** The LED is gone; LS1 beeps through D7 and R15. That is the right call for commissioning — you hear it without looking at the box, which matters on a roof in sunlight. It changes what this issue is asking for:
  - The Node has **A0** reading the reed line and **A1** reading the latch state, so firmware knows both that a magnet was applied and which way the latch went. Everything needed is on the board.
  - **The patterns must be distinguishable, and the OFF pattern must sound before the latch drops.** A single beep for ON and a distinct double beep for OFF. If the firmware beeps *after* commanding shutdown, the ground is already gone and nothing sounds — so the OFF beep has to complete first, then A1 pulls `1~RD` low.
  - **Silence must be unambiguous.** If a magnet pass produces no sound at all, that has to mean "the device is dead", not "it turned off quietly". That is the whole point of this issue and it is a sequencing requirement, not a hardware one.
  - A **low-battery pattern** costs nothing once HW-025 telemetry exists.
- Closes when the beep patterns are written into the firmware specification, not before.

---

### HW-046 — Check the Pro Mini for a D13 LED; D13 is the LoRa SPI clock
- Severity: MINOR → **MAJOR (v26)**
- Status: **OPEN — question answered on the bench 2026-08-26: the LED is fitted and has not been removed**
- Component / net: U2 D13, J2 pin 5 (SCK)
- Problem: D13 on this design is **SCK for the Ra-02 SPI bus**. Many Arduino-compatible boards fit an LED plus series resistor on D13. If your module has one, it is across the SPI clock line.
- Impact: Two effects, neither fatal but both worth removing. The LED loads the clock edge and adds capacitance to the highest-frequency net on the board, and it draws current on every SPI transaction — roughly a milliamp during each clock high, throughout every transmission, several times per wake for the whole life of the product.
- Recommended fix: Inspect one of your modules. If a D13 LED is fitted, remove it along with the power LED and regulator you have already taken off (HW-002) — same rework step, no extra cost. If it is not fitted, close this issue.
- Notes: The genuine SparkFun Pro Mini is generally fitted with a power LED only, but clone modules vary between batches and this is worth two minutes with a magnifier. **Tell me what you find and I will close or action it.**
- **Update (v26) — answered: the LED is on the board and was not removed.** The power LED and the regulator came off (HW-002); the D13 LED did not. Reported as *"it's turned off anyway while in deep sleep"*, and that observation is where this issue stops being cosmetic.
- **"Off" is not the same as "drawing nothing", and the difference is invisible.** D13's state during sleep depends entirely on what the firmware left it as:

  | D13 left as | LED current | Visible? |
  |---|---|---|
  | `OUTPUT`, driven **LOW** | **0** | off — correct |
  | `INPUT` (no pull-up) | **0** | off — correct |
  | `INPUT_PULLUP` | (3.3 − 1.8) / (~35 kΩ internal + series R) ≈ **40 µA** | **glows too faintly to see in daylight — looks off** |
  | `OUTPUT`, driven **HIGH** | ~1.5 mA | obviously lit |

  The third row is the trap. The ATmega328P's internal pull-up is 20–50 kΩ, and when D13 is an input with the pull-up on, that pull-up **sources current out of the pin, through the LED, to ground**. Roughly 40 µA — which is a large fraction of the 100 µA measured in **HW-070**, and it produces a glow well below what the eye picks up against ambient light. "The LED is off" is an observation about brightness, not about current.
- Second path to the same place: the SPI library leaves SCK as an output. If `SPI.end()` is never called, or if the last SPI clock edge left SCK **high**, D13 sits high through sleep at ~1.5 mA. That would show as 1.5 mA on the meter rather than 100 µA, so it is not what is happening here — but it is the reason the SPI teardown belongs in the sleep routine.
- Recommended fix, now concrete and in priority order:
  1. **Firmware, free:** after the radio is put to sleep, `SPI.end();` then `pinMode(13, OUTPUT); digitalWrite(13, LOW);`. Never leave D13 as `INPUT_PULLUP`.
  2. **Hardware, permanent:** desolder the D13 LED (or its series resistor — easier, and it leaves the LED body in place). This also removes the capacitive load from the highest-frequency net on the board, which was the original reason this issue was raised.
  3. Do both. Rev B should specify a Pro Mini with the LED removed as part of the same rework step as HW-002.
- Severity raised MINOR → MAJOR because it is now a live candidate for a measured 4× overshoot on the sleep budget, not a theoretical milliamp during SPI transactions.

## RESOLVED / WON'T FIX

### HW-002 — Arduino Pro Mini on-board power LED and MIC5205 regulator are permanently powered  ✅ RESOLVED (v5)
- Original problem: The design feeds the battery straight into the Pro Mini's VCC pin, but the module's own always-on parts stayed in circuit — the power LED (~1–3 mA) and the MIC5205 LDO back-fed through its output pin (~50 µA). Sleep current was therefore ~2 mA instead of ~10–25 µA, giving roughly 88 days of battery life against a 2-year target.
- Resolution: **The power LED and the MIC5205 regulator are removed from every module, and this is already done on the current build.** Confirmed by you, 2026-08-19.
- Residual, tracked elsewhere — not reasons to reopen:
  - The per-unit rework burden at production volume is **HW-026** (put the ATmega328P directly on the board). Hand-reworking a hobby module on every unit is not a manufacturable process, and this resolution depends on it being done correctly every time.
  - The **BOD fuse** is a separate setting and still costs ~20 µA in sleep if left enabled. Disable it. Tracked under HW-035 and the Stage 7 checklist.
  - The claimed sleep current is still **unmeasured**. Stage 7 must confirm ~4.5–10 µA on a bench supply. If the measurement disagrees, that is a new finding, not a reopening of this one.
- Notes: See also **HW-046** — check whether your specific module also carries a D13 LED, because D13 is the LoRa SPI clock.

---

### HW-006 — LoRa band, region and legal radiated power are undefined  ✅ RESOLVED (v7)
- Original problem: The Ra-02 is a 433 MHz module rated +18 dBm, and nothing stated the deployment country or the intended output power. In ITU Region 1 the 433 MHz band is generally limited to 10 mW ERP, which +18 dBm (63 mW) would breach — making the legal power a hard input to the link budget and therefore to the battery budget.
- Resolution: **Deployment is Syria, and you have confirmed no enforced constraint on band or radiated power.** The regulatory input to the design is therefore removed: the Ra-02 may run at its full +18 dBm, and TX power becomes a pure engineering trade between link margin, battery life and self-interference rather than a compliance limit. Confirmed by you, 2026-08-19.
- What this unlocks: the v6 recommendation to cut TX power rested on two independent arguments — regulation and battery. Only the regulatory one disappears. At **+18 dBm with two isolated LS14500s the margin is 2.42×** (2.26× once per-packet acknowledgements are budgeted), so full power is affordable. **HW-003's recommendation is amended accordingly: keep the two-cell isolation, drop the mandatory power cut.**
- Residual, tracked elsewhere — not reasons to reopen:
  - Channel selection, co-existence with other 433 MHz users, and collisions between Hydro Nodes are **HW-047**. Interference is physics, not law, and it does not go away with the regulator.
  - Export beyond Syria would reintroduce compliance. Note it if the market ever widens; not a design constraint today.

---

---

### HW-008 — Ra-02 fed directly from a fresh Li-SOCl₂ cell, at the top of its rated supply range  ⛔ WON'T FIX (v10)
- Original problem: The Ra-02's specified operating range is 1.8–3.7 V, and a fresh unloaded LS14500 sits at ~3.6–3.67 V. The module therefore runs at the very top of its range with almost no margin, and there is no regulation.
- Decision: **Accepted as designed.** You tested a brand-new cell with a real module and it works. On review the risk is smaller than I first rated it: at ~3.65 V the module is *inside* the manufacturer's rated range, not outside it, so this was always a margin observation rather than a defect. The exposure window is also short — a Li-SOCl₂ cell only sits at its peak open-circuit voltage until it has delivered a little charge, then settles onto the ~3.6 V plateau for the rest of its life. The SX1278 silicon's absolute maximum is 3.9 V, so there is 250 mV before anything is at risk of damage.
- Residual risk, accepted:
  - The margin to the 3.7 V rated limit is roughly **50 mV**, so there is nothing left for cell-to-cell OCV spread or a future batch that runs slightly higher. If you change cell supplier, re-check the fresh OCV before assuming this still holds.
  - One module tested at one temperature is one sample. Since you will already have the setup for the **HW-042** droop measurement, record the *upper* rail voltage in the same run, at cold as well as hot — it costs nothing extra.
  - The rail is unregulated, so the PA sees a supply that moves with cell state and load. That affects output power stability rather than survival, and is tracked under **HW-047** where the link budget lives.
- Notes: I checked whether the per-cell blocking diodes from HW-003 would incidentally raise the margin here. They would — a Schottky drops ~50 mV even at microamp currents, putting the module at ~3.6 V instead of 3.65 V — so if HW-003 is fixed with diodes, this concern shrinks further as a side effect.

---

### HW-011 — Battery and flow-switch connectors are both 2-pin JST-XH and are interchangeable  ✅ RESOLVED (v10)
- Original problem: Two mechanically identical 2-pin JST-XH connectors on the same board. Plugging the battery into J4 would put 3.6 V onto D5 and onto the switched-ground net.
- Resolution: **The mechanical arrangement already prevents it, and you were right to push back — I raised this without knowing the enclosure layout.** The battery connector is **inside** the sealed enclosure and is only reachable by removing the four lid screws, while the three sensor connectors face **outward** through the enclosure wall. A user replacing the battery never sees a sensor connector, and an installer connecting sensors never sees the battery connector. Each connector will also be labelled with its function. Confirmed by you, 2026-08-19.
- Residual, handled as a production-test step rather than an open issue: during factory assembly, before the lid goes on, both the battery and the sensor connectors are briefly accessible on the same bare board. Put a polarity-and-position check in the functional test (**HW-029**) so a mis-plugged board is caught at test rather than at the customer. Physical separation on the PCB — battery connector well away from the sensor connector row — makes even that unlikely, and is worth doing in the respin at zero cost.


### HW-009 — C3 (2200 µF aluminium electrolytic) is the wrong part in the wrong place  ✅ RESOLVED (v16)
- Severity: MAJOR
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: C3, VBAT / GND_SW
- Problem: Three separate problems with this one part:
  1. **Leakage.** Aluminium electrolytics leak continuously and the leakage roughly doubles every 10 °C. A 2200 µF part at 3.6 V will leak on the order of 5–30 µA at 25 °C and can reach 50–200 µA at a 60 °C rooftop enclosure temperature. That is a permanent, unswitched drain on the 2-year budget, and it is the second-largest sleep-current item after HW-002.
  2. **Cold ESR.** Aluminium electrolytic ESR rises 5–20× at −10 to −20 °C. The cap is least effective exactly when the cell is weakest, which is the opposite of what you need.
  3. **It cannot do the job it was added for.** To hold a 120 mA TX burst within 0.2 V for even a short 60 ms SF7 packet you would need ~36,000 µF. 2200 µF holds that current for about 3.7 ms. **The battery supplies the TX current, not C3.** C3 only softens the first few milliseconds of the current edge.
- Impact: Threatens NFR-1 directly, and gives false confidence that the Li-SOCl₂ pulse problem is solved when it is not.
- Recommended fix: Remove C3 and replace the function properly:
  - **Local bulk:** 100–220 µF of **X5R/X7R ceramic** (e.g. 2× 100 µF 6.3 V 1210) placed at the Ra-02 supply pins. Near-zero leakage, stable ESR over temperature, no wear-out. Sized to cover the current edge, which is all a capacitor can do here.
  - **Pulse support for the cell:** if measurement shows the cell voltage dipping too far during TX, add a **hybrid layer capacitor (HLC) or a 0.1–0.5 F supercapacitor** charged through a current-limiting resistor. This is the standard Li-SOCl₂ + LoRa arrangement and it is the correct answer to the pulse problem. Budget the supercap's own leakage (typically 5–50 µA) into the power model before committing — it may cost more than it saves.
- Notes (v6): The arithmetic is now definitive, and it also answers the "one cell plus a big capacitor" proposal under HW-003. A 60 ms TX burst at 120 mA moves **7.2 mC**. Into 470 µF that is a 15 V collapse; into 2200 µF, 3.3 V; you would need **36,000 µF** to hold the rail within 0.2 V. No bulk capacitor you can fit in this enclosure carries a LoRa transmission. C3's only real function is smoothing the first few milliseconds of the current edge, and a few hundred µF of ceramic does that better than 2200 µF of leaky aluminium. Only a **supercapacitor** (0.1 F and up, low ESR, behind a series charge resistor) can actually carry the burst — see `HYDRO-NODE-REFERENCE.md` §10.3.
- Notes: First measure. Put a scope on VBAT during a real TX burst at −5 °C and at 50 °C, on a cell that has been sitting idle for a week (see HW-032). If the dip is acceptable, you may not need any pulse-support part at all and can delete C3 outright, which is the cheapest possible fix.
- **Resolution (v16):** the 2200 µF part is gone from the schematic. Bulk decoupling at the radio is now **C7 100 µF 10 V** and **C8 10 µF 50 V**, both sized to the SX1278 transmit burst rather than to a mains supply, and both drawn at the Ra-02 socket where the burst is drawn. Verified in `SCHEMATIC-CHECK.md` stage 13.

---

### HW-013 — Decoupling is wrong: only two bypass caps, neither local to any device  ✅ RESOLVED (v16)
- Severity: MAJOR
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: C4 (VBAT–GND_RAW), C5 (VBAT–GND_SW); C1 and C2 are not decoupling
- Problem: The BOM's four 100 nF caps are used as: C1 = reed debounce, C2 = power-on-reset coupling, C4 = raw-battery bypass, C5 = switched-rail bypass. So the entire board has **two** bypass capacitors, and from the PCB layout both sit at the far left, physically distant from the CD4013 (right edge), the Pro Mini (lower left) and the Ra-02 headers (top). **There is no decoupling at all at the Ra-02 supply pins** — the highest di/dt load on the board.
- Impact: Rail collapse and ringing during TX; unreliable SPI at the module; noise coupled into the echo timing. Compounds HW-004 and HW-009.
- Recommended fix: Add, placed within a few millimetres of the pin they serve:
  - Ra-02 3V3 (J1.3): 100 nF + 10 µF ceramic, with the return going straight into the pour.
  - Pro Mini VCC: 100 nF.
  - CD4013 VDD (pin 14): 100 nF — this one is genuinely absent today.
  - Ultrasonic supply at J5: 100 nF + 10 µF (it draws 6 mA in bursts down a cable).
- Notes: Ceramic X7R throughout; leakage is negligible so this costs nothing in the power budget.
- **Resolution (v16):** the schematic now carries **nine 100 nF**, one per device — C5 at the Pro Mini, C6 at the Ra-02, C2 at the ultrasonic connector, C10 at the 74HC74, C4 across BATT+/BATT-, C11 and C12 on the two latch nodes, C1 on the flow input, C3 general — plus C8 10 µF and C7 100 µF as bulk at the radio. Verified in `SCHEMATIC-CHECK.md`.
- **Condition carried into the PCB stage:** a schematic cannot express placement. Each of these is only worth fitting if it is laid out at the device it belongs to, with the shortest possible loop back to the ground pour. That requirement is now on the pre-PCB list in `SCHEMATIC-CHECK.md`.

---

### HW-014 — Reed latch has no series resistor and no effective debounce  ✅ RESOLVED (v16)
- Severity: MAJOR
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: S1, R3 (10 kΩ), C1 (100 nF), U1 pin 3 (CLOCK1)
- Problem: **The latch topology itself is correct** — I traced it and it is a proper toggle (see Notes). The implementation has two defects:
  1. **No series resistance between S1 and the C1/CLOCK1 node.** When the reed closes it dumps VBAT straight into C1 with only wiring resistance in the loop. Repeated hot-switching into a capacitor is the classic way to weld or pit reed contacts.
  2. **Debounce is inadequate.** Contact bounce, and the multiple make/break events you get when a magnet is swept past rather than placed, each produce a clean rising edge at CLOCK1 — and each rising edge toggles the flip-flop. An even number of toggles leaves the device in the state it started in.
- Impact: The user brings the magnet up, the LED does something ambiguous, and the device may be OFF when they think it is ON — on a roof, sealed, with no other feedback. Also a slow degradation path for S1.
- Recommended fix:
  - Add **R_series ≈ 10 kΩ** between S1 and the C1/R3 node. This limits contact current and gives a defined attack time constant.
  - Once you add that series resistor, **both** edges at CLOCK1 become slow (attack ≈ 1 ms, decay ≈ 1 ms). Feed the node through a **Schmitt-trigger buffer (74LVC1G17, non-inverting, Icc typically well under 1 µA)** into CLOCK1 rather than driving the flip-flop directly. This both cleans the bounce and guarantees a fast edge into the clock input.
  - Use a **non-inverting** buffer so the toggle still happens on magnet *approach*, not on removal.
- Notes (v4): The Schmitt buffer is **required again** — the v3 PRE/CLR proposal that would have removed it is withdrawn (see HW-043). This issue is now fully absorbed into **HW-043**, which specifies the buffer, the inverted reed connection and the new RC values. The contact-wear withdrawal below still stands.
- Notes (v3, superseded): **Largely superseded by HW-043.** If the latch moves from edge-triggered CLOCK to asynchronous PRE/CLR, contact bounce becomes harmless (each bounce simply re-asserts ON) and the Schmitt buffer is deleted. The series resistor is still worth fitting because it costs nothing, but the **contact-wear argument in this issue is withdrawn** — at roughly 5 operations over the product's life, hot-switching into C1 is not a wear concern. You were right to push back on that.
- Notes (v2): The Schmitt buffer requirement is now coupled to the U1 part choice — see **HW-041**. If U1 becomes an **SN74HCS74**, every input is already Schmitt-triggered with no transition-rate requirement, and the separate buffer is deleted (the series resistor and the RC are still required). If U1 stays **CD4013BE** or becomes a plain **74HC74**, the buffer is mandatory — and more so for the 74HC74, which is the least tolerant of slow edges of the three.
- Notes: **Verification of the latch as drawn, per your Section 6 item 6.** I traced it from the extracted netlist: U1 is wired as a T-flip-flop (D1 ← Q1̄, pin 5 ← pin 2), CLOCK1 ← reed node, Q1 (pin 1) → R2 1 kΩ → Q1 gate, R1 1 MΩ gate pulldown to the raw battery return. SET1, SET2, RESET2, CLOCK2, D2 are all correctly tied to VSS; the unused half's outputs are correctly left open. C2 (100 nF from VBAT) with R4 (100 kΩ to VSS) is a correct **active-high power-on-reset** giving ~10 ms — so the device powers up OFF when a cell is first fitted. **Toggle logic is correct and the OFF state is genuinely clean:** with Q1 off, every resistive path (R1, R3, R4) sits at 0 V across it, C2 and C4 are ceramic, and the only OFF-state current is the MOSFET's leakage plus the CD4013's quiescent (< 1 µA at 25 °C). I have no objection to the concept — only to the debounce, the contact protection, and the placement (HW-015).
- **Resolution (v16):** the reed no longer drives the clock pin directly. The schematic has **R12 100 Ω** in series with the reed, **R14 470 kΩ** pulling the node down and **C12 100 nF** across it, giving roughly **47 ms** of recovery — long enough to swallow contact bounce without a Schmitt buffer. Verified in `SCHEMATIC-CHECK.md` stage 7. The 470 kΩ also drops the magnet-held current from about 360 µA to about 7.7 µA.

---

### HW-016 — Blue LED has no forward-voltage headroom on a 3.0–3.6 V rail  ✅ RESOLVED (v16)
- Severity: MAJOR
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: DS1, R5, U2.D8
- Problem: The BOM specifies a **blue** 3 mm LED (confirmed in the component photos), while the schematic symbol is a red LED. A blue LED has a forward voltage of roughly 2.7–3.2 V. The rail is 3.6 V falling to ~3.0 V over the cell's life, and the ATmega output stage drops another ~0.2 V. Through the schematic's 330 Ω that gives ~2 mA when fresh and essentially nothing at end of life — the status indicator goes dark exactly when you most need to know the battery is low.
- Impact: The Node's only local feedback stops working. Compounds HW-014 (ambiguous on/off feedback).
- Recommended fix: Use a **red or yellow LED (Vf ≈ 1.8–2.1 V)**, which leaves 1.0–1.8 V across the resistor across the whole cell life. Size R5 for 2 mA (it only ever flashes briefly), so ~680 Ω–1 kΩ. Make the schematic, the BOM and the fitted part agree.
- Notes (v4): **This is no longer cosmetic.** The LED is now the confirmation mechanism for the magnet on/off control (HW-043, HW-044), so a blue LED going dark at ~3.0 V means the user-facing control loses its feedback exactly as the battery ages — the point at which someone is most likely to be on the roof investigating. Treat the change to a red or yellow part as required, not optional.
- Notes: There is also a **BOM/schematic value mismatch** here — the BOM lists a 220 Ω resistor, the schematic says 330 Ω. Whichever survives, one document is wrong today.
- **Resolution (v16):** the LED is gone from the schematic. Indication is now **LS1**, a CPT-1255C-090 piezo transducer on D7, which has no forward-voltage threshold to run out of. The headroom problem is deleted rather than mitigated.
- Note: the substitution brought its own problems, tracked as **HW-055**, and it does not by itself answer **HW-044**, which stays open.

---

### HW-018 — Echo is on D7 instead of D8, so hardware input capture is unavailable  ✅ RESOLVED (v16)
- Severity: MAJOR
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: J5 pin 3 (Echo) → U2.D7; U2.D8 → R5/DS1
- Problem: The ATmega328P's **Input Capture pin (ICP1) is PB0 = Arduino D8**, which this design uses for the status LED. Echo went to D7, an ordinary GPIO, so the echo pulse can only be measured in software (`pulseIn` or a pin-change interrupt).
- Impact: `pulseIn` at 8 MHz resolves to roughly ±4 µs, and a pin-change ISR adds 1–2 µs of latency jitter. That is only ±0.7 mm of distance error so it is not fatal — but ICP1 gives a hardware timestamp at 125 ns resolution with zero interrupt-latency jitter, it is completely free, and it lets the MCU sleep during the flight time instead of spinning in a blocking loop (which also saves awake energy). Given NFR-2 explicitly drives this design, leaving a free hardware timer on the table is not defensible.
- Recommended fix: **Swap them — Echo to D8, LED to D7.** One net change in the respin, no cost, no extra parts.
- Notes: This also means the ultrasonic measurement no longer blocks the CPU, which shortens the awake window and helps NFR-1.
- **Resolution (v16):** echo is on **D8**. `J3.3` → R1 100 Ω → `U1.JP7_2 (D8)`, so ICP1 is available for a hardware timestamp. D7 now carries the buzzer. Verified in `SCHEMATIC-CHECK.md` stage 10.

---

### HW-020 — Flow input D5 has no external pull-up, no filter and no series protection  ✅ RESOLVED (v16)
- Severity: MAJOR
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: J4 pin 1 → U2.D5
- Problem: D5 goes straight from the MCU to a connector and out on a cable to the fill pipe, with nothing on it. The design relies entirely on the ATmega's internal pull-up.
- Impact: Three separate problems. (a) If firmware leaves the internal pull-up enabled during sleep while the switch is closed, that is **~110 µA continuous** — on its own about half of the entire allowable average current for the 2-year target. (b) An unfiltered mechanical contact on a long cable will produce chatter and pick up noise. (c) No series protection (see HW-012).
- Recommended fix: Add an **external 1 MΩ pull-up** to the switched rail plus a **100 nF** cap to ground at the connector (giving a 100 ms RC filter, which is fine for a flow signal that changes on a timescale of seconds), and a **100 Ω** series resistor at the MCU pin. 1 MΩ costs 3.6 µA when the switch is closed, and the internal pull-up can then stay off permanently. Combine with the wetting-pulse scheme from HW-019 for the actual sampling.
- Notes: Firmware must still be explicit: internal pull-up **disabled** before sleeping, every time.
- **Resolution (v16):** the flow input now has all three. **R6 1 MΩ** pulls the node up to BATT+, **C1 100 nF** filters it to the switched ground, **R5 100 Ω** protects the D5 drive pin and **R3 330 Ω** feeds the A2 sense pin. Net N06 carries exactly those five nodes. Verified in `SCHEMATIC-CHECK.md` stage 12. The 330 Ω is also what pushes the wetting current through the contact for HW-019.

---

### HW-021 — The MCU cannot read the reed line and cannot command its own shutdown  ✅ RESOLVED (v16)
- Severity: MAJOR
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: U1 (CD4013), S1 node, U2
- Problem: The latch is entirely autonomous. The MCU has no connection to the reed node and no connection to the CD4013's RESET, so it cannot tell whether a magnet is present, cannot detect a magnet *gesture* (e.g. hold for 10 s), and cannot turn itself off.
- Impact: Blocks several things you will want later:
  - No magnet-based user input beyond raw on/off — which is what a local unpair needs (HW-022).
  - No firmware-commanded shutdown on critical battery, which for a Li-SOCl₂ cell matters (deep-discharging past the plateau is where cells get unhappy).
  - No way for firmware to log or report an unexpected power-cycle.
- Recommended fix: Two wires, both to currently-free pins:
  - **Reed sense:** reed node (after the Schmitt buffer from HW-014) → A0. Free.
  - **Firmware shutdown:** A1 → a small-signal diode (cathode at A1) or a resistor → CD4013 RESET1, so driving A1 high resets the latch and kills the load. Must not fight the POR network — put a 100 kΩ in series and check the RC interaction with C2/R4.
- Notes (v3): **DECIDED — the hardware latch stays.** You argued that a firmware-independent off is required on a sealed device, and that is the stronger position: a hung or crashed MCU can still be switched off with a magnet, which the MCU-sleep alternative cannot offer. The ~5 µA saving is not worth that. This issue therefore shrinks to just adding the two wires (reed sense + firmware shutdown), and those are now folded into **HW-043**. The alternative below is recorded for history only — do not implement it.
- Notes: Alternative architecture, **rejected in v3**, recorded for history: delete the CD4013, Q1, R1, R2 and C2 entirely. Make the MCU always powered, put it in power-down (~4.5 µA) as the "OFF" state, and switch the ultrasonic and LoRa rails with small load switches. The reed then just drives a wake interrupt. This removes a whole subsystem, gives firmware full control over the on/off state, and costs ~5 µA in storage (≈88 mAh over a year on shelf — under 2 % of the pack). The one thing you lose is a true zero-power OFF for long-term storage. **Your call — tell me which way you want to go and I will rework the affected issues.**
- **Resolution (v16):** both paths exist in the schematic. **A0** reads the reed node through **R13 100 Ω**, so the MCU can see the magnet. **A1** drives the flip-flop's `1~RD` through **R9 100 kΩ**, so the MCU can command its own shutdown. Against R11's 1 MΩ pull-up the 100 kΩ divider puts the reset node at roughly 0.3 V when A1 drives low — a valid HC low. Verified in `SCHEMATIC-CHECK.md` stages 6 and 7.
- The hardware latch is unchanged and the magnet still has final authority, which is what the v3 decision required.

---

### HW-034 — C3 voltage rating mismatch between BOM and supplied part  ✅ RESOLVED (v16)
- Severity: MINOR
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: C3
- Problem: The BOM and schematic both specify 2200 µF **16 V**; the component photo shows a 2200 µF **25 V** part. Electrically the 25 V part is fine, but the can diameter differs (typically 13 mm vs 10 mm), so it may not match the PCB footprint or the enclosure clearance.
- Impact: Fit problem at assembly; documentation does not describe the built article.
- Recommended fix: Moot if HW-009 is adopted and C3 is deleted or replaced with ceramics. If C3 survives in any form, make the BOM, schematic, footprint and purchased part agree, and specify the diameter and lead pitch explicitly.
- Notes: Flagged mainly because it is a symptom — the BOM and the built board have drifted apart in at least three places (this, HW-016's LED colour, and HW-016's resistor value). Worth a full reconciliation pass.
- **Resolution (v16):** closed with HW-009. The part whose rating did not match the BOM no longer exists in the design. Its replacements carry their ratings as schematic parameters — C7 is 100 µF / 10 V, C8 is 10 µF / 50 V — so the BOM is generated from the sheet rather than maintained beside it.

---

### HW-037 — CD4013BE in a plastic DIP  ✅ RESOLVED (v16)
- Severity: MINOR
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: U1
- Problem: The CD4013BE is a 14-pin plastic DIP. If it is socketed (as the through-hole build style suggests), the socket contacts oxidise over years in a humid, thermally-cycling outdoor enclosure, and vibration can back the part out.
- Impact: An intermittent contact in the power-latch IC means the device randomly turns off, or fails to turn on, in the field.
- Recommended fix: Use the **SOIC-14 version soldered directly** (CD4013BM or equivalent). If U1 survives the architecture decision under HW-021 at all, do not socket it. Confirm the quiescent current and the operating temperature range against your chosen vendor's datasheet — the family covers −55 °C to +125 °C and 3–18 V, which is comfortable here, but I want the specific part's Iq over temperature in the power model rather than an assumption.
- Notes: I also want to check one specification I could not retrieve during this review: most CD4013B datasheets state a **maximum clock input rise/fall time** (I believe around 15 µs at V_DD = 5 V, but treat that as unverified until you or I read the vendor datasheet). As drawn today the *active* rising edge at CLOCK1 is fast, so the spec is probably not violated — but the moment you add the series resistor from HW-014 it will be, which is exactly why HW-014 also calls for a Schmitt-trigger buffer. **Please confirm the number from your part's datasheet.**
- Notes (v2): Partly superseded by **HW-041**. The package question here (SOIC, not socketed DIP) stands regardless of which logic family wins.
- **Resolution (v16):** closed with HW-041. The CD4013BE is not in the schematic; U2 is a **74HC74N** in the same 14-pin DIP.

---

### HW-041 — Logic family for the power-latch flip-flop: CD4013BE vs 74HC74 vs SN74HCS74  ✅ RESOLVED (v16)
- Severity: MAJOR *(raised from MINOR in v3)*
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: U1
- Problem (v3, primary): **The CD4013B's 3 V minimum supply has no margin against the LoRa TX droop.** U1 is powered from the raw battery, upstream of the MOSFET, so it sees the full sag when the Ra-02 draws up to 120 mA — and it has no decoupling capacitor at all (the gap noted in HW-013). Fresh cells at ~0.1 Ω parallel impedance sag ~12 mV, which is nothing; an aged, passivated or cold pack at ~5 Ω sags ~600 mV, which on top of a 3.2 V end-of-life cell puts U1 at ~2.6 V — below its minimum. See **HW-042** for why the consequence is severe. Credit for this one goes to you, not to the v1 review.
- Problem (v1, secondary): The CD4013BE is being run at 3.0–3.6 V. That is inside its recommended 3–18 V supply range, but **outside every characterisation table in the datasheet** — CD4013B DC and AC parameters are specified at V_DD = 5 V, 10 V and 15 V only. So no timing, drive or threshold parameter is guaranteed at the voltage this product actually runs at. This is the same class of defect as HW-017 (the IRLZ44N driven at an uncharacterised V_GS): it will work on the bench, and nothing in the datasheet says it must.
- Impact: No guaranteed operating point for the part that decides whether the device is on or off. Low probability of failure, but it is not a defensible production position, and it is cheap to fix.
- Recommended fix, in order of preference:
  0. **74HC74 — CONFIRMED, part already on the bench (Toshiba TC74HC74AP, DIP-14).** Specified 2–6 V, so it doubles the droop margin against the CD4013BE and closes the characterisation gap. Under **HW-043** the latch no longer uses the CLOCK input at all, which removes the one area where the 74HC74 was the weaker part. **Fit this.** It is not sufficient on its own — HW-042 is still required.
  1. **SN74HCS74 (still the better production part, if a non-DIP package is acceptable).** A 74HC74 with **Schmitt-trigger action on every input** and, per TI, **no input signal transition-rate requirement**. Specified 2–5.5 V, so 3.0–3.6 V is squarely inside the characterised range. This single substitution fixes the uncharacterised-operating-point problem *and* removes the separate Schmitt buffer that HW-014 would otherwise require. Only drawback: TSSOP/SOIC only, no DIP — which is fine, because HW-026 and HW-037 want SMD anyway, but it means it cannot go on the breadboard today.
  2. **Keep the CD4013BE — rejected in v3.** Best quiescent-current specification of the three on paper and the most tolerant of slow edges, but the 3 V minimum against a measured-unknown droop is not a risk worth carrying when a 2 V part is already on the bench.
  Also required whichever part wins: a **100 nF decoupling capacitor directly across pins 14 and 7** (this is still missing — HW-013).
- Notes: **Do not fit a 74HCT74.** It is one letter different, physically identical, and will not work here: HCT has TTL input thresholds and needs V_CC = 4.5–5.5 V. Check the marking on the bench part before wiring anything.
- Notes (v4): With HW-043 withdrawn back to a clock-driven toggle, the 74HC74's weaker slow-edge tolerance **is relevant again** — but it is fully handled by the Schmitt buffer that HW-043 now mandates, which presents a fast edge to CLOCK regardless of the family. The part decision is unchanged: fit the 74HC74 for its 2 V minimum.
- Notes (v3): The v2 note that this "closes no blocker and is a production-quality item" was wrong — it understated the droop risk. Corrected above. The v1/v2 concern that the 74HC74 is less edge-tolerant than the CD4013BE was correct in itself but is now irrelevant, because HW-043 stops using the clock input. It is also moot if the architecture decision under HW-021 goes the "delete the flip-flop entirely" way — **settle HW-021 first**, because it may delete U1 rather than replace it.
- Notes: Pin-for-pin migration detail (CD4013BE → 74HC74/HCS74) is in `HYDRO-NODE-REFERENCE.md` section 7. The two parts are both 14-pin dual D flip-flops but the pinouts are **not** compatible, and the set/reset polarity is **inverted** (CD4013 SET/RESET are active HIGH; 74HC74 PRE/CLR are active LOW). It is not a socket swap.
- **Resolution (v16):** **U2 is a 74HC74N**, confirmed from the schematic's LibReference. The 2 V minimum supply is the reason, against the CD4013BE's 3 V — the margin that matters during a transmit burst on aged cells. The SN74HCS74 remains the preferred part if a respin ever needs it, but the 74HC74 is on the bench and closes the issue.

---

### HW-042 — Latch supply has no hold-up; a TX droop can switch the device off permanently in the field  ✅ RESOLVED (v16)
- Severity: MAJOR
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: U1 pin 14, VBAT
- Problem: U1 is powered directly from the raw battery with **no local decoupling and no hold-up**. During a LoRa TX burst the Ra-02 draws up to 120 mA, and the cell's internal impedance — which rises with age, with passivation and with falling temperature — drops the rail. If that dip takes U1 below its minimum supply, the flip-flop can lose state.
- Impact: **This is the highest-consequence failure mode on the list after the six blockers**, because it is self-latching and self-concealing. The sequence: the latch glitches → Q goes low → the MOSFET opens → the load disappears → the rail instantly recovers → **and the device stays off.** Nothing turns it back on. A Node dies silently on a roof and needs a physical site visit with a magnet. Worse, it is age- and temperature-correlated, so it will hit a whole fleet at roughly the same point in life and preferentially in cold weather. From the Hub's point of view it is indistinguishable from a Node that has gone out of range.
- Recommended fix: Give the latch its own protected rail. Two parts:
  - A **Schottky diode** (BAT54, 1N5819 or similar low-Vf part) from VBAT to U1 pin 14.
  - A **10 µF X7R ceramic** from U1 pin 14 to GND_RAW, physically at the IC.
  U1 draws well under a microamp, so over a 60 ms TX burst the hold-up cap sags by roughly 6 mV (ΔV = I·t/C). The diode blocks the droop propagating back into the latch. Forward drop at these currents is ~0.15 V, so even at a 3.0 V end-of-life cell the latch sees ~2.85 V — comfortably above the 74HC74's 2 V minimum (**HW-041**), and above the CD4013BE's 3 V minimum only marginally, which is a second reason to make the part change.
  This also finally provides the decoupling capacitor at U1 that **HW-013** identified as missing.
- Notes: **This issue is currently unquantified and must be measured before Stage 7 can sign off.** Scope VBAT at U1 pin 14 during a real +18 dBm transmission, on **cells that have been left idle for at least a week** so passivation is present, at room temperature and at the coldest temperature the product will see. Record the minimum. If the measured floor is below the fitted logic's minimum supply, **this escalates to BLOCKER**.
- Notes: The same droop also affects the ATmega328P (which needs ≥2.4 V at 8 MHz, and will reset if the BOD fuse is set to 2.7 V) and the Ra-02 (1.8 V minimum, so it is fine). The difference is that an MCU reset is recoverable — it reboots and carries on — whereas a lost latch state is not.
- **Resolution (v16):** the latch now has its own held-up supply. **D1 (1N5819)** feeds it from BATT+, **C9 10 µF** and **C10 100 nF** hold it through a transmit burst, and the 74HC74's VCC and its three asynchronous inputs sit on that rail rather than on BATT+. Verified in `SCHEMATIC-CHECK.md` stage 4.
- **Still to measure (HW-042 test, carried to Stage 17):** the voltage on that rail during a transmit after the cells have sat idle a week. Above 2.0 V passes. The circuit is right; the number is not yet taken.

---

### HW-043 — Reed input needs real debounce; magnet retains full on/off control  ✅ RESOLVED (v16)
- Severity: MAJOR
- Status: ✅ RESOLVED (v16) — closed by the 2026-08-22 schematic
- Component / net: S1, R3, C1, U1 CLOCK input, one new MCU net
- Problem: The reed input is filtered only by R3 × C1 = 10 kΩ × 100 nF = **1 ms**, which is the same order as reed contact bounce. In a **toggle** latch every surviving bounce edge inverts the state, so the outcome of a deliberate magnet gesture is not deterministic — roughly a coin flip on each actuation. The failure is not "someone waves a magnet by accident"; it is "the installer brings the magnet to switch the Node on, two bounce edges get through, and it stays off."
- Impact: Ambiguous on/off on a sealed rooftop device. Also, the present 10 kΩ pull-down means a magnet left resting on the enclosure draws **360 µA continuously** — on its own more than the entire 251 µA average budget for NFR-1.
- Recommended fix — **keep magnet control of both on and off** (see the v4 note below for why the earlier proposal was withdrawn). Two parts:
  - Insert an **inverting Schmitt-trigger buffer (74LVC1G14 or similar)** between the reed node and U1's CLOCK input. Select the specific part on quiescent current — 74LVC1G14 is typically well under a microamp but its datasheet maximum is around 10 µA over temperature, which is 4 % of the budget; an ultra-low-power alternative such as the 74AUP1G14 should be compared before committing. **Verify I_CC at 3.6 V from the vendor datasheet.**
  - **Invert the reed connection**: reed from the node to GND_RAW through **1 kΩ**, with a **1 MΩ** pull-up to the latch rail and **1 µF** on the node. Delete R3.
- Recommended fix — resulting behaviour: attack ≈ 1 ms, release ≈ 1 s. Bounce and magnet-sweep chatter are fully swallowed; two contacts would have to be **more than ~3 s apart** to register as two events, which is a deliberate visible double-tap rather than bounce. The buffer must be **inverting** because the node now goes low on magnet approach, and an inversion is what restores a rising edge at CLOCK — i.e. the device still toggles when the magnet arrives, not when it leaves.
- Recommended fix — standby current: the 1 MΩ pull-up drops the magnet-resting-on-enclosure current from **360 µA to 3.6 µA**, a 100× improvement, and removes that risk from the NFR-1 budget entirely.
- Recommended fix — one additional wire, **not** for power control: tap the **buffer output** to a spare MCU input (A0, through 100 Ω). Zero extra parts and no loading of the 1 MΩ node, since the buffer output is a clean logic level that is high for as long as the magnet is present. This gives firmware the magnet gesture it needs for local unpair (**HW-022**) and for the shutdown warning in **HW-044**. Optionally also wire an MCU line to U1's reset for **automatic low-battery shutdown only** (**HW-025**) — never as the user-facing off path.
- Notes (v4): **The v3 proposal — asynchronous PRE/CLR with firmware-only OFF — is WITHDRAWN, and you were right to challenge it.** It contained a contradiction: HW-021 rejects the MCU-sleep architecture precisely because a hung MCU must still be switchable off with a magnet, and then v3 proposed making OFF firmware-dependent, which reintroduces exactly that failure. Magnet control of both directions is the correct requirement and it stands.
- Notes (v4): Your LED indicator also changes the analysis in favour of the toggle. A blink-on-startup turns an ambiguous toggle into a **self-correcting** one — the installer sees no blink, taps again. That converts a silent failure into a visible retry and defuses most of the original concern. It is why this issue is now "add proper debounce" rather than "change the topology". The dependency it creates is covered in HW-016 and HW-044.
- Notes: A hardware-timed alternative was evaluated and **not recommended**: tap = ON via PRE, hold ~3 s = OFF via an RC timer into CLR, keeping both directions firmware-independent while making bounce structurally impossible. It works, but it needs a Schmitt inverter, a low-leakage timing capacitor, two resistors and a steering diode — about five extra parts and a tuning exercise — for a control that is used roughly five times in the product's life. The complexity does not earn its place. Recorded in `HYDRO-NODE-REFERENCE.md` §9 in case the field data later says otherwise.
- **Resolution (v16):** closed with HW-014, by the same three parts. The magnet keeps full on/off control — the reed still clocks the flip-flop directly and nothing about OFF depends on firmware, which is what the v4 rewrite required.

---

### HW-007 — Ra-02: only one of its four GND pins is connected  ✅ RESOLVED (v17)
- Severity: MAJOR
- Status: ✅ RESOLVED (v17) — 2026-08-22 schematic
- Component / net: `U3.1`, `U3.2`, `U3.16` unconnected; `U3.9` on the switched ground
- Problem: Extracted from the schematic netlist: the module's ground returns to the board through **one pin, `U3.9`**. The Ra-02 symbol has four GND pins — 1, 2, 9 and 16 — and three of them are left open. The module's 120 mA TX return current therefore takes one long trace.
- Impact: Supply droop and ground bounce at the RF module during TX, reduced TX power and RX sensitivity, and a large radiating current loop. Compounds HW-004 directly — one ground pin and no pour under it is the worst combination available.
- Recommended fix: Connect **all four** GND pins to the switched-ground pour, each with its own wire rather than daisy-chained. In the 2-layer layout, tie them to the pour with short wide connections and stitch around the module footprint.
- Notes: `U3.4 (RESET)` is connected via D9 and `U3.5 (DIO0)` via D2 — both correct in the current schematic. DIO1–DIO5 remain open; consider bringing DIO1 out to a spare pin, since some LoRa stacks use it for RX-timeout and CAD-done, which the pairing protocol (Stage 6) may want.
- Note on the count (v16): this issue originally said "three GND pins" based on the earlier schematic's connector symbols. The current Ra-02 symbol has four. Nothing about the defect changed — one is connected either way.
- **Resolution (v17):** all four GND pins are now on the switched-ground net — `U3.1`, `U3.2`, `U3.9`, `U3.16`. Verified in the 2026-08-22 schematic.
- **Carried to the PCB stage:** four wires on the schematic is not four return paths on the board. Each pad needs its own short, wide tie into the pour, with stitching vias around the module footprint. That is a layout requirement, not a schematic one.

---

### HW-036 — Carbon-film ½ W resistors throughout  ✅ RESOLVED (v17)
- Severity: MINOR
- Status: ✅ RESOLVED (v17) — 2026-08-22 schematic
- Component / net: R1–R6
- Problem: All six resistors are ½ W axial carbon film (±5 % typical, and a temperature coefficient in the −200 to −1000 ppm/°C region). They are also physically large for a board that should be moving to SMD.
- Impact: Low, in this circuit — none of these resistors is in a precision path. R6 (the 1-Wire pull-up) and R3/R4 (the RC networks) all tolerate ±5 % easily. The real cost is size and assembly method.
- Recommended fix: Move to **0603 1 % metal-film** in the respin, for placement by machine and for a defined tempco. Not urgent on its own — bundle it with HW-026.
- Notes: Recorded for completeness; this is the lowest-priority item on the list.
- **Resolution (v17):** every resistor in the schematic is a **Yageo MFR-25, metal film, 0.25 W, 1 %**, confirmed from the part parameters (`MFR-25FBF52-…`) and the `FP-MFR-25-MFG` footprint on all fifteen. Metal film also brings the temperature coefficient down to 100 ppm/°C, which matters for R14 and C12 setting the reed's ~47 ms recovery across a rooftop temperature swing.

---

### HW-053 — The DS18B20 probes have no supply: D3 → J2.3 is missing  ✅ RESOLVED (v17)
- Severity: BLOCKER
- Status: ✅ RESOLVED (v17) — 2026-08-22 schematic
- Component / net: `U1.JP7_7 (D3)` unconnected; `J2.3` on net N28
- Problem: `U1.JP7_7 (D3)` has no wire on it, and `J2.3` — the temperature connector's VCC pin — connects to exactly one thing: `R7.1`, the far end of the 4.7 kΩ pull-up. Build sheet step 14.10 calls for **D3 → the temperature connector's VCC pin**. That wire is not in the schematic.
- Impact: both DS18B20 probes sit with their VDD pin floating, held only through 4.7 kΩ back to their own data line. The DS18B20 datasheet allows VDD to be a supply of 3.0–5.5 V, or tied to GND for parasite power. Floating is neither, and the part's behaviour there is undefined. In practice: no temperature reading.
- Second-order impact: the temperature reading is not only a reported value, it is what corrects the ultrasonic distance for the speed of sound. Losing it costs roughly **0.18 % of distance per °C** of uncorrected error — on a 1 m range and a 30 °C swing between night and afternoon on a Syrian roof, that is a systematic error of several centimetres that moves with the time of day.
- Third-order impact: with VCC floating there is a current path from D4 → R4 → the probe's data pin into the sensor die. The probes are powered through their protection structures whenever D4 drives high. That is not a designed parasite-power connection and it is not safe to leave.
- Fix: **wire `U1.JP7_7 (D3)` to `J2.3`.** One net. D3 then switches probe power as the build sheet intends, so the probes draw nothing between readings.
- Notes: the connector's pin order is DATA · GND · VCC, the reverse of the build sheet's VCC · GND · DATA. That is harmless with a hand-made cable but the build-sheet label and the silkscreen (**HW-038**) must be corrected to match, or the cable gets built to the wrong drawing.
- **Resolution (v17):** fixed, though not the way the build sheet specified. `J2.3` is now tied to **BATT+** rather than switched from D3, and `R7` pulls the data line up to the same rail. The probes have a proper supply, their GND stays on the switched ground, so the ground switch still removes them completely when the device is off.
- **Cost of the change, so it is on the record:** the two probes now draw their standby current the whole time the device is *on*, instead of only during a reading. At the DS18B20's typical 750 nA standby that is about 1.5 µA for the pair, roughly 26 mAh over two years — **under 1 % of a 4.4 Ah pack**. Not worth another wire.
- **What was given up:** there is no longer a way to power-cycle a hung 1-Wire bus, and a probe cable that shorts VDD to GND is now fed straight from the battery through the pack fuse instead of being current-limited by an MCU pin. The second point belongs with **HW-012**.
- **Revision in progress (v17, same day):** you have decided to move the probe supply onto **D3** after all, which is the original build-sheet intent (steps 11.3 and 14.10) and the better answer. **Two wires move, not one:**

  | Wire | Checked schematic | Change to |
  |---|---|---|
  | `J2.3` — probe VCC | BATT+ | **D3** |
  | `R7` — top of the 4.7 kΩ pull-up | BATT+ | **`J2.3`** (so it follows D3) |

  Moving only `J2.3` achieves nothing: with `R7` still on BATT+, a low on D3 leaves the pull-up holding the data line at 3.6 V and the probes are fed through the internal protection diode on their data pin — half-powered, and out of spec. With both on D3, a low kills the supply and the pull-up together.

  **Current check:** two DS18B20 converting is about 1.5 mA each, plus roughly 0.7 mA of pull-up current while the bus is held low — **under 4 mA**, against the ATmega328P's 20 mA pin rating, for about 0.1–0.2 V of drop.

  **End-of-life caveat:** the DS18B20 needs at least 3.0 V. The Pro Mini runs straight off the pack, so the probes get VBAT minus the pin drop. At a 3.2 V pack that is about 3.0 V — on the limit. LS14500 holds 3.6 V nearly flat until almost empty, so this is only reached at the very end of the two years.

  **What it buys:** the ~1.5 µA of standby back, and a probe cable that shorts VDD to ground is limited by the MCU pin instead of being fed from the battery through the pack fuse — a genuine improvement for **HW-012**.

  **Firmware:** allow a few milliseconds after D3 goes high before addressing the probes.

---

### HW-054 — Ultrasonic connector pin order does not match the RCWL-1670 pad order  ⛔ WON'T FIX (v17)
- Severity: MAJOR
- Status: ⛔ WON'T FIX (v17) — 2026-08-22 schematic
- Component / net: `J3.2` on BATT+, `J3.4` on N27
- Problem: the RCWL-1670's four pads are printed, left to right, **`GND · RX · TX · +5V`**. J3 is wired GND · **+5V** · TX · **RX** — positions 2 and 4 are swapped relative to the module.
- Impact: with a straight-through 1:1 cable, **3.6 V lands on the module's RX input** while the module's supply pin is driven by an MCU output through a 100 Ω resistor. The module never powers up, and its RX pin is fed from a rail with no current limit beyond the cable.
- Why this is worth fixing in the schematic rather than in the cable: a crossed cable makes it work, and a crossed cable is exactly what **HW-001** already flags — an undrawn cross-over harness that nobody can verify at assembly and that fails silently when someone builds the obvious 1:1 version.
- Fix: **swap the nets on `J3.2` and `J3.4`.** J3 then reads `GND · RX · TX · +5V`, matching the module, matching the build sheet's tape label, and making a straight cable correct.
- Notes: check the module in hand before the swap. Several different boards carry the RCWL-1670 name with different pad orders; ours is the 4-pad UART version photographed in `Components Images/1-19.jpg`, printed `GND RX TX +5V`. Confirm the silkscreen on the actual module matches before committing the change.
- **WON'T FIX (v17) — your decision, 2026-08-22: the pin order is intentional.** J3 stays `GND · +5V · TX · RX`.
- What this locks in: the ultrasonic harness is now permanently a **cross-over cable**. Pins 2 and 4 must swap between the board and the module. A straight 1:1 cable will put 3.6 V on the module's RX pin and leave the module unpowered.
- That requirement moves to **HW-001**, which already covers the undrawn cross-over harness. It is no longer a schematic fault; it is a harness drawing that must exist before anyone builds a cable.
- Recorded so nobody 'fixes' it later: the swap is not a mistake and must not be corrected in a future revision without also changing the cable.

---

### HW-055 — Buzzer LS1 is driven straight off D7 with no series resistor  ✅ RESOLVED (v17)
- Severity: MAJOR
- Status: ✅ RESOLVED (v17) — 2026-08-22 schematic
- Component / net: N23 — `LS1.P` to `U1.JP7_3 (D7)`, direct
- Problem: the CPT-1255C-090 is an **externally driven** piezo transducer with an electrostatic capacity of **8,400–15,600 pF** (datasheet, at 120 Hz / 1 V). It is wired directly to a logic pin. A capacitor connected straight to an output draws its charging current at the switching edge, limited only by the pin's own output impedance — a peak well above the ATmega328P's **40 mA absolute maximum per pin**.
- Impact: the energy per edge is tiny (½CV² ≈ 65 nJ) so the pin is unlikely to fail quickly, but the current spike is real, it is repeated at 4 kHz for the whole beep, and it is injected into a supply that has no ground plane under it (**HW-004**) and a 433 MHz receiver a few centimetres away.
- Fix: **100 Ω in series between D7 and `LS1.P`.** At the rated 4 kHz the transducer's own impedance is around 3 kΩ, so 100 Ω costs no measurable loudness. This is the second 1 kΩ from the build sheet's buy list changing value — it was the LED resistor.
- Three further points on the substitution, which replaced the LED and its 1 kΩ:
  1. **Audibility.** Rated output is **70 dB at 10 cm at 3 Vp-p**. Driven single-ended from 3.3 V you get about that in free air. Sealed inside a rooftop enclosure you will get far less — it needs a **sound port**, which is another penetration to seal against **HW-027** and **HW-028**. A port that keeps water out will also cost several dB.
  2. **State, not events.** A beep says something happened. It cannot say whether the device is currently on, which is the whole substance of **HW-044** — that issue is *not* closed by this change.
  3. **Optional, if it turns out too quiet:** driving the transducer differentially from two pins gives 6.6 Vp-p instead of 3.3 and roughly +6 dB. Costs one more pin and one more 100 Ω.
- Question for you: **was dropping the LED deliberate, or did the buzzer replace it by accident?** If commissioning on a roof matters — and it does, since the magnet is the only control — a red LED and the buzzer together is a few cents and one pin.
- **Resolution (v17):** **R15, 100 Ω** is fitted between `U1.JP7_3 (D7)` and `LS1.P`. Peak edge current into the transducer's 8.4–15.6 nF is now limited to about 36 mA, inside the ATmega328P's 40 mA absolute maximum, and at the rated 4 kHz the 100 Ω is negligible against the transducer's own ~3 kΩ.
- **The LED-to-buzzer substitution is confirmed deliberate** (your call, 2026-08-22): a short beep after the magnet is what tells you the device turned on. That is a better commissioning signal than an LED you have to be looking at.
- Two things it does not settle, both still live: the enclosure needs a **sound port** (HW-027, HW-028), and the *pattern* has to distinguish on from off — see **HW-044**.
- Buy-list change: **7 × 100 Ω**, not 6. The second 1 kΩ from the original list is no longer needed.

---

### HW-056 — Four Pro Mini power and ground pins are left unconnected  ✅ RESOLVED (v17)
- Severity: MINOR
- Status: ✅ RESOLVED (v17) — 2026-08-22 schematic
- Component / net: `U1.JP1_4 (VCC)`, `U1.JP1_5 (GND)`, `U1.JP1_6 (GND)`, `U1.JP7_9 (GND_2)`
- Problem: the Pro Mini symbol exposes two VCC pins and four GND pins. Only `JP6_4 (VCC_1)` and `JP6_2 (GND_1)` are wired.
- Impact: nothing electrical — these are the same nets inside the module. Two things that do matter:
  1. Each unconnected GND pad is a **stitching point thrown away**. On a board that already has **HW-004** against it, every extra tie from the module's ground into the pour shortens a return path.
  2. Four unconnected-pin errors in the DRC report is where a real one hides. The report should be empty except for pins deliberately marked no-connect.
- Fix: wire `JP1_4` to BATT+ and `JP1_5`, `JP1_6`, `JP7_9` to the switched ground. Place a **No ERC** marker on every pin that is meant to stay open — A3–A7, RAW, DTR, TXO, RXI and the duplicate RST/TXO/RXI — so the DRC report goes to zero and stays meaningful.
- Not a fault, recorded so it is not "fixed" by mistake: `U1.JP6_3 (RST_1)` is correctly left open. The Pro Mini module carries its own reset pull-up.
- **Resolution (v17):** `JP1_4 (VCC)` is on BATT+, and `JP1_5`, `JP1_6`, `JP7_9 (GND_2)` are on the switched ground. Every power and ground pad on the module is now tied, which is four more stitching points into the pour against **HW-004**.
- Still worth doing at layout: put **No ERC** markers on the pins meant to stay open — A3–A7, RAW, DTR, TXO, RXI and the duplicate RST/TXO/RXI — so the DRC report goes to zero and a real error cannot hide in the noise.

---

### HW-004 — No ground plane and no copper pour anywhere on the PCB  ✅ RESOLVED (v18)
- Severity: BLOCKER
- Status: ✅ RESOLVED (v18) — closed by the 2026-08-24 PCB
- Component / net: Whole board
- Problem: I parsed the Altium PCB file directly. It contains **zero polygons, zero regions and zero vias**. All 233 copper tracks are 1.0 mm wide: 227 on the bottom layer, 6 on the top. Every net — including both ground nets and the RF module's supply and return — is a long, thin, point-to-point trace. There is no reference plane under the Ra-02, under the SPI bus, or under the ultrasonic echo line.
- Impact:
  - **RF:** the Ra-02 pulls up to 120 mA in TX bursts through a single narrow trace with no plane. Supply droop at the module degrades PA output and RX sensitivity, directly costing link margin (FR-4).
  - **Accuracy (NFR-2):** no return-current reference means the echo timing line shares its return with everything else. Any ground bounce during a transition shifts the echo edge.
  - **EMC:** an unplaned board with a 433 MHz PA and 300 mm of unshielded sensor cable is a radiator and a receiver. It will be hard to pass emissions testing and easy to upset.
- Recommended fix: Respin as a **4-layer board** (signal / GND / VBAT / signal) — at this board size the cost delta is small and it removes a whole class of problems at once. If you must stay 2-layer: pour solid GND_SW on the bottom, route signals on top, stitch the pour with vias every ~5 mm, keep an unbroken plane under the Ra-02 footprint, and widen VBAT and GND_SW to ≥1.5 mm. Board is currently ~90 × 68 mm, so there is plenty of room.
- Notes (v15): **Full mechanism and step-by-step implementation now in `HYDRO-NODE-REFERENCE.md` §11**, including the Altium menu paths. Headline number: a 100 mm ground trace with no plane beneath it is ~100 nH, which is **272 Ω at 433 MHz** — so the Ra-02's ground connection is presently 272 Ω at its own operating frequency, against 2.7–5.4 Ω with a plane and a short via. DC resistance is irrelevant here (48 mΩ, 5.8 mV at 120 mA); inductance and loop area are the whole issue.
- Notes (v15): Design-specific point that §11.5 covers — there are **two** ground nets. **GND_SW gets the main pour** (it returns every signal and every load); GND_RAW gets a small local pour around U1/S1/Q1 plus one short wide link from Q1's source to the battery connector, which means **placing Q1 adjacent to the battery connector**. Do not merge the two pours, or the master power switch is bypassed.
- Notes (v15): The routing is currently backwards for this — 227 tracks on Bottom, 6 on Top, and **zero vias**. Signals move to Top, pour goes on Bottom, and every ground pin needs its own via. Also §11.8: never cut long slots through the pour, because the return current detours around them and recreates the loop the plane was meant to remove.
- Notes: This respin is where HW-001, HW-007, HW-013, HW-015, HW-017, HW-018 and HW-029 should all be fixed together.
- **Resolution (v18):** the PCB now carries a **solid ground pour on the Top layer covering the whole 90 × 70 mm board**, on net GND, and all 20 GND pads sit inside it. The defect this issue was written about — no plane and no pour anywhere — no longer exists. Verified in `PCB-CHECK.md`.
- **Three residuals were split out rather than left buried here**, because each needs its own fix: **HW-062** (one via on the whole board, none on ground, so nothing stitches the top pour to the bottom-layer routing), **HW-063** (295 mm of top-layer routing cuts slots through the pour — the slotted-plane trap from reference §11), and **HW-064** (Remove Dead Copper is off).
- The pour is on **Top** and the routing is on **Bottom**, which is the right way round: every bottom-layer trace has continuous copper directly above it. That was the v15 recommendation and it was followed.
- **Scope note (v24) — where the 272 Ω figure does and does not apply.** This issue quantified a 100 mm ground trace as ~100 nH, i.e. **272 Ω at 433 MHz**. That number assumes **433 MHz current is flowing on the board ground**, which is true when a whip is soldered straight to the PCB. It is **not** the configuration being built: the antenna leaves the module on a **u.FL pigtail to an SMA bulkhead**, so the RF return is the **coax shield** and only a small common-mode component rides the board.
- What the board ground actually carries is the transmit **supply pulse**, and at those frequencies the same inductance is trivial:

  | Ground path | Inductance | Drop at 120 mA, 1 MHz | Same L at 433 MHz |
  |---|---|---|---|
  | 20 mm of wire | ~20 nH | 15 mV | 54 Ω |
  | 60 mm solder track | ~60 nH | 45 mV | 163 Ω |
  | long daisy chain | ~150 nH | 113 mV | 408 Ω |

  Millivolts out of 3600. The pour on the production PCB is still right and still worth having — it costs nothing and it fixes the common-mode component and the receive noise floor. But **an imperfect ground on the hand-built board is not what limits range**, and the honest consequence of the coax-fed configuration is that it mostly makes the radiation *pattern* less predictable rather than losing decibels outright.
- **Consequence for the hand-built prototype:** a separate copper sheet bonded by wires is **not** a ground plane. A plane works because the return flows in copper directly beneath the trace; a bonded sheet is joined at points, so the loop area is unchanged. It buys a low-inductance common reference and some shielding, nothing more. The sheet is worth far more as an antenna counterpoise — moved to **HW-040**.

---

### HW-015 — Reed switch is mounted in the centre of the PCB  ✅ RESOLVED (v18)
- Severity: MAJOR
- Status: ✅ RESOLVED (v18) — closed by the 2026-08-24 PCB
- Component / net: S1
- Problem: From the PCB layout, S1 sits vertically in the middle of a ~90 × 68 mm board, roughly 30 mm from the nearest edge. The magnet has to actuate it through a PETG-CF wall plus that 30 mm of standoff, and the user has no marked spot to place the magnet against.
- Impact: Unreliable or impossible actuation — the primary and only user control on a sealed device (FR-6, NFR-5). Magnet field falls off steeply; 30 mm of extra distance can easily be the difference between working and not.
- Recommended fix: Move S1 to a **board edge**, oriented along its sensitive axis, as close to the enclosure wall as clearance allows. Mark the spot on the outside of the enclosure (embossed target in the print). Then verify the actual pull-in distance with the production magnet and the production wall thickness, and specify a minimum magnet grade in the BOM.
- Notes: Also see HW-039 — the bare glass reed body is mechanically fragile. If you adopt the Hall-sensor alternative there, this placement problem gets easier because a Hall device is small and can sit right at the wall.
- **Resolution (v18):** S1 has moved from the middle of the board to the **bottom edge** — pads at y = 29.2 mm with the board edge at 25.5 mm. The magnet spot is now at the enclosure wall instead of buried in the middle of the layout. Verified in `PCB-CHECK.md`.
- Note for the enclosure drawing: the reed's **glass body sits between its two pads**, which are 35 mm apart at x = 53 mm and x = 87 mm. The sensitive part is therefore the middle of that span, near x = 70 mm — mark the magnet spot there, not at either pad.

---

### HW-060 — The Ra-02 footprint: 25.40 mm between pad rows  ✅ RESOLVED (v19) — not a fault
- Severity: was BLOCKER — withdrawn
- Status: ✅ RESOLVED (v19) — the footprint is correct; I had the wrong part
- Component / net: U3, footprint `RA-02_BREAKOUT_THT_2X8`
- Problem: the Ai-Thinker Ra-02 is a **16-pad module on a 2.0 mm pitch**, two rows of eight. The commonest generic "2×8 through-hole" footprints are drawn on the 2.54 mm pitch used by ordinary headers. The footprint name does not say which this one is, and it cannot be read out of the `.SchDoc` — the geometry lives in the PcbLib.
- Impact: if the footprint is 2.54 mm, the module does not fit, and the mistake is not visible until the boards and the stencil are already made. This is the single most expensive error available at this stage.
- Fix: before routing anything, open the footprint in the PCB library editor and check the pad pitch and the row-to-row spacing against the module in your hand, with callipers. Do the same check for `MODULE_ARDUINO_PRO_MINI` (should be 2.54 mm, 12 × 2 plus the end rows) and for the three JST XH headers (**2.50 mm**, not 2.54 mm — XH is a metric 2.5 mm series and the difference accumulates to 0.2 mm across a 4-way part).
- Notes: `BUILD-SHEET.md` stage 13 has the Ra-02 in sockets rather than soldered flat. If the production intent is sockets, the footprint must be the socket's pitch, and the socket must be one that accepts a 2.0 mm module. Settle this before the layout, not after.
- **CONFIRMED (v18) — measured from `U3`'s pads in `Hydro_Node_PCB.PcbDoc`:**

  | | Footprint on the board | Ai-Thinker Ra-02 |
  |---|---|---|
  | Pad rows | **25.40 mm apart** (1.000 inch) | module is **16 mm** wide |
  | Pitch along the row | **2.54 mm** (0.100 inch) | **2.0 mm** |
  | Row length, 8 pads | **17.78 mm** | module is **17 mm** long |

  The module is 17 × 16 × 3.2 mm. Its pad rows cannot be more than 16 mm apart because that is the whole width of the part, so **they cannot span 25.4 mm**. The pitch is wrong too: at 2.0 mm against 2.54 mm the error reaches 3.8 mm by the eighth pad. Ordered as drawn, the boards are scrap.
- **Question that decides the fix:** is the intention the **bare Ra-02** or an **Ra-02 breakout adapter** with 2.54 mm headers? The footprint is named `RA-02_BREAKOUT_THT_2X8`, so a breakout may well be what was meant.
  - Bare module → rebuild the footprint at 2.0 mm pitch, measured with callipers first.
  - Breakout board → measure the breakout. 25.4 mm between header rows is possible but unusual; most are narrower.
- Everything else on the PCB waits on this, because fixing it moves U3 and every track routed to it — see **HW-061** and **HW-062**.
- The other footprints were measured at the same time and are all correct: U2 DIP-14 at 7.62 mm rows, U1 Pro Mini at 15.24 mm rows and 2.54 mm pitch, Q1 TO-220 at 2.54 mm, J2 and J3 at **2.50 mm** (JST XH is metric, not 2.54), S1 at 35.00 mm between two pads.
- **RESOLUTION (v19) — I was wrong, and the footprint is right.** You are fitting the **Ra-02 breakout board**, not the bare 17 × 16 mm module. Confirmed from your own photo, `Components Images/images - 2026-08-01T183803.546.jpeg`: the Ra-02 shield can is soldered onto a larger carrier PCB with 8 through-holes along each edge and its own C1/C2. Measuring the photo — 8 holes per row, rows about **9.7 pitches apart** — gives 2.54 mm pitch and roughly 25 mm between rows, which is what the footprint has.
- **Where the error came from, so it does not repeat:** the footprint is named `RA-02_BREAKOUT_THT_2X8`, and "BREAKOUT" was the answer sitting in the name. I measured the footprint against the module's datasheet dimensions without first establishing which of the two parts was being fitted. The lesson is the same one as **HW-019** and **HW-059** — check which physical part is in play *before* measuring anything against a datasheet.
- Still worth one minute at the bench: put callipers across your breakout's two hole rows and confirm 25.4 mm. Everything else was measured off the file and is exact; this one number came off a photograph.

---


## CHANGELOG

| Version | Date | Change |
|---|---|---|
| v26 | 2026-08-26 | **R14 → 2.2 MΩ fitted and tested good; first sleep-current measurement taken — 100 µA against a 25 µA budget.** **HW-070 raised (MAJOR)** with the cost worked out: at 25 µA the two-year margin on the 4400 mAh pack is 2.43×, at 100 µA it is **1.41×**. That does not fail the target, it eats the headroom that has to absorb capacity loss at rooftop temperature, passivation (HW-032), HW-003's diode drops and whatever spreading factor HW-047 forces — so it is worth fixing but is not a blocker. **The measurement is not yet valid**, for two reasons recorded in the issue: A0 is still a bare floating input (HW-035), and a reading of 0.10 mA implies the sensor cables were unplugged. Ranked suspects with magnitudes: BOD left enabled in power-down ~20 µA, the D13 LED ~40 µA, ADC 200–300 µA if left on, analog comparator tens of µA, floating pins tens of µA, watchdog ~5 µA and required, two DS18B20 ~1.5 µA, 74HC74 <1 µA, C9 <1 µA; R6, R7, R9, R11 and R14 all sit at 0 µA when idle. A seven-step subtraction method replaces guessing, and a meter caveat is recorded — 0.10 mA on a milliamp range is **one count**, ~10 µA of resolution plus burden voltage, so it must be re-read on a µA range or across a shunt. **HW-046 answered and raised MINOR → MAJOR:** the D13 LED is fitted and was never removed. "Off" is a statement about brightness, not current — with D13 left as `INPUT_PULLUP` the internal 20–50 kΩ pull-up sources ~40 µA out through the LED, which glows below what the eye sees in daylight and is a large fraction of the measured 100 µA. Fix is `SPI.end()` then D13 as an output driven low, plus desoldering the LED in Rev B. **HW-071 raised (MAJOR):** J3.2 is on BATT+ and J3.1 on the switched ground, so the **ultrasonic module is fully powered for all 120 seconds between wakes** and for the life of the product — a ranging module with its own MCU and receiver chain idles in milliamps, which would exceed every other term in the power budget combined. Its idle current is **unverified** (datasheet hosts are blocked from this environment) and must be measured on a bench supply before the severity is settled; the fix is a P-channel high-side switch on the J3.2 feed driven from **D3**, which HW-053 left free. Recorded why it was missed: HW-053 asked exactly this question about the temperature probes and answered it correctly on 1.5 µA — nobody asked it about J3, where the answer is likely the opposite. **HW-035 widened** from "unused pins" to the full sleep teardown, with the actual code — `SPI.end()`, D13 low, `ADCSRA = 0`, `ACSR \|= (1<<ACD)`, `power_all_disable()`, every unused pin defined, then `sleep_bod_disable()` inside the same interrupt-disabled window as `sleep_enable()` because the 328P holds that bit for only four clock cycles. |
| v25 | 2026-08-24 | **Workshop list narrowed after three questions from the bench; two items withdrawn as unnecessary today.** **C9 removed from today's work.** Its 5 µA leakage figure is the datasheet maximum at the part's rated 50 V; sitting at 3.5 V on a 50 V part the real figure is normally well under 1 µA, and the other reason to change it — electrolytic dry-out at rooftop temperature (HW-058) — is a two-year problem that a prototype will never see. Correct order is to **measure sleep current first** and swap only if the number says so, which also makes the before-and-after meaningful. **R14 → 2.2 MΩ reclassified as optional.** It has nothing to do with the fault that was fixed; it only widens the chatter ignore-window from 57 ms to 265 ms, and since hand chatter runs over a few hundred milliseconds it will *reduce* the symptom rather than cure it — the cure is the Schmitt trigger in HW-069. Not worth sourcing a part for. **HW-035 updated with a live item: A0 is now a bare floating input**, because R13 was removed to clear HW-067 and A0 went nowhere else on the board. A floating CMOS input oscillates around its threshold and draws current continuously, which lands straight in the sleep-current measurement being taken today — so `pinMode(A0, INPUT_PULLUP)` is required *before* that measurement means anything. Recorded what removing R13 actually costs: nothing today, since no firmware feature uses A0; it was reserved for HW-022's magnet-hold gesture and returns in Rev B on the Schmitt's driven output. |
| v24 | 2026-08-24 | **Photos of the built board received; ground-plane scope corrected and the antenna counterpoise promoted.** **HW-004 scope note:** its 272 Ω figure assumes 433 MHz current on the board ground, which is true for a whip soldered to the PCB and **not** true for this build — the antenna leaves on a u.FL pigtail to an SMA bulkhead, so the RF return is the coax shield. What the board ground actually carries is the TX supply pulse, where 60 nH of solder track costs **45 mV out of 3600** at 1 MHz. The pour on the production PCB remains right and free, but an imperfect ground on the hand-built board is **not** what limits range; it mostly makes the radiation pattern unpredictable rather than losing dB. Also recorded that a copper sheet bonded by wires is **not** a ground plane — a plane works because the return flows directly beneath the trace, whereas a bonded sheet is joined at points and leaves the loop area unchanged. **HW-040 updated and promoted to the highest-value range item:** a quarter-wave whip is half an antenna and the counterpoise is the other half. λ = 69 cm, quarter wave 17.3 cm; an SMA bulkhead in a plastic wall with nothing behind it leaves the coax braid as the counterpoise, which is why range on such builds is not repeatable between identical boxes — and why HW-047's link measurement would be measuring the wrong thing until it is fixed. Requirement added: a plate inside the enclosure wall with the bulkhead bolted through it metal-to-metal and bonded to the coax shield. The bench's 7 × 9 cm sheet is 0.13 λ — partial but far better than absent, and worth much more there than under the board. Workshop file gained the full explanation plus two warnings from the photos: **clean the flux** before judging the 2.2 MΩ change, since residue conducts enough at that value to hold pin 3 up, and **do not key the transmitter with the u.FL empty**. |
| v23 | 2026-08-24 | **Workshop rework list issued (`WORKSHOP-TODAY.md`), and one earlier instruction corrected.** **HW-067's advice to raise R9 to 1 MΩ was wrong and is withdrawn** — R9 is how A1 pulls the flip-flop's active-low reset down against R11's 1 MΩ, so it must be the *stronger* side of that divider. At 1 MΩ the reset pin would only reach 1.75 V against a 1.05 V limit and the MCU could never command a shutdown; at the as-built 100 kΩ it reaches 0.32 V and works. **R9 stays at 100 kΩ.** The A1 path was never the same case as A0 — its off-state leak pushes the reset pin *up*, the inactive direction — so removing R13 is the whole fix. **HW-069's bench fix changed from C12 → 1 µF to R14 → 2.2 MΩ**, which leaves the clock's rising edge at 10 µs instead of stretching it to 100 µs, gives a 265 ms ignore-window against 57 ms, and drops the magnet-held current from 7.4 µA to 1.6 µA. Recorded that 2.2 MΩ works only because real HC parts leak nanoamps where the datasheet allows ±1 µA — which through 2.2 MΩ would lift the pin 2.2 V and reproduce HW-067 by another road — so it is a prototype fix and the **Schmitt trigger is now required for production, not optional**; 1 MΩ is the highest value defensible from the datasheet alone. Today's list also swaps C9 to ceramic before the sleep-current measurement, since an aluminium can on the always-live latch rail would inflate the reading by up to its 5 µA leakage spec against a 25 µA target, and adds a second 100 nF directly at U3 rather than reworking C6's placement. |
| v22 | 2026-08-24 | **The board switches on.** Lifting one leg of R13 dropped U2 pin 3 from 2.4 V to 0 V and the magnet now toggles the latch — **HW-067 confirmed on hardware**, and it was a design fault, not a build fault. Blockers back to 1: HW-003 alone. **HW-069 raised (MAJOR)** from the first thing observed on the working board: a *slow* magnet approach makes the reed chatter, because it parks the magnet at the distance where the field only just exceeds pull-in and hand tremor opens and closes the blades several times — every closure being one toggle. R14 × C12's 47 ms window was sized in HW-014 against **contact bounce** (hundreds of microseconds) and does that job; hand chatter is two orders of magnitude slower and walks straight through. **This makes the v16 decision to drop the Schmitt trigger wrong** — HW-014 originally required one, and the reasoning that replaced it covered bounce but not chatter. Fix in two stages: a one-part bench experiment (C12 → 1 µF for a 470 ms window, reverting if the slower clock edge upsets the flip-flop — flagged as a genuine experiment since every datasheet host is blocked from this environment), then a **74LVC1G14 Schmitt inverter** in Rev B, which also fixes HW-067 structurally by moving A0 onto the Schmitt's driven output where the Pro Mini's leak cannot reach it. Free mechanical fix recorded alongside: a pocket in the enclosure so the magnet seats in one defined position and cannot hover at the threshold. Consolidated Rev B change list written into `BRINGUP-DEBUG.md`. |
| v21 | 2026-08-24 | **Root cause of the bring-up failure found and confirmed on hardware. HW-067 raised MAJOR → BLOCKER.** Six measurements on the built board: the latch rail is 3.5 V, reset and set are both released, the reed works — but **U2 pin 3, the clock, sits at 2.4 V instead of 0 V**. The 74HC74 triggers on a rising edge, and at 2.4 V the input already reads high, so the magnet's move to 3.6 V produces **no edge**. Q never toggles, the gate never rises, the MOSFET never conducts; every reading follows from that one thing. **Source:** the Pro Mini's A0 reaches pin 3 through R13's **100 Ω** against R14's **470 kΩ** pull-down — 4,700× stronger — and when the device is off the Pro Mini has BATT+ on VCC with a floating ground, so it leaks out of A0. Working back from the measurement the internal path is ~235 kΩ to BATT+, giving 3.6 × 470/(470+235) = **2.40 V**, exactly what was read. **Fix: R13 → 1 MΩ, R14 → 100 kΩ, C12 → 470 nF** (keeps R14 × C12 at 47 ms so HW-014's debounce is unchanged), **and R9 → 1 MΩ** for the same reason on the reset line — the 3.2 V on pin 1 against a 3.5 V rail is that same leak, ~300 nA out through R9. Immediate workaround: lift one leg of R13. Firmware rule now mandatory: A0 and A1 are inputs with pull-ups disabled except while A1 deliberately commands a shutdown. **General lesson recorded:** low-side switching leaves the Pro Mini permanently half-powered when off, so every connection from an MCU pin into the always-on latch domain must be high-impedance enough to lose to that domain's own pull-up or pull-down — R13 was sized for pin protection without anyone asking what it does in the off state. Nothing is wrong with the soldering. |
| v20 | 2026-08-24 | **First hand-built board does not switch on — Stage 1 bring-up opened.** Symptom: the magnet never toggles the MOSFET, and bridging Drain to Source by hand powers the Pro Mini for about 3 seconds. The second half of that is **not a fault** — a meter in continuity mode cannot supply the radio's first transmit burst, so the rail collapses; bridging with wire instead holds it up. That observation is diagnostically valuable: everything downstream of the MOSFET works, so the fault is confined to six parts — D1, the latch rail, the 74HC74, R10, the gate and Q1. Added `BRINGUP-DEBUG.md`: a six-measurement binary search with the real pad positions out of the PCB file, plus the orientation checklist for the five parts that can be fitted backwards (D1, C9, U2, Q1, the reed) and the note that all voltages must be referenced to BATT−, not the switched ground. **Two new issues found by analysing the failure rather than the board: HW-067** — R9's 100 kΩ from the Pro Mini's A1 to the flip-flop's active-low reset is **ten times stronger** than R11's 1 MΩ pull-up, so a low on A1 holds the latch in reset permanently and the device can never be switched on with every connection correct; the same applies far more severely to R13's 100 Ω against R14's 470 kΩ on the clock line. Fix is a firmware rule (A0 and A1 are inputs unless deliberately driving) plus raising R9 to 1 MΩ in any respin. **HW-068** — R11's 1 MΩ against the 74HC74's worst-case ±1 µA input leakage leaves only about 20 mV of margin at V<sub>IH</sub>; 220 kΩ removes it. Prime suspects for the current failure, in order: D1 fitted backwards, Q1 rotated so Gate and Source are swapped, and HW-067. |
| v19 | 2026-08-24 | **Three corrections to v18, all of which reduce the work.** **HW-060 → RESOLVED, not a fault — I had the wrong part.** The board takes the **Ra-02 breakout**, not the bare 17 × 16 mm module; confirmed from the project photo, where the shield can sits on a carrier PCB with 8 through-holes per edge. Measuring that photo gives 2.54 mm pitch and ~25 mm rows, matching the footprint. The answer was in the footprint's own name — `RA-02_BREAKOUT_THT_2X8` — and I measured against a datasheet before establishing which part was being fitted, the same mistake as HW-019 and HW-059. **Blockers 2 → 1; HW-003 is now the only one.** **HW-061 MAJOR → MINOR and narrowed to C6 alone.** v18 flagged C6, C7, C8 and C9; running the numbers, only C6 is degraded — its 24 mm loop is ~20 nH, dropping the 100 nF's self-resonance to 3.6 MHz against 7.1 MHz at 3 mm. C7 and C8 are bulk caps supplying 120 mA over milliseconds, where 12 mm of trace is 12 mΩ and 1.4 mV; C9 feeds a part drawing microamps and sags 5 mV over a 5 ms burst. The v18 claim that C9's placement re-opens HW-042 is **withdrawn**. **HW-062 MAJOR → MINOR** — the v18 entry undercounted what exists: the board is entirely through-hole, so its **20 ground pads are already 20 layer-to-layer ties**. Stitching is still worth doing for the slots and for a ring around U3, but after HW-063 rather than before. **HW-063 is now the first PCB job** and the only remaining MAJOR on the layout — 295 mm of top-layer routing slotting the pour, 75 mm of it BATT+. Noted that moving those traces needs no new vias on an all-through-hole board. Added `PCB-FIXES.md`: the loop-and-return-current explanation, the arithmetic behind each call, and step-by-step Altium instructions. |
| v18 | 2026-08-24 | **PCB received and checked. Routing is clean; one footprint is not.** Geometry read straight from `Hydro_Node_PCB.PcbDoc`: **every net is fully connected**, minimum different-net clearance is **0.351 mm**, track widths are 0.3 and 0.5 mm, annular ring is ≥ 0.25 mm everywhere but one boss pad, there are no component collisions and no duplicate designators. **HW-004 CLOSED** — there is now a solid GND pour on Top covering the whole 90 × 70 mm board, with all 20 GND pads inside it, and the pour is on the opposite layer from the routing, which is the right way round. **HW-015 CLOSED** — S1 moved from the middle of the board to the bottom edge. **HW-060 escalated MAJOR → BLOCKER and confirmed**: U3's pad rows are **25.40 mm** apart at **2.54 mm** pitch, against a module that is **17 × 16 mm** at **2.0 mm** pitch — the part cannot physically span it, so the boards would be scrap. Every other footprint measured correct, including J2/J3 at 2.50 mm (JST XH is metric). **Six new issues, all from the layout rather than the netlist: HW-061** — the radio's C6/C7/C8 sit 8.3–12.2 mm from U3 and the latch's C9 sits 15.0 mm from U2, which is HW-013's carried placement condition failing and re-opens HW-042 by the back door; **HW-062** — exactly **one via** on the whole board and it is on BATT+, so nothing stitches the top pour to the bottom-layer routing; **HW-063** — **295 mm** of top-layer routing cuts slots through the pour, 75 mm of it BATT+, the slotted-plane trap from reference §11; **HW-064** — Remove Dead Copper is off; **HW-065** — J1's boss pad has a zero annular ring and two stored DRC violations put silkscreen across S1's pads; **HW-066** — the M3 mounting holes leave 1.0 mm of board between hole and edge. Also confirmed the D3 → J2.3 change is on the board (NetJ2_3 = U1 + R7 + J2). Blockers unchanged at 2: HW-003 and now HW-060 in place of HW-004. |
| v17 | 2026-08-22 | **Second schematic pass — connectivity is now signed off.** All 34 build-sheet connections verified correct; the remaining problems are library and part-family, not wiring. **Closed: HW-055** (R15 100 Ω fitted between D7 and the buzzer), **HW-007** (all four Ra-02 GND pins now tied), **HW-056** (all Pro Mini power and ground pads tied), **HW-053** (probe supply fixed by tying J2.3 to BATT+ rather than switching it from D3 — costs about 1.5 µA of standby, roughly 26 mAh over two years, under 1 % of the pack; D3 is now spare), and **HW-036** (every resistor is a Yageo MFR-25 metal film 0.25 W 1 %). **HW-054 → WON'T FIX** — the ultrasonic pin order is intentional; that permanently makes the harness a cross-over cable, and the requirement moves to HW-001. The LED-to-buzzer substitution is confirmed deliberate. **Three new issues, all found by checking the library rather than the netlist: HW-058** — C7, C8 and C9 are aluminium electrolytics rated 105 °C / 4000 h, which at HW-027's 70–85 °C internal temperature is 1.8–3.6 years, inside the two-year target; C9 also leaks on the always-live latch rail that the magnet cannot switch off. **HW-059** — S1's part number `MDSM-4R-12-18` is a Littelfuse *surface-mount* reed while its footprint is through-hole; the BOM would order the wrong part. **HW-060** — the Ra-02 footprint pitch must be measured against the module before layout, since the Ra-02 is a 2.0 mm pitch part and generic 2×8 through-hole footprints are 2.54 mm. Also confirmed every one of the 38 components has a current PCB footprint assigned, no duplicate designators, no orphan junction dots, and no missing connections. Blockers 3 → 2. |
| v16 | 2026-08-22 | **Schematic received and checked line by line against `BUILD-SHEET.md`; full result in `SCHEMATIC-CHECK.md`.** 29 of 34 connections correct. Five faults found: **HW-053 (BLOCKER)** — `D3 → J2.3` is missing, so both DS18B20 probes have no supply and their VDD floats, which also loses the speed-of-sound correction on the ultrasonic reading; **HW-054** — the ultrasonic connector is wired GND·+5V·TX·RX against the module's GND·RX·TX·+5V, so a straight cable puts 3.6 V on the module's RX and leaves it unpowered; **HW-055** — the buzzer that replaced the LED sits directly on D7 with no series resistor, and its 8.4–15.6 nF draws an edge current above the pin's 40 mA absolute maximum; **HW-056** — four Pro Mini power and ground pads left open, losing ground stitching against HW-004; **HW-057** — a stray second reed pin pair parked off-sheet at (1060, −180). **HW-007 confirmed still open** — the symbol has four GND pins and three are unconnected, not three with two as originally written. **Twelve issues closed by the new schematic:** HW-009 and HW-034 (2200 µF gone, replaced by 100 µF/10 µF sized to the TX burst), HW-013 (nine local 100 nF — placement condition carried to the PCB stage), HW-014 and HW-043 (100 Ω + 470 kΩ + 100 nF giving ~47 ms recovery), HW-016 (LED removed), HW-018 (echo moved to D8, ICP1 available), HW-020 (flow input pulled up, filtered and protected), HW-021 (A0 reads the reed, A1 drives ~RD through 100 kΩ), HW-037 and HW-041 (74HC74N fitted), HW-042 (D1 + 10 µF + 100 nF holding the latch rail — measurement still owed). Also fixed a real bug in `tools/extract_netlist.py`: it treated both ends of a pin as electrical, which falsely shorted C1 and R7 across their own pins. The connecting end is `Location + PinLength × direction`; the tool now uses it and self-checks that no two-pin part is shorted across itself. Blockers 2 → 3. |
| v15 | 2026-08-19 | HW-004 expanded with the full mechanism and implementation detail — reference §11. Quantified the actual defect: a 100 mm ground trace is ~100 nH, i.e. **272 Ω at 433 MHz**, against 2.7–5.4 Ω with a plane and a via; DC resistance is a red herring at 5.8 mV. Recorded the design-specific split — GND_SW takes the main pour, GND_RAW takes a small local pour with Q1 placed next to the battery connector — plus the routing flip (signals to Top, currently 227 tracks on Bottom with zero vias) and the slotted-plane trap. No severity or count change. |
| v14 | 2026-08-19 | Link distance received — 50 m through thick concrete. **HW-003 DECIDED: two LS14500 in parallel, isolated with one `1N5819` per cell, 0.5 A fuse in the pack lead, supplied as a sealed non-serviceable pack; 2-year target kept.** The v13 single-cell recommendation is withdrawn: at 50 m through concrete the required spreading factor is uncertain, one cell only survives SF7 (SF9 gives 1.02×), and two cells cover every plausible outcome — and two cells is the *reversible* choice, since an easy measured link means simply fitting one, whereas the reverse needs a respin. HW-047 updated with the three modelled link scenarios; the worst (4 walls × 25 dB = 159 dB) fails at +20 dBm and SF12, which would be a siting problem not a radio one, so Stage 5 must measure it in a real building. Also recorded: put antenna gain at the mains-powered Hub where it costs no Node battery and helps both directions, and stay at 433 MHz — the earlier 868 MHz suggestion is withdrawn now that concrete penetration matters more than EU compliance. |
| v13 | 2026-08-19 | HW-003 recommended fix reordered. **A single LS14500 at +14 dBm on a 1.5-year target gives a 2.01× margin and a ~45 mA peak that is inside the cell's 50 mA continuous rating** — so relaxing the target enables a one-cell design, which deletes the paralleling hazard outright rather than mitigating it. Now the recommendation. Recorded that relaxing the target does **not** work as a safety mitigation on its own, since the cells still deplete on their own schedule. Risk reframed more fairly: matched cells discharged together diverge modestly; the realistic hazard is a user replacing one cell of two, which a sealed non-serviceable pack prevents for free. Added concrete searchable part numbers (1N5819, SS14, PMEG2010AEH, LM66100, MAX40200, LTC4412) and corrected the earlier "PTC or fuse" advice — a PTC's 0.9–1.2 Ω costs the same headroom as a diode, so use a plain 0.5 A fuse. |
| v12 | 2026-08-19 | HW-019 updated from a photo of the physical part. Confirmed WY-90 DC 12–24 V, 万阳 brand, two-wire dry contact. **Recorded the flow-direction arrow as an installation requirement** — fitted backwards the switch never closes, and the failure is silent and indistinguishable from "no water was used", so it needs a marked step in the installation guide and a commissioning check. Also recorded that the contact has no polarity and that the bare-stripped leads need crimped terminations. No severity change; no new issues. |
| v11 | 2026-08-19 | **HW-019 MAJOR → MINOR and rewritten** — the original review was based on the HT-60 (AC 220 V) in the component photos; the part actually fitted is a **WY-90, DC 12–24 V**, almost certainly a sealed reed, so the mains-contact argument is withdrawn. Added the contact-physics explanation: voltage fritts the surface film at 0.3–0.5 V so 3.6 V is fine, but the ~110 µA from the internal pull-up is far below the part's designed current — the label's "12~24 V" is a maximum switching rating, not a minimum. Wetting pulse retained as cheap insurance at 0.5 % of the energy budget. HW-033 updated — this is the fourth documentation-vs-hardware mismatch and the first to have caused a wrong severity, so a full BOM reconciliation is now overdue. |
| v10 | 2026-08-19 | **HW-008 → WON'T FIX** — tested working on a fresh cell, and on review it sits *inside* the 3.7 V rated range rather than outside; residual 50 mV margin recorded. **HW-011 → RESOLVED** — the enclosure layout already prevents it (battery connector internal, sensor connectors external, all labelled); raised without knowing the mechanical arrangement. **HW-010 MAJOR → MINOR** and corrected — JST-XH is keyed so a pack cannot be mis-mated, as you said; the surviving risk is a mis-crimped housing, which is caught at first power-on, so it is a yield issue not a reliability one. Also recorded that HW-003's per-cell diodes do **not** cover reverse polarity. **HW-012 held at MAJOR** with the mechanism explained — ESD and induced surge are independent of supply voltage, and dry dusty air makes Syria a harsher ESD environment, not a gentler one; fix tiered so 100 Ω series resistors give most of the benefit for pennies. |
| v9 | 2026-08-19 | Range pinned down: 0.05–0.15 m to full water, 0.70–1.00 m to tank floor. **HW-030 BLOCKER → MAJOR — the v8 escalation was wrong.** With the real range the geometry inverts: objects near the lid sit at small depth where the cone is narrow, so they are excluded unless almost on-axis; a float at the water surface returns an echo at the water's own distance. Fix is a 200 mm clearance spec plus flow-switch gating, not a stilling well. Added HW-051 (blind zone against a 5 cm minimum — now the top ultrasonic risk) and HW-052 (split-transducer parallax, +7.7 % at 5 cm, exactly correctable on the Hub). HW-050 sharpened — the metal-tank RF issue is antenna *proximity*, not enclosure; λ/4 = 17 cm standoff specified. HW-023 rescaled: under ~7 mm total across the whole range once corrected. Blockers 3 → 2. |
| v8 | 2026-08-19 | Tank sizes and mounting height received (500/1000/2000 L, plastic and metal, sensor 0.7–1.5 m above water). **HW-030 raised MAJOR → BLOCKER** with the beam geometry computed: a 60° cone reaches the sidewall at 0.87 m in a 1000 L tank against a range starting at 0.7 m, so internal obstructions — the float valve above all — sit inside the beam and the module will report a stable, plausible, permanently wrong "full". Stilling-well sizing given against the 8.6 mm wavelength; hydrostatic pressure sensing recorded as the robust alternative. Added HW-050 (metal tanks are worse on RF, thermal, acoustic and electrical grounds; both types must be supported). HW-023 error budget rescaled to the real range and restated in litres — under 1 % of tank volume is achievable once HW-030 is solved. HW-048 cross-linked to the stilling-well conflict. Blockers 2 → 3. |
| v7 | 2026-08-19 | **HW-006 → RESOLVED** — deployment is Syria with no enforced RF power limit, so +18 dBm is available and HW-003's mandatory TX power cut is amended to optional. HW-045 material question answered: potable-rated materials specified regardless, status NEEDS INFO → OPEN. HW-027 strengthened with Syrian climate data — 70–85 °C internal is at or above PETG's glass transition, so the material change and sun shield move from advisable to necessary. Added HW-047 (433 MHz channel plan, collisions, antenna-before-PA), HW-048 (transducer and surface fouling in an uncleaned tank) and HW-049 (Hub power outages waste ~0.50 Ah and lose data). Blockers 3 → 2. |
| v6 | 2026-08-19 | HW-003 recommended fix revised for real part availability (LS26500 not sourceable): keep two LS14500 in parallel with per-cell isolation — ideal-diode controller preferred, low-Vf Schottky acceptable — and cut TX power. Answered the diode leakage concern (unfounded — diodes are always forward-biased) and the drop concern (real, quantified, ~2.73 V worst case). Rejected the one-cell-plus-bulk-capacitor option with arithmetic (needs 36,000 µF) and the one-cell-plus-supercap option on energy margin (1.21×). HW-009 updated with the same arithmetic. Rechargeable and solar options assessed in reference §10.5. No change to issue counts. |
| v5 | 2026-08-19 | **HW-002 → RESOLVED** (LED and MIC5205 removed per module, already done on the current build). HW-001 BLOCKER → MAJOR and reframed — assembly method confirmed, but the harness is a cross-over cable with no controlled drawing. HW-005 BLOCKER → MAJOR and reframed around the transducer feedthrough now that the sensor gets its own in-tank enclosure. HW-003 → IN DISCUSSION with a full answer to the matched-voltage argument; single larger cell (LS26500) now the preferred fix. Added HW-045 (tank-wall penetration, in-tank connector, potable-water materials) and HW-046 (check for a D13 LED on the SPI clock). Blockers 6 → 3. |
| v4 | 2026-08-19 | HW-043 rewritten — the v3 firmware-only-OFF proposal is **withdrawn**; it contradicted HW-021's own reasoning by making OFF firmware-dependent. Magnet keeps full on/off control; the fix is a Schmitt buffer plus an inverted 1 MΩ reed connection (also drops magnet-resting standby from 360 µA to 3.6 µA). Added HW-044 (LED cannot confirm power-off, carries no state). HW-016 escalated in emphasis — the LED is now load-bearing. HW-014 Schmitt buffer requirement reinstated. HW-041 note updated; part decision unchanged. |
| v3 | 2026-08-19 | Added HW-042 (latch rail hold-up — TX droop can switch the device off permanently) and HW-043 (move the latch from edge-triggered toggle to asynchronous PRE/CLR). HW-041 raised MINOR → MAJOR: your droop argument is the real justification, not the characterisation gap; 74HC74 confirmed as the part. HW-021 DECIDED — hardware latch stays, MCU-sleep alternative rejected. HW-014 contact-wear argument withdrawn (~5 operations in service) and its Schmitt buffer superseded by HW-043. |
| v2 | 2026-08-19 | Added HW-041 (logic family for U1: CD4013BE vs 74HC74 vs SN74HCS74; SN74HCS74 recommended). Updated HW-014 notes — the Schmitt-buffer requirement is now conditional on the HW-041 outcome. Updated HW-037 notes. |
| v1 | 2026-08-19 | Initial Stage 0 hardware review — 40 issues found (6 BLOCKER, 26 MAJOR, 8 MINOR). Netlist extracted directly from `Hydro-Node.SchDoc`; PCB stackup and routing extracted from `Hydro-Node.PcbDoc`. Reed latch topology traced and verified logically correct (recorded under HW-014). |
