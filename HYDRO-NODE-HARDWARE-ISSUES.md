# HYDRO NODE — HARDWARE ISSUE TRACKER
Version: v3   |   Last updated: 2026-08-19   |   Status: Stage 0 review — power-latch architecture agreed, awaiting droop measurement

## STATUS SUMMARY
Total issues: 43   |   Open: 43   |   Resolved: 0   |   Won't fix: 0
Blockers remaining: 6
Production-ready: NO — as built the Node draws ~2 mA in sleep (≈88 days of battery, not 2 years), the ultrasonic connector pin order does not match the module it plugs into, and the two Li-SOCl₂ cells are hard-paralleled without blocking diodes.

---

## OPEN ISSUES

### HW-001 — Ultrasonic connector J5 pin order does not match the RCWL-1670 module
- Severity: BLOCKER
- Status: OPEN
- Component / net: J5 (4-pin), nets to U2.D6 (Trig), U2.D7 (Echo), VCC, GND_SW
- Problem: The schematic defines J5 as **1=GND, 2=VCC, 3=Echo, 4=Trig**. The RCWL-1670 module's pads are, left to right, **GND, RX, TX, +5V**, where RX = TRIG input and TX = ECHO output. Mapped position-for-position, pins 2 and 4 are swapped: the battery rail lands on the module's TRIG input, and the MCU's Trig output (D6) lands on the module's supply pin. The module never powers up, and D6 is driven into a rail node.
- Impact: Node does not measure at all (FR-1). Risk of damage to the ATmega328P output stage and to the module. On a production line this is an unrecoverable build error because the harness looks correct.
- Recommended fix: Re-order J5 in the schematic and PCB to **1=GND, 2=Trig, 3=Echo, 4=VCC** so a straight-through 1:1 harness works, and print the four signal names on the silkscreen next to the connector. Do not solve this with a crossed harness — a crossed cable is invisible at incoming inspection and will be built wrong.
- Notes: Confirmed from the module photo in `Components Images/1-19.jpg` (silkscreen reads `GND RX TX +5V`) and from the RCWL-1670 published pinout (RX = TRIG, TX = ECHO). **Question for you: how is the existing cable on your built board actually wired?** If you already cross-wired it by hand, the board works today but the schematic is still wrong and must be corrected before the production release.

### HW-002 — Arduino Pro Mini on-board power LED and MIC5205 regulator are permanently powered
- Severity: BLOCKER
- Status: OPEN
- Component / net: U2 (Arduino Pro Mini 3.3 V/8 MHz), VCC / GND_SW
- Problem: The design feeds the battery straight into the Pro Mini's **VCC** pin (correctly bypassing RAW), but the module's own always-on parts stay in circuit: the on-board power LED, and the MIC5205 LDO back-fed through its output pin. Measured figures for this exact board: the power LED alone draws roughly 1–3 mA, and swapping the MIC5205 for a low-Iq part is reported to save about 50 µA of sleep current. Neither is switched by anything in this design.
- Impact: **This is the single biggest threat to NFR-1.** Sleep current is ~2 mA instead of the ~10–25 µA the rest of the design is capable of. With 4.4 Ah usable, that is **≈88 days**, not 2 years — a factor of ~12 miss.
- Recommended fix: Two options, and I recommend the second for a production line.
  1. **Short term (to unblock firmware work):** on each module, remove the power LED (or its series resistor) and remove the MIC5205. Verify sleep current on a bench supply — you should land at 5–10 µA. Also program the fuses to disable BOD in sleep (BOD costs ~20 µA) and confirm the sketch puts the ATmega328P in SLEEP_MODE_PWR_DOWN (~4.5 µA with the WDT running).
  2. **Production:** place the ATmega328P (or a lower-power MCU — see HW-026) directly on the Hydro Node PCB. Reworking a hobby module on every unit is not a manufacturable process, and clone Pro Minis vary in regulator, LED resistor and fuse settings between batches.
- Notes: Independently reported measurements put a stripped Pro Mini 3.3 V/8 MHz at **~4.5 µA** in power-down. ATmega328P datasheet: power-down with WDT enabled ≈ 4.2 µA typ; with WDT and BOD disabled ≈ 0.1 µA. FR-5's 2-minute wake needs the WDT, so budget ~4.5 µA for the MCU.

### HW-003 — Two LS14500 Li-SOCl₂ cells hard-paralleled with no blocking diodes
- Severity: BLOCKER
- Status: OPEN
- Component / net: Battery connector (2-pin JST-XH), VBAT / VBAT_RTN
- Problem: The BOM specifies "2x LS14500 SAFT 3.6 V LITHIUM BATTERY (Parallel Connection)" and the schematic shows a single 2-pin battery connector, i.e. the two cells are wired directly in parallel. Lithium-thionyl-chloride primary cells must never be charged. Two directly-paralleled cells that age or passivate at different rates will push current into each other — the stronger cell charges the weaker one. Cell manufacturers require a **series blocking diode per cell** whenever primary Li cells are paralleled, for exactly this reason.
- Impact: Safety. A charged Li-SOCl₂ cell can vent, and it is inside a sealed enclosure on an occupied building's roof. This is also a certification and liability blocker, not just an engineering one.
- Recommended fix: Add a **series Schottky (or better, an ideal-diode controller) in each cell's positive leg** before they join. Use a low-leakage, low-Vf Schottky (e.g. a 20–30 V, ~0.2 V @ 1 mA part with reverse leakage in the sub-µA range at 25 °C) and check the reverse-leakage spec at 60 °C, because that leakage is a permanent drain. Also add a **PTC or fuse** in the pack lead. Confirm the choice against SAFT's own application guidance for paralleling LS-series cells.
- Notes: Also decide whether you actually need two cells. One LS14500 (2.6 Ah) supports 50 mA continuous; the Ra-02 draws up to 120 mA in TX, so the pack is sized for the pulse, not the energy. If HW-009 is solved with a proper pulse-support capacitor, a **single cell plus an HLC/supercapacitor** removes this entire problem — that is the standard industry answer for Li-SOCl₂ + LoRa.

### HW-004 — No ground plane and no copper pour anywhere on the PCB
- Severity: BLOCKER
- Status: OPEN
- Component / net: Whole board
- Problem: I parsed the Altium PCB file directly. It contains **zero polygons, zero regions and zero vias**. All 233 copper tracks are 1.0 mm wide: 227 on the bottom layer, 6 on the top. Every net — including both ground nets and the RF module's supply and return — is a long, thin, point-to-point trace. There is no reference plane under the Ra-02, under the SPI bus, or under the ultrasonic echo line.
- Impact:
  - **RF:** the Ra-02 pulls up to 120 mA in TX bursts through a single narrow trace with no plane. Supply droop at the module degrades PA output and RX sensitivity, directly costing link margin (FR-4).
  - **Accuracy (NFR-2):** no return-current reference means the echo timing line shares its return with everything else. Any ground bounce during a transition shifts the echo edge.
  - **EMC:** an unplaned board with a 433 MHz PA and 300 mm of unshielded sensor cable is a radiator and a receiver. It will be hard to pass emissions testing and easy to upset.
- Recommended fix: Respin as a **4-layer board** (signal / GND / VBAT / signal) — at this board size the cost delta is small and it removes a whole class of problems at once. If you must stay 2-layer: pour solid GND_SW on the bottom, route signals on top, stitch the pour with vias every ~5 mm, keep an unbroken plane under the Ra-02 footprint, and widen VBAT and GND_SW to ≥1.5 mm. Board is currently ~90 × 68 mm, so there is plenty of room.
- Notes: This respin is where HW-001, HW-007, HW-013, HW-015, HW-017, HW-018 and HW-029 should all be fixed together.

### HW-005 — RCWL-1670 electronics have no defined environmental protection in a condensing headspace
- Severity: BLOCKER
- Status: NEEDS INFO
- Component / net: RCWL-1670 module, J5
- Problem: The module in the photos is a **bare, uncoated PCB** with two transducers soldered to it. The tank headspace it looks into is at or near 100 % RH and will condense every time the tank refills with cool water or the roof cools at night. Nothing in the design says where this PCB physically lives or how it is protected. Its rated operating range is −25 °C to +85 °C, 5–95 % RH — the tank headspace is outside the humidity rating.
- Impact: Corrosion and dendrite growth on an uncoated PCB in a condensing environment is a matter of months, not years. Condensate on the transducer faces also attenuates the echo and causes intermittent dropouts — which read as bad level data, not as a fault. Threatens NFR-2 and NFR-3.
- Recommended fix: Decide the mechanical arrangement, then protect accordingly:
  - Keep the **PCB inside the sealed enclosure** and bring only the two transducers into the headspace on a short cable through a potted feedthrough; or
  - Conformally coat / pot the module PCB (leaving the transducer faces clear) and mount it as a sealed sub-assembly.
  Either way, add a small **drip shield / sun shield** over the transducer faces so condensate running down the tank roof cannot pool on them.
- Notes: **Questions for you:** (1) Is your RCWL-1670 the variant with the transducers soldered to the board, or the variant with them on flying leads? (2) Where do you intend to physically mount the module relative to the tank lid? (3) Roughly what is the tank headspace temperature range at your site?

### HW-006 — LoRa band, region and legal radiated power are undefined
- Severity: BLOCKER
- Status: NEEDS INFO
- Component / net: Ra-02 module (J1/J2), antenna
- Problem: The Ra-02 is a **433 MHz** module (silkscreen: ISM 410–525 MHz, PA +18 dBm). Nothing in the documentation states the deployment country or the intended output power. In ITU Region 1 the 433.05–434.79 MHz band is generally limited to **10 mW ERP** with duty-cycle restrictions — +18 dBm is 63 mW and would be non-compliant. Other regions differ, and in some the whole band is unusable for this purpose.
- Impact: A product that cannot be legally sold or installed. Also drives the link budget, and therefore the spreading factor, and therefore the battery budget (see HW-031) — so this answer changes the power design, not just the paperwork.
- Recommended fix: Tell me the deployment country/countries. Then we fix the band, the maximum TX power, and the duty-cycle budget, and I will size the link and the spreading factor against them. If the market is EU/UK, seriously consider moving to **868 MHz (Ra-01 / SX1276)**: 25 mW (14 dBm) allowed, better antenna efficiency for a given size, and a far less congested band than 433 MHz.
- Notes: I cannot close this one without an answer from you. Everything downstream — antenna selection, range expectation, SF choice, battery life — depends on it.

---

### HW-007 — Ra-02: only one of its three GND pins is connected
- Severity: MAJOR
- Status: OPEN
- Component / net: J1 pins 1 and 2, J2 pin 1 (all GND), J2 pin 8
- Problem: Extracted from the schematic netlist: the module's ground returns to the board through **J2 pin 8 only**. J1.1, J1.2 and J2.1 are left unconnected. The module's 120 mA TX return current therefore takes one long 1 mm trace.
- Impact: Supply droop and ground bounce at the RF module during TX, reduced TX power and RX sensitivity, and a large radiating current loop. Compounds HW-004.
- Recommended fix: Connect all three GND pins to the ground pour. In a 2-layer respin, tie them to the pour with a short, wide connection and stitch around the module footprint.
- Notes: Also connect J1 pin 4 (RST) — already done via D9 — and consider bringing DIO1 out to a spare pin; some LoRa stacks use DIO1 for RX-timeout and CAD-done, which the pairing protocol (Stage 6) may want.

### HW-008 — Ra-02 fed directly from a fresh Li-SOCl₂ cell, at the top of its rated supply range
- Severity: MAJOR
- Status: OPEN
- Component / net: J1 pin 3 (3V3), VBAT
- Problem: The Ra-02's specified operating range is **1.8–3.7 V**. A fresh, unloaded LS14500 sits at ~3.6–3.67 V. The design connects the cell straight to the module with no regulation, so the module runs at the very top of its range with essentially no margin, and any transient above 3.7 V is out of spec.
- Impact: No design margin on the most expensive and most failure-sensitive part on the board. Also, an unregulated rail that sags under a 120 mA TX pulse means the PA sees a moving supply — output power and frequency stability both move with it.
- Recommended fix: Either (a) add a low-Iq LDO (target quiescent < 2 µA, e.g. a 3.0–3.3 V part) feeding the Ra-02 and the MCU, accepting ~0.3 V of dropout headroom loss; or (b) keep the direct connection and formally accept the risk, but then you must measure the rail at the module during TX across the full temperature range and confirm it never exceeds 3.7 V. Option (a) also stabilises the ADC reference and the ultrasonic drive amplitude, which helps NFR-2.
- Notes: The SX1278 silicon's absolute maximum is 3.9 V, so this is a margin issue rather than an immediate destruction risk — but "operating at the datasheet limit" is exactly the kind of thing that passes on a bench and fails in a batch.

### HW-009 — C3 (2200 µF aluminium electrolytic) is the wrong part in the wrong place
- Severity: MAJOR
- Status: OPEN
- Component / net: C3, VBAT / GND_SW
- Problem: Three separate problems with this one part:
  1. **Leakage.** Aluminium electrolytics leak continuously and the leakage roughly doubles every 10 °C. A 2200 µF part at 3.6 V will leak on the order of 5–30 µA at 25 °C and can reach 50–200 µA at a 60 °C rooftop enclosure temperature. That is a permanent, unswitched drain on the 2-year budget, and it is the second-largest sleep-current item after HW-002.
  2. **Cold ESR.** Aluminium electrolytic ESR rises 5–20× at −10 to −20 °C. The cap is least effective exactly when the cell is weakest, which is the opposite of what you need.
  3. **It cannot do the job it was added for.** To hold a 120 mA TX burst within 0.2 V for even a short 60 ms SF7 packet you would need ~36,000 µF. 2200 µF holds that current for about 3.7 ms. **The battery supplies the TX current, not C3.** C3 only softens the first few milliseconds of the current edge.
- Impact: Threatens NFR-1 directly, and gives false confidence that the Li-SOCl₂ pulse problem is solved when it is not.
- Recommended fix: Remove C3 and replace the function properly:
  - **Local bulk:** 100–220 µF of **X5R/X7R ceramic** (e.g. 2× 100 µF 6.3 V 1210) placed at the Ra-02 supply pins. Near-zero leakage, stable ESR over temperature, no wear-out. Sized to cover the current edge, which is all a capacitor can do here.
  - **Pulse support for the cell:** if measurement shows the cell voltage dipping too far during TX, add a **hybrid layer capacitor (HLC) or a 0.1–0.5 F supercapacitor** charged through a current-limiting resistor. This is the standard Li-SOCl₂ + LoRa arrangement and it is the correct answer to the pulse problem. Budget the supercap's own leakage (typically 5–50 µA) into the power model before committing — it may cost more than it saves.
- Notes: First measure. Put a scope on VBAT during a real TX burst at −5 °C and at 50 °C, on a cell that has been sitting idle for a week (see HW-032). If the dip is acceptable, you may not need any pulse-support part at all and can delete C3 outright, which is the cheapest possible fix.

### HW-010 — No reverse-polarity protection on the battery input
- Severity: MAJOR
- Status: OPEN
- Component / net: Battery connector, VBAT
- Problem: The battery lands on a 2-pin JST-XH and goes straight to the CD4013's VDD, the MCU's VCC, the Ra-02's 3V3 and the ultrasonic module's supply. A reversed pack destroys all of them at once, and reverse-biases C3.
- Impact: On a production line with hand-crimped battery leads, reversed packs happen. One reversal is a scrapped board, not a recoverable fault.
- Recommended fix: Add a **series P-channel MOSFET ideal-diode** in the VBAT line (gate to ground through a resistor, source to battery, drain to the rail). Costs ~20 mΩ and essentially zero quiescent current, and it does not eat the 0.2–0.3 V that a Schottky would — which matters when the cell plateau is only 3.6 V. Note that the per-cell blocking diodes from HW-003 give partial protection but do not cover a reversed connector.
- Notes: Combine with HW-011 — a keyed, non-interchangeable battery connector plus the P-FET makes this failure mode essentially impossible.

### HW-011 — Battery and flow-switch connectors are both 2-pin JST-XH and are interchangeable
- Severity: MAJOR
- Status: OPEN
- Component / net: Battery connector, J4 (FLOW) — BOM lists 2× B2B-XH-A
- Problem: Two mechanically identical 2-pin connectors sit on the same board. Plugging the battery pack into J4 puts 3.6 V directly onto D5 and onto the switched-ground net. Plugging the flow switch into the battery connector shorts VBAT to the raw battery return whenever water flows.
- Impact: Board or cell damage from a plausible assembly or field-service mistake. Fails the "repeatable assembly" half of NFR-6.
- Recommended fix: Make them physically impossible to swap. Use a **different connector family or a different key for the battery** (e.g. JST-PH 2-pin or a polarised 2-pin locking connector for the battery, keep XH for the sensors), and add function names to the silkscreen (see HW-038).
- Notes: Cheapest possible fix; do it in the same respin.

### HW-012 — No ESD or surge protection on any of the three external sensor cables
- Severity: MAJOR
- Status: OPEN
- Component / net: J3 (Temp), J4 (Flow), J5 (Ultrasonic)
- Problem: Three cables leave a sealed enclosure and run across a rooftop to a tank and a fill pipe. Nothing on the board protects the pins they land on: D3, D4, D5, D6, D7 all go straight to the MCU.
- Impact: Direct ESD hits during installation, and induced surges from nearby lightning, will kill MCU pins. The 1-Wire line (D4) is the classic victim — it is a long, high-impedance, pulled-up line. A dead pin on a sealed rooftop device is a truck roll.
- Recommended fix: On each externally-exposed line: a **low-capacitance TVS/ESD array to ground** (choose < 5 pF for the echo and 1-Wire lines so you do not slow the edges), plus a **100 Ω series resistor** at the MCU pin. Ensure the TVS ground is the pour, close to the connector. Also fit a TVS across the supply feeding the ultrasonic module.
- Notes: The 100 Ω series resistors are nearly free and also limit fault current if a cable shorts to the rail.

### HW-013 — Decoupling is wrong: only two bypass caps, neither local to any device
- Severity: MAJOR
- Status: OPEN
- Component / net: C4 (VBAT–GND_RAW), C5 (VBAT–GND_SW); C1 and C2 are not decoupling
- Problem: The BOM's four 100 nF caps are used as: C1 = reed debounce, C2 = power-on-reset coupling, C4 = raw-battery bypass, C5 = switched-rail bypass. So the entire board has **two** bypass capacitors, and from the PCB layout both sit at the far left, physically distant from the CD4013 (right edge), the Pro Mini (lower left) and the Ra-02 headers (top). **There is no decoupling at all at the Ra-02 supply pins** — the highest di/dt load on the board.
- Impact: Rail collapse and ringing during TX; unreliable SPI at the module; noise coupled into the echo timing. Compounds HW-004 and HW-009.
- Recommended fix: Add, placed within a few millimetres of the pin they serve:
  - Ra-02 3V3 (J1.3): 100 nF + 10 µF ceramic, with the return going straight into the pour.
  - Pro Mini VCC: 100 nF.
  - CD4013 VDD (pin 14): 100 nF — this one is genuinely absent today.
  - Ultrasonic supply at J5: 100 nF + 10 µF (it draws 6 mA in bursts down a cable).
- Notes: Ceramic X7R throughout; leakage is negligible so this costs nothing in the power budget.

### HW-014 — Reed latch has no series resistor and no effective debounce
- Severity: MAJOR
- Status: OPEN
- Component / net: S1, R3 (10 kΩ), C1 (100 nF), U1 pin 3 (CLOCK1)
- Problem: **The latch topology itself is correct** — I traced it and it is a proper toggle (see Notes). The implementation has two defects:
  1. **No series resistance between S1 and the C1/CLOCK1 node.** When the reed closes it dumps VBAT straight into C1 with only wiring resistance in the loop. Repeated hot-switching into a capacitor is the classic way to weld or pit reed contacts.
  2. **Debounce is inadequate.** Contact bounce, and the multiple make/break events you get when a magnet is swept past rather than placed, each produce a clean rising edge at CLOCK1 — and each rising edge toggles the flip-flop. An even number of toggles leaves the device in the state it started in.
- Impact: The user brings the magnet up, the LED does something ambiguous, and the device may be OFF when they think it is ON — on a roof, sealed, with no other feedback. Also a slow degradation path for S1.
- Recommended fix:
  - Add **R_series ≈ 10 kΩ** between S1 and the C1/R3 node. This limits contact current and gives a defined attack time constant.
  - Once you add that series resistor, **both** edges at CLOCK1 become slow (attack ≈ 1 ms, decay ≈ 1 ms). Feed the node through a **Schmitt-trigger buffer (74LVC1G17, non-inverting, Icc typically well under 1 µA)** into CLOCK1 rather than driving the flip-flop directly. This both cleans the bounce and guarantees a fast edge into the clock input.
  - Use a **non-inverting** buffer so the toggle still happens on magnet *approach*, not on removal.
- Notes (v3): **Largely superseded by HW-043.** If the latch moves from edge-triggered CLOCK to asynchronous PRE/CLR, contact bounce becomes harmless (each bounce simply re-asserts ON) and the Schmitt buffer is deleted. The series resistor is still worth fitting because it costs nothing, but the **contact-wear argument in this issue is withdrawn** — at roughly 5 operations over the product's life, hot-switching into C1 is not a wear concern. You were right to push back on that.
- Notes (v2): The Schmitt buffer requirement is now coupled to the U1 part choice — see **HW-041**. If U1 becomes an **SN74HCS74**, every input is already Schmitt-triggered with no transition-rate requirement, and the separate buffer is deleted (the series resistor and the RC are still required). If U1 stays **CD4013BE** or becomes a plain **74HC74**, the buffer is mandatory — and more so for the 74HC74, which is the least tolerant of slow edges of the three.
- Notes: **Verification of the latch as drawn, per your Section 6 item 6.** I traced it from the extracted netlist: U1 is wired as a T-flip-flop (D1 ← Q1̄, pin 5 ← pin 2), CLOCK1 ← reed node, Q1 (pin 1) → R2 1 kΩ → Q1 gate, R1 1 MΩ gate pulldown to the raw battery return. SET1, SET2, RESET2, CLOCK2, D2 are all correctly tied to VSS; the unused half's outputs are correctly left open. C2 (100 nF from VBAT) with R4 (100 kΩ to VSS) is a correct **active-high power-on-reset** giving ~10 ms — so the device powers up OFF when a cell is first fitted. **Toggle logic is correct and the OFF state is genuinely clean:** with Q1 off, every resistive path (R1, R3, R4) sits at 0 V across it, C2 and C4 are ceramic, and the only OFF-state current is the MOSFET's leakage plus the CD4013's quiescent (< 1 µA at 25 °C). I have no objection to the concept — only to the debounce, the contact protection, and the placement (HW-015).

### HW-015 — Reed switch is mounted in the centre of the PCB
- Severity: MAJOR
- Status: OPEN
- Component / net: S1
- Problem: From the PCB layout, S1 sits vertically in the middle of a ~90 × 68 mm board, roughly 30 mm from the nearest edge. The magnet has to actuate it through a PETG-CF wall plus that 30 mm of standoff, and the user has no marked spot to place the magnet against.
- Impact: Unreliable or impossible actuation — the primary and only user control on a sealed device (FR-6, NFR-5). Magnet field falls off steeply; 30 mm of extra distance can easily be the difference between working and not.
- Recommended fix: Move S1 to a **board edge**, oriented along its sensitive axis, as close to the enclosure wall as clearance allows. Mark the spot on the outside of the enclosure (embossed target in the print). Then verify the actual pull-in distance with the production magnet and the production wall thickness, and specify a minimum magnet grade in the BOM.
- Notes: Also see HW-039 — the bare glass reed body is mechanically fragile. If you adopt the Hall-sensor alternative there, this placement problem gets easier because a Hall device is small and can sit right at the wall.

### HW-016 — Blue LED has no forward-voltage headroom on a 3.0–3.6 V rail
- Severity: MAJOR
- Status: OPEN
- Component / net: DS1, R5, U2.D8
- Problem: The BOM specifies a **blue** 3 mm LED (confirmed in the component photos), while the schematic symbol is a red LED. A blue LED has a forward voltage of roughly 2.7–3.2 V. The rail is 3.6 V falling to ~3.0 V over the cell's life, and the ATmega output stage drops another ~0.2 V. Through the schematic's 330 Ω that gives ~2 mA when fresh and essentially nothing at end of life — the status indicator goes dark exactly when you most need to know the battery is low.
- Impact: The Node's only local feedback stops working. Compounds HW-014 (ambiguous on/off feedback).
- Recommended fix: Use a **red or yellow LED (Vf ≈ 1.8–2.1 V)**, which leaves 1.0–1.8 V across the resistor across the whole cell life. Size R5 for 2 mA (it only ever flashes briefly), so ~680 Ω–1 kΩ. Make the schematic, the BOM and the fitted part agree.
- Notes: There is also a **BOM/schematic value mismatch** here — the BOM lists a 220 Ω resistor, the schematic says 330 Ω. Whichever survives, one document is wrong today.

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

### HW-018 — Echo is on D7 instead of D8, so hardware input capture is unavailable
- Severity: MAJOR
- Status: OPEN
- Component / net: J5 pin 3 (Echo) → U2.D7; U2.D8 → R5/DS1
- Problem: The ATmega328P's **Input Capture pin (ICP1) is PB0 = Arduino D8**, which this design uses for the status LED. Echo went to D7, an ordinary GPIO, so the echo pulse can only be measured in software (`pulseIn` or a pin-change interrupt).
- Impact: `pulseIn` at 8 MHz resolves to roughly ±4 µs, and a pin-change ISR adds 1–2 µs of latency jitter. That is only ±0.7 mm of distance error so it is not fatal — but ICP1 gives a hardware timestamp at 125 ns resolution with zero interrupt-latency jitter, it is completely free, and it lets the MCU sleep during the flight time instead of spinning in a blocking loop (which also saves awake energy). Given NFR-2 explicitly drives this design, leaving a free hardware timer on the table is not defensible.
- Recommended fix: **Swap them — Echo to D8, LED to D7.** One net change in the respin, no cost, no extra parts.
- Notes: This also means the ultrasonic measurement no longer blocks the CPU, which shortens the awake window and helps NFR-1.

### HW-019 — HT-60 flow switch: a 220 VAC contact switched dry at ~110 µA
- Severity: MAJOR
- Status: OPEN
- Component / net: Flow switch (HT-60), J4, U2.D5
- Problem: The flow switch in the photos is an **HT-60, rated AC 220 V 0.5 A** — a mains pump-control switch. The design uses it as a dry contact at 3.3 V, sensed through the ATmega's internal pull-up (20–50 kΩ), i.e. roughly **110 µA** of contact current. Contacts designed for mains loads rely on the arc to burn through oxide and sulphide films; at microamp levels and a few volts, that film is never broken down and the contact reads open while it is mechanically closed.
- Impact: FR-3 fails intermittently in the field, months after installation, in a way that looks like "no fill events detected" rather than a hardware fault. This is a well-known and very hard-to-diagnose failure mode.
- Recommended fix: Either
  1. **Change the sensor** to one specified for low-level/dry-circuit switching (gold-plated reed contacts, rated for signal-level currents), or
  2. **Force a wetting current.** Drive a spare GPIO high through ~330 Ω into the switch node for a few milliseconds each wake, sample D5, then release the pin. That gives ~10 mA of wetting current for a few ms per 2 minutes — an average of a few nanoamps, so it costs nothing in the power budget, and it breaks down the film. This needs one spare GPIO (A0–A5 are all free) and a resistor.
  I recommend (2) as an addition regardless of which sensor you use, because it also protects against the same problem in any replacement part.
- Notes: **Question: is your HT-60 normally-open (closes on flow) or normally-closed?** Confirm before firmware. Also confirm its minimum actuation flow rate against your actual fill rate — if the fill is slower than the switch's threshold, it will never trip.

### HW-020 — Flow input D5 has no external pull-up, no filter and no series protection
- Severity: MAJOR
- Status: OPEN
- Component / net: J4 pin 1 → U2.D5
- Problem: D5 goes straight from the MCU to a connector and out on a cable to the fill pipe, with nothing on it. The design relies entirely on the ATmega's internal pull-up.
- Impact: Three separate problems. (a) If firmware leaves the internal pull-up enabled during sleep while the switch is closed, that is **~110 µA continuous** — on its own about half of the entire allowable average current for the 2-year target. (b) An unfiltered mechanical contact on a long cable will produce chatter and pick up noise. (c) No series protection (see HW-012).
- Recommended fix: Add an **external 1 MΩ pull-up** to the switched rail plus a **100 nF** cap to ground at the connector (giving a 100 ms RC filter, which is fine for a flow signal that changes on a timescale of seconds), and a **100 Ω** series resistor at the MCU pin. 1 MΩ costs 3.6 µA when the switch is closed, and the internal pull-up can then stay off permanently. Combine with the wetting-pulse scheme from HW-019 for the actual sampling.
- Notes: Firmware must still be explicit: internal pull-up **disabled** before sleeping, every time.

### HW-021 — The MCU cannot read the reed line and cannot command its own shutdown
- Severity: MAJOR
- Status: OPEN
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

### HW-022 — No local unpair or recovery path on a sealed Node
- Severity: MAJOR
- Status: OPEN
- Component / net: System-level; U2, S1
- Problem: FR-7 states that unpairing is initiated from the Hub only. If a Hub is lost, destroyed or replaced, the Node is permanently bound to a Hub that no longer exists — inside a sealed enclosure with no ports, no buttons and (per HW-021) no way for the magnet to say anything except "toggle power".
- Impact: A dead Hub bricks every Node on the site. That is a support and warranty problem, not just an engineering one.
- Recommended fix: Implement HW-021's reed-sense line, then define a **magnet gesture** for local unpair — e.g. hold the magnet on the target for 10 seconds *after* power-on, confirmed by a distinctive LED pattern. This preserves the intent of FR-7 (no accidental unpairing, Hub is the normal path) while giving a documented field-recovery route. Firmware-only once the sense wire exists.
- Notes: Raised now because it costs one PCB net; retrofitting it after the respin is expensive. The protocol details belong in Stage 6.

### HW-023 — A single air-temperature sensor cannot represent the headspace thermal gradient
- Severity: MAJOR
- Status: OPEN
- Component / net: J3, DS18B20
- Problem: FR-2 corrects the speed of sound using one DS18B20 in the headspace. But on a sunlit rooftop tank, the air touching the hot tank roof can be 15–25 °C warmer than the air just above the cool water. The ultrasonic pulse travels through that entire gradient, so what matters is the **path-average** temperature — and a single sensor mounted near the sensor (i.e. at the hot end) systematically over-estimates it.
- Impact: **This is the dominant error source in the whole measurement.** The speed of sound changes by about 0.606 m/s per °C, so a 1 °C path-average error is a 0.177 % distance error. A realistic 8–10 °C path-average error gives **28–35 mm of error at a 2 m range** — an order of magnitude worse than every other term in the budget. Directly threatens NFR-2.
- Recommended fix, in increasing order of effectiveness:
  1. **Two DS18B20s on the same 1-Wire bus** — one at the transducer, one on a lead reaching down near the low-water line — and average them. This is the highest value-for-money fix on this whole list: one extra part, **zero extra pins** (that is the point of 1-Wire), and it turns the worst error term into one of the smaller ones. Do this.
  2. Shade the sensor head and the tank lid so the gradient is smaller to begin with.
  3. Longer term, if you need better than ~1 %: a **fixed reference reflector** at a precisely known distance in the beam, so the firmware measures the actual speed of sound every cycle and cancels temperature, humidity and clock error in one step. Honest caveat: HC-SR04-compatible modules report only the *first* echo, so this needs a module that exposes the raw echo envelope or supports multi-echo. Not achievable with the RCWL-1670.
- Notes: **Humidity is a second, smaller term.** Saturated air raises the speed of sound by roughly 0.35–0.6 % versus dry air at 30–40 °C, which is +7 to +12 mm at 2 m. A tank headspace is essentially always saturated, so this one is easy — apply a fixed saturated-air correction constant on the Hub and most of it disappears. Note that this correction lives on the **Hub**, consistent with your Section 2 split.

### HW-024 — Pro Mini 8 MHz clock source is unknown (crystal vs ceramic resonator)
- Severity: MAJOR
- Status: NEEDS INFO
- Component / net: U2 clock source
- Problem: The echo pulse is timed by the MCU, so **the MCU's clock accuracy is the distance measurement's scale factor.** A quartz crystal is ±30 ppm and irrelevant. A ceramic resonator is typically ±0.5 % initial plus a few tenths of a percent over temperature.
- Impact: With a resonator, ±0.5 % is **±10 mm at 2 m**, and it drifts with the rooftop temperature swing, so it is not even a fixed offset you could calibrate out once. That would make the clock the second-largest error term after HW-023.
- Recommended fix: Identify the part on your actual modules. If it is a resonator, put a **±30 ppm crystal (or a TCXO) on the Node PCB** when you move the MCU onto the board (HW-026). If you must ship with a resonator, calibrate each unit's timebase at production test against a known reference distance and store the correction factor in EEPROM — which is a real production step with real cost, and is a good argument for just fitting a crystal.
- Notes: **Please photograph the clock component on one of your modules, or tell me the exact board variant.** SparkFun and the various clone vendors do not all use the same part.

### HW-025 — No battery voltage or health telemetry
- Severity: MAJOR
- Status: OPEN
- Component / net: U2, VBAT
- Problem: Nothing measures the battery. There is no divider to an ADC pin, and no other mechanism.
- Impact: For a 2-year sealed field device this is a serious operational gap. You cannot schedule replacement, cannot distinguish "Node is dead" from "Node is out of range", and cannot detect a bad cell batch before it becomes a field campaign.
- Recommended fix: **Use the ATmega328P's internal bandgap reference measured against VCC.** This needs **zero extra components and zero leakage** — you set the ADC mux to the 1.1 V bandgap with AVcc as the reference, read it, and compute VCC = 1.1 × 1024 / ADC. Send the raw ADC count to the Hub and let the Hub do the conversion, per your Section 2 split. Do not use a resistive divider: any divider across VBAT leaks continuously unless you MOSFET-gate it, which is more parts for a worse answer.
- Notes: For Li-SOCl₂ the open-circuit voltage is famously flat, so absolute voltage tells you little about remaining capacity. The genuinely useful signal is the **loaded voltage dip during the LoRa TX burst** — sample VCC mid-transmission. A dip that grows over months is the real end-of-life and passivation indicator. Log both.

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
- Notes: I know NFR-4 specifies PETG-CF and I am not overriding that — this is the recommendation and the reasoning; the decision is yours. If you want to stay with PETG-CF, the sun shield becomes mandatory rather than optional, and I would want the internal temperature logged over a full summer before sign-off.

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

### HW-030 — Tank geometry is unknown — beam angle versus sidewall echoes
- Severity: MAJOR
- Status: NEEDS INFO
- Component / net: RCWL-1670, mechanical
- Problem: Ultrasonic modules of this class have a beam width in the region of 60–75°. In a narrow tank the beam illuminates the sidewall before it reaches the water, and the module reports the *first* echo it hears — which is the wall, not the surface. The reading then reads as a constant, plausible-looking, completely wrong distance.
- Impact: Systematically wrong level readings that do not look like a fault. Threatens NFR-2 fundamentally, and it is a mechanical problem that no amount of firmware can fix after the fact.
- Recommended fix: I need the tank dimensions to size this. Then the standard mitigations are: centre the sensor in the tank; keep it clear of the fill pipe, the outlet and any internal structure; and if the geometry is tight, fit a **stilling well / waveguide** (a smooth vertical tube of 75–100 mm bore running down from the sensor) which confines the beam, kills sidewall returns and also damps surface ripple. A waveguide is the standard industrial answer and it would substantially improve NFR-2.
- Notes: **Please give me: tank internal diameter (or length × width), tank height, typical mounting height of the sensor above the maximum water level, and whether the tank is plastic or metal.** Also whether the fill pipe discharges above or below the water line — an above-water fill produces splashing directly in the beam.

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

### HW-033 — BOM omissions
- Severity: MINOR
- Status: OPEN
- Component / net: BOM
- Problem: Comparing the BOM against the schematic and the assembly, the following are used but not listed: the **two 8-pin headers/sockets for the Ra-02** (J1, J2 — the largest single omission), the **battery holder or cell tabs** (the LS14500s in the photo are bare button-top cells with no tabs), a **DIP-14 socket** if one is used for U1, the **PCB itself**, cable glands, enclosure hardware and fasteners, conformal coating, desiccant, and the enclosure gasket.
- Impact: An incomplete BOM means an incomplete kit at the production line and unbudgeted cost.
- Recommended fix: Rebuild the BOM from the schematic's designator list rather than by hand, add a mechanical/consumables section, and add a **manufacturer part number and a lifecycle status** column for every line. Cross-check every designator appears exactly once.
- Notes: Also add: LoRa antenna, IPEX-to-SMA pigtail (listed), SMA bulkhead gasket, and the actuating magnet (listed, but with no grade or dimensions specified — see HW-015).

### HW-034 — C3 voltage rating mismatch between BOM and supplied part
- Severity: MINOR
- Status: OPEN
- Component / net: C3
- Problem: The BOM and schematic both specify 2200 µF **16 V**; the component photo shows a 2200 µF **25 V** part. Electrically the 25 V part is fine, but the can diameter differs (typically 13 mm vs 10 mm), so it may not match the PCB footprint or the enclosure clearance.
- Impact: Fit problem at assembly; documentation does not describe the built article.
- Recommended fix: Moot if HW-009 is adopted and C3 is deleted or replaced with ceramics. If C3 survives in any form, make the BOM, schematic, footprint and purchased part agree, and specify the diameter and lead pitch explicitly.
- Notes: Flagged mainly because it is a symptom — the BOM and the built board have drifted apart in at least three places (this, HW-016's LED colour, and HW-016's resistor value). Worth a full reconciliation pass.

### HW-035 — Unused MCU I/O left floating will add sleep current if not configured
- Severity: MINOR
- Status: OPEN
- Component / net: U2 — A0–A7, D0, D1, DTR, and the unconnected Ra-02 DIO lines
- Problem: Nine analogue pins plus the UART pins are unconnected. A floating CMOS input sits near its switching threshold and its input stage draws crossbar current; several floating pins can add tens of microamps.
- Impact: Silent addition to sleep current — precisely the kind of thing that makes a measured power budget disagree with the calculated one.
- Recommended fix: Firmware must explicitly configure **every** unused pin before sleeping — either as an input with the internal pull-up enabled, or as an output driven low. Add this to the Stage 7 checklist and make it a measured pass/fail. It is a firmware fix, but it is recorded here because it is a power-path issue and it will be forgotten otherwise.
- Notes: In the respin, tie genuinely unused pins to ground through pads so the state is defined by hardware rather than by remembering.

### HW-036 — Carbon-film ½ W resistors throughout
- Severity: MINOR
- Status: OPEN
- Component / net: R1–R6
- Problem: All six resistors are ½ W axial carbon film (±5 % typical, and a temperature coefficient in the −200 to −1000 ppm/°C region). They are also physically large for a board that should be moving to SMD.
- Impact: Low, in this circuit — none of these resistors is in a precision path. R6 (the 1-Wire pull-up) and R3/R4 (the RC networks) all tolerate ±5 % easily. The real cost is size and assembly method.
- Recommended fix: Move to **0603 1 % metal-film** in the respin, for placement by machine and for a defined tempco. Not urgent on its own — bundle it with HW-026.
- Notes: Recorded for completeness; this is the lowest-priority item on the list.

### HW-037 — CD4013BE in a plastic DIP
- Severity: MINOR
- Status: OPEN
- Component / net: U1
- Problem: The CD4013BE is a 14-pin plastic DIP. If it is socketed (as the through-hole build style suggests), the socket contacts oxidise over years in a humid, thermally-cycling outdoor enclosure, and vibration can back the part out.
- Impact: An intermittent contact in the power-latch IC means the device randomly turns off, or fails to turn on, in the field.
- Recommended fix: Use the **SOIC-14 version soldered directly** (CD4013BM or equivalent). If U1 survives the architecture decision under HW-021 at all, do not socket it. Confirm the quiescent current and the operating temperature range against your chosen vendor's datasheet — the family covers −55 °C to +125 °C and 3–18 V, which is comfortable here, but I want the specific part's Iq over temperature in the power model rather than an assumption.
- Notes: I also want to check one specification I could not retrieve during this review: most CD4013B datasheets state a **maximum clock input rise/fall time** (I believe around 15 µs at V_DD = 5 V, but treat that as unverified until you or I read the vendor datasheet). As drawn today the *active* rising edge at CLOCK1 is fast, so the spec is probably not violated — but the moment you add the series resistor from HW-014 it will be, which is exactly why HW-014 also calls for a Schmitt-trigger buffer. **Please confirm the number from your part's datasheet.**
- Notes (v2): Partly superseded by **HW-041**. The package question here (SOIC, not socketed DIP) stands regardless of which logic family wins.

### HW-038 — Connector functions are not on the silkscreen; DS18B20 wire order is undocumented
- Severity: MINOR
- Status: OPEN
- Component / net: J3, J4, J5, battery connector
- Problem: The silkscreen shows reference designators (J4, J5, …) but not what plugs into them or what each pin does. The DS18B20 waterproof probe ships as three flying wires with no connector, so the crimp order is a build instruction that exists nowhere in the documentation.
- Impact: Assembly errors, and field-service errors. Combined with HW-011's interchangeable 2-pin connectors, this is how a battery ends up in the flow-switch socket.
- Recommended fix: Silkscreen every connector with its function and its pin-1 signal name — `TEMP  1:DATA 2:GND 3:VCC`, `FLOW`, `ULTRASONIC  1:GND 2:TRIG 3:ECHO 4:VCC`, `BATTERY +/−`. Add a wire-colour table to the assembly drawing (the DS18B20 probe's usual convention is red = VDD, black or blue = GND, yellow or white = DATA, but **verify it on your actual probes** — clone probes are not consistent, and a swapped VDD/DATA will destroy the sensor).
- Notes: Free to fix in the respin, and it prevents several of the more expensive mistakes on this list.

### HW-039 — Reed switch is a bare glass body with long unsupported leads
- Severity: MINOR
- Status: OPEN
- Component / net: S1
- Problem: The reed in the photo is a bare glass envelope (4 × 29 mm) with long thin leads. From the 3D view it stands vertically off the board with nothing supporting the body.
- Impact: Glass reeds crack from shock, and unsupported leads fatigue under vibration. This is the device's only user control and it is inside a sealed enclosure, so a failure is unrecoverable in the field.
- Recommended fix: At minimum, use a **plastic-encapsulated or moulded reed**, mount it lying flat against the board, and secure the body with a dab of adhesive. Better: replace it with a **micropower Hall-effect sensor** (for example a device sampling at ~20 Hz for around 1.5 µA average). Solid-state, tiny, no glass, no bounce, and a clean digital output — which also removes the debounce problem in HW-014 and makes the placement problem in HW-015 much easier. The cost is ~1.5 µA of continuous current, which is negligible against the 251 µA budget, and it only applies in the ON state.
- Notes: A latching Hall device would let you keep the toggle behaviour in the sensor itself. If you go with the "delete the CD4013" architecture from HW-021, an omnipolar Hall sensor driving an MCU interrupt is the natural pairing.

### HW-040 — Antenna: no RF keep-out, and the SMA bulkhead is an unmanaged sealing penetration
- Severity: MINOR
- Status: OPEN
- Component / net: Ra-02 IPEX connector, SMA pigtail, enclosure
- Problem: The PCB has no RF keep-out region around the module, and the antenna leaves the enclosure through an SMA bulkhead — a metal penetration in a sealed wall that nothing in the design specifies how to seal. There is also no defined antenna position relative to the tank, which for a metal tank matters a great deal.
- Impact: Reduced range (which, per HW-031, you cannot compensate for by raising the spreading factor), and a water-ingress path straight into the electronics.
- Recommended fix: Keep copper and metal away from the module's antenna feed area; specify a **gasketed/O-ring SMA bulkhead** with a defined torque, and add thread sealant to the assembly instructions. Define the antenna's mounting position and orientation in the installation guide — vertical, clear of the tank body, and if the tank is metal, mounted off it rather than against it. Verify the installed link budget on a real roof before locking the SF choice from HW-031.
- Notes: A cheaper and more reliable alternative for a sealed product is an **external antenna on a short pigtail mounted through a single gland**, or an internal antenna if the enclosure and the range allow it. Worth evaluating once HW-006 fixes the band.

---

### HW-041 — Logic family for the power-latch flip-flop: CD4013BE vs 74HC74 vs SN74HCS74
- Severity: MAJOR *(raised from MINOR in v3)*
- Status: IN DISCUSSION
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
- Notes (v3): The v2 note that this "closes no blocker and is a production-quality item" was wrong — it understated the droop risk. Corrected above. The v1/v2 concern that the 74HC74 is less edge-tolerant than the CD4013BE was correct in itself but is now irrelevant, because HW-043 stops using the clock input. It is also moot if the architecture decision under HW-021 goes the "delete the flip-flop entirely" way — **settle HW-021 first**, because it may delete U1 rather than replace it.
- Notes: Pin-for-pin migration detail (CD4013BE → 74HC74/HCS74) is in `HYDRO-NODE-REFERENCE.md` section 7. The two parts are both 14-pin dual D flip-flops but the pinouts are **not** compatible, and the set/reset polarity is **inverted** (CD4013 SET/RESET are active HIGH; 74HC74 PRE/CLR are active LOW). It is not a socket swap.

---

### HW-042 — Latch supply has no hold-up; a TX droop can switch the device off permanently in the field
- Severity: MAJOR
- Status: OPEN
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

### HW-043 — Change the latch from edge-triggered toggle to asynchronous set/reset
- Severity: MAJOR
- Status: IN DISCUSSION
- Component / net: U1, S1, R3, C1, R4, C2, and two new MCU nets
- Problem: The latch is currently a **toggle**: every rising edge on CLOCK inverts the state. That is why bounce is dangerous (an even number of edges leaves the device in the state it started in, per HW-014), why the reed needs a low-impedance pull-up that costs 360 µA while a magnet is present, and why there is no way to tell what state the device is in. The stated product requirement is that the Node **must not be able to turn off unintentionally**, and a toggle is the one topology that cannot guarantee that.
- Impact: Serves FR-6 and NFR-5 properly, and removes three separate open problems at once rather than patching each.
- Recommended fix: Use the flip-flop as an **SR latch** instead of a toggle. Tie 1CLK and 1D to GND (unused — do not leave them floating), then:
  - **1PRE (active low) ← the reed.** Magnet present pulls PRE low → Q high → device **ON**. Pure hardware; works before any firmware exists and independently of firmware state.
  - **1CLR (active low) ← an MCU pin** through 100 kΩ. **OFF becomes firmware-only**, after firmware has validated a deliberate magnet gesture.
  - **Reed sense → a second MCU pin** through 100 kΩ, so firmware can see the magnet.

  What this buys:
  1. **Accidental turn-off becomes impossible.** Every magnet event does exactly one thing — turn the device on. There is no toggle to fall out of sync, and no "did it switch an even number of times?" ambiguity.
  2. **The debounce problem disappears.** PRE is a *level* input, not an edge input. Five contact bounces assert ON five times and the answer is still ON. The Schmitt buffer from HW-014 is deleted, R3 is deleted, and the whole input-transition-rate question that drove HW-037 and part of HW-041 becomes irrelevant — asynchronous inputs have no transition-rate requirement.
  3. **Reed current drops roughly 100×.** A level input tolerates a high-impedance source, so the reed can pull down against a **1 MΩ** pull-up instead of the present 10 kΩ. A magnet accidentally left on the enclosure now costs **~3.6 µA instead of ~360 µA** — that was a real threat to NFR-1 and this removes it.
  4. **Every fault mode fails toward ON.** With both PRE and CLR low (the datasheet's "invalid" state) the 74HC74 drives Q high — the device stays on. An MCU reset leaves the shutdown pin as a hi-Z input, so a crash or brownout cannot shut the device down. For a product whose worst outcome is "silently off on a roof", that is the correct direction to fail.
  5. **Real user feedback, and two features for free.** Touch magnet → on immediately, LED confirms. To turn off: hold the magnet, the LED counts down, a long blink at 3 s means "armed", remove the magnet → firmware waits for the reed to open and then asserts CLR. Release early and nothing happens. The same sense line then gives you the 10-second hold for local unpair (**HW-022**) and firmware-commanded shutdown on critical battery (**HW-021**, **HW-025**).
- Recommended fix — interlock: PRE and CLR must never be low simultaneously in normal operation. Firmware reads the reed sense line and only asserts CLR once the reed reads open. Keep the 100 kΩ in series with the MCU's CLR drive so that even if the interlock is violated nothing is stressed — and note that the resulting state is Q high, i.e. the device stays on, which is the safe outcome.
- Recommended fix — parts delta: **add** a Schottky and a 10 µF (shared with HW-042), a 1 MΩ pull-up, a 1 kΩ reed series resistor, and two 100 kΩ MCU interface resistors. **Delete** R3 and the Schmitt buffer that HW-014 would have needed. Reuse C1 as the PRE-node filter and C2/R4 as the power-on-reset, with the network inverted for the active-low CLR (see `HYDRO-NODE-REFERENCE.md` §7.3). Roughly break-even on part count.
- Notes: Needs two MCU pins. A0–A5 are all free (HW-021), so there is no contention.
- Notes: **Question for you — confirm the turn-off gesture before I write it into the Stage 8 firmware spec.** Is "hold 3 s, LED countdown, remove magnet to commit" the behaviour you want, or would you rather it commit at the end of the hold without needing the magnet removed? The former is safer and gives a free interlock; the latter is one fewer step for the installer.

---

## RESOLVED / WON'T FIX

*(Nothing resolved yet — this is the initial review.)*

---

## CHANGELOG

| Version | Date | Change |
|---|---|---|
| v3 | 2026-08-19 | Added HW-042 (latch rail hold-up — TX droop can switch the device off permanently) and HW-043 (move the latch from edge-triggered toggle to asynchronous PRE/CLR). HW-041 raised MINOR → MAJOR: your droop argument is the real justification, not the characterisation gap; 74HC74 confirmed as the part. HW-021 DECIDED — hardware latch stays, MCU-sleep alternative rejected. HW-014 contact-wear argument withdrawn (~5 operations in service) and its Schmitt buffer superseded by HW-043. |
| v2 | 2026-08-19 | Added HW-041 (logic family for U1: CD4013BE vs 74HC74 vs SN74HCS74; SN74HCS74 recommended). Updated HW-014 notes — the Schmitt-buffer requirement is now conditional on the HW-041 outcome. Updated HW-037 notes. |
| v1 | 2026-08-19 | Initial Stage 0 hardware review — 40 issues found (6 BLOCKER, 26 MAJOR, 8 MINOR). Netlist extracted directly from `Hydro-Node.SchDoc`; PCB stackup and routing extracted from `Hydro-Node.PcbDoc`. Reed latch topology traced and verified logically correct (recorded under HW-014). |
