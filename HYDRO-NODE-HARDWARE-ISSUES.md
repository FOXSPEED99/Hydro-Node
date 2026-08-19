# HYDRO NODE — HARDWARE ISSUE TRACKER
Version: v14   |   Last updated: 2026-08-19   |   Status: Stage 0 review — HW-003 decided (two cells, isolated); 2 blockers pending implementation

## STATUS SUMMARY
Total issues: 52   |   Open: 48   |   Resolved: 3   |   Won't fix: 1
Blockers remaining: 2
Production-ready: NO — the two Li-SOCl₂ cells are hard-paralleled without blocking diodes, and the PCB has no ground plane.

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

### HW-007 — Ra-02: only one of its three GND pins is connected
- Severity: MAJOR
- Status: OPEN
- Component / net: J1 pins 1 and 2, J2 pin 1 (all GND), J2 pin 8
- Problem: Extracted from the schematic netlist: the module's ground returns to the board through **J2 pin 8 only**. J1.1, J1.2 and J2.1 are left unconnected. The module's 120 mA TX return current therefore takes one long 1 mm trace.
- Impact: Supply droop and ground bounce at the RF module during TX, reduced TX power and RX sensitivity, and a large radiating current loop. Compounds HW-004.
- Recommended fix: Connect all three GND pins to the ground pour. In a 2-layer respin, tie them to the pour with a short, wide connection and stitch around the module footprint.
- Notes: Also connect J1 pin 4 (RST) — already done via D9 — and consider bringing DIO1 out to a spare pin; some LoRa stacks use DIO1 for RX-timeout and CAD-done, which the pairing protocol (Stage 6) may want.

---

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
- Notes (v6): The arithmetic is now definitive, and it also answers the "one cell plus a big capacitor" proposal under HW-003. A 60 ms TX burst at 120 mA moves **7.2 mC**. Into 470 µF that is a 15 V collapse; into 2200 µF, 3.3 V; you would need **36,000 µF** to hold the rail within 0.2 V. No bulk capacitor you can fit in this enclosure carries a LoRa transmission. C3's only real function is smoothing the first few milliseconds of the current edge, and a few hundred µF of ceramic does that better than 2200 µF of leaky aluminium. Only a **supercapacitor** (0.1 F and up, low ESR, behind a series charge resistor) can actually carry the burst — see `HYDRO-NODE-REFERENCE.md` §10.3.
- Notes: First measure. Put a scope on VBAT during a real TX burst at −5 °C and at 50 °C, on a cell that has been sitting idle for a week (see HW-032). If the dip is acceptable, you may not need any pulse-support part at all and can delete C3 outright, which is the cheapest possible fix.

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

---

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
- Notes (v4): The Schmitt buffer is **required again** — the v3 PRE/CLR proposal that would have removed it is withdrawn (see HW-043). This issue is now fully absorbed into **HW-043**, which specifies the buffer, the inverted reed connection and the new RC values. The contact-wear withdrawal below still stands.
- Notes (v3, superseded): **Largely superseded by HW-043.** If the latch moves from edge-triggered CLOCK to asynchronous PRE/CLR, contact bounce becomes harmless (each bounce simply re-asserts ON) and the Schmitt buffer is deleted. The series resistor is still worth fitting because it costs nothing, but the **contact-wear argument in this issue is withdrawn** — at roughly 5 operations over the product's life, hot-switching into C1 is not a wear concern. You were right to push back on that.
- Notes (v2): The Schmitt buffer requirement is now coupled to the U1 part choice — see **HW-041**. If U1 becomes an **SN74HCS74**, every input is already Schmitt-triggered with no transition-rate requirement, and the separate buffer is deleted (the series resistor and the RC are still required). If U1 stays **CD4013BE** or becomes a plain **74HC74**, the buffer is mandatory — and more so for the 74HC74, which is the least tolerant of slow edges of the three.
- Notes: **Verification of the latch as drawn, per your Section 6 item 6.** I traced it from the extracted netlist: U1 is wired as a T-flip-flop (D1 ← Q1̄, pin 5 ← pin 2), CLOCK1 ← reed node, Q1 (pin 1) → R2 1 kΩ → Q1 gate, R1 1 MΩ gate pulldown to the raw battery return. SET1, SET2, RESET2, CLOCK2, D2 are all correctly tied to VSS; the unused half's outputs are correctly left open. C2 (100 nF from VBAT) with R4 (100 kΩ to VSS) is a correct **active-high power-on-reset** giving ~10 ms — so the device powers up OFF when a cell is first fitted. **Toggle logic is correct and the OFF state is genuinely clean:** with Q1 off, every resistive path (R1, R3, R4) sits at 0 V across it, C2 and C4 are ceramic, and the only OFF-state current is the MOSFET's leakage plus the CD4013's quiescent (< 1 µA at 25 °C). I have no objection to the concept — only to the debounce, the contact protection, and the placement (HW-015).

---

### HW-015 — Reed switch is mounted in the centre of the PCB
- Severity: MAJOR
- Status: OPEN
- Component / net: S1
- Problem: From the PCB layout, S1 sits vertically in the middle of a ~90 × 68 mm board, roughly 30 mm from the nearest edge. The magnet has to actuate it through a PETG-CF wall plus that 30 mm of standoff, and the user has no marked spot to place the magnet against.
- Impact: Unreliable or impossible actuation — the primary and only user control on a sealed device (FR-6, NFR-5). Magnet field falls off steeply; 30 mm of extra distance can easily be the difference between working and not.
- Recommended fix: Move S1 to a **board edge**, oriented along its sensitive axis, as close to the enclosure wall as clearance allows. Mark the spot on the outside of the enclosure (embossed target in the print). Then verify the actual pull-in distance with the production magnet and the production wall thickness, and specify a minimum magnet grade in the BOM.
- Notes: Also see HW-039 — the bare glass reed body is mechanically fragile. If you adopt the Hall-sensor alternative there, this placement problem gets easier because a Hall device is small and can sit right at the wall.

---

### HW-016 — Blue LED has no forward-voltage headroom on a 3.0–3.6 V rail
- Severity: MAJOR
- Status: OPEN
- Component / net: DS1, R5, U2.D8
- Problem: The BOM specifies a **blue** 3 mm LED (confirmed in the component photos), while the schematic symbol is a red LED. A blue LED has a forward voltage of roughly 2.7–3.2 V. The rail is 3.6 V falling to ~3.0 V over the cell's life, and the ATmega output stage drops another ~0.2 V. Through the schematic's 330 Ω that gives ~2 mA when fresh and essentially nothing at end of life — the status indicator goes dark exactly when you most need to know the battery is low.
- Impact: The Node's only local feedback stops working. Compounds HW-014 (ambiguous on/off feedback).
- Recommended fix: Use a **red or yellow LED (Vf ≈ 1.8–2.1 V)**, which leaves 1.0–1.8 V across the resistor across the whole cell life. Size R5 for 2 mA (it only ever flashes briefly), so ~680 Ω–1 kΩ. Make the schematic, the BOM and the fitted part agree.
- Notes (v4): **This is no longer cosmetic.** The LED is now the confirmation mechanism for the magnet on/off control (HW-043, HW-044), so a blue LED going dark at ~3.0 V means the user-facing control loses its feedback exactly as the battery ages — the point at which someone is most likely to be on the roof investigating. Treat the change to a red or yellow part as required, not optional.
- Notes: There is also a **BOM/schematic value mismatch** here — the BOM lists a 220 Ω resistor, the schematic says 330 Ω. Whichever survives, one document is wrong today.

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

### HW-018 — Echo is on D7 instead of D8, so hardware input capture is unavailable
- Severity: MAJOR
- Status: OPEN
- Component / net: J5 pin 3 (Echo) → U2.D7; U2.D8 → R5/DS1
- Problem: The ATmega328P's **Input Capture pin (ICP1) is PB0 = Arduino D8**, which this design uses for the status LED. Echo went to D7, an ordinary GPIO, so the echo pulse can only be measured in software (`pulseIn` or a pin-change interrupt).
- Impact: `pulseIn` at 8 MHz resolves to roughly ±4 µs, and a pin-change ISR adds 1–2 µs of latency jitter. That is only ±0.7 mm of distance error so it is not fatal — but ICP1 gives a hardware timestamp at 125 ns resolution with zero interrupt-latency jitter, it is completely free, and it lets the MCU sleep during the flight time instead of spinning in a blocking loop (which also saves awake energy). Given NFR-2 explicitly drives this design, leaving a free hardware timer on the table is not defensible.
- Recommended fix: **Swap them — Echo to D8, LED to D7.** One net change in the respin, no cost, no extra parts.
- Notes: This also means the ultrasonic measurement no longer blocks the CPU, which shortens the awake window and helps NFR-1.

---

### HW-020 — Flow input D5 has no external pull-up, no filter and no series protection
- Severity: MAJOR
- Status: OPEN
- Component / net: J4 pin 1 → U2.D5
- Problem: D5 goes straight from the MCU to a connector and out on a cable to the fill pipe, with nothing on it. The design relies entirely on the ATmega's internal pull-up.
- Impact: Three separate problems. (a) If firmware leaves the internal pull-up enabled during sleep while the switch is closed, that is **~110 µA continuous** — on its own about half of the entire allowable average current for the 2-year target. (b) An unfiltered mechanical contact on a long cable will produce chatter and pick up noise. (c) No series protection (see HW-012).
- Recommended fix: Add an **external 1 MΩ pull-up** to the switched rail plus a **100 nF** cap to ground at the connector (giving a 100 ms RC filter, which is fine for a flow signal that changes on a timescale of seconds), and a **100 Ω** series resistor at the MCU pin. 1 MΩ costs 3.6 µA when the switch is closed, and the internal pull-up can then stay off permanently. Combine with the wetting-pulse scheme from HW-019 for the actual sampling.
- Notes: Firmware must still be explicit: internal pull-up **disabled** before sleeping, every time.

---

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
- Notes (v4): With HW-043 withdrawn back to a clock-driven toggle, the 74HC74's weaker slow-edge tolerance **is relevant again** — but it is fully handled by the Schmitt buffer that HW-043 now mandates, which presents a fast edge to CLOCK regardless of the family. The part decision is unchanged: fit the 74HC74 for its 2 V minimum.
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

---

### HW-043 — Reed input needs real debounce; magnet retains full on/off control
- Severity: MAJOR
- Status: IN DISCUSSION
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

---

### HW-034 — C3 voltage rating mismatch between BOM and supplied part
- Severity: MINOR
- Status: OPEN
- Component / net: C3
- Problem: The BOM and schematic both specify 2200 µF **16 V**; the component photo shows a 2200 µF **25 V** part. Electrically the 25 V part is fine, but the can diameter differs (typically 13 mm vs 10 mm), so it may not match the PCB footprint or the enclosure clearance.
- Impact: Fit problem at assembly; documentation does not describe the built article.
- Recommended fix: Moot if HW-009 is adopted and C3 is deleted or replaced with ceramics. If C3 survives in any form, make the BOM, schematic, footprint and purchased part agree, and specify the diameter and lead pitch explicitly.
- Notes: Flagged mainly because it is a symptom — the BOM and the built board have drifted apart in at least three places (this, HW-016's LED colour, and HW-016's resistor value). Worth a full reconciliation pass.

---

### HW-035 — Unused MCU I/O left floating will add sleep current if not configured
- Severity: MINOR
- Status: OPEN
- Component / net: U2 — A0–A7, D0, D1, DTR, and the unconnected Ra-02 DIO lines
- Problem: Nine analogue pins plus the UART pins are unconnected. A floating CMOS input sits near its switching threshold and its input stage draws crossbar current; several floating pins can add tens of microamps.
- Impact: Silent addition to sleep current — precisely the kind of thing that makes a measured power budget disagree with the calculated one.
- Recommended fix: Firmware must explicitly configure **every** unused pin before sleeping — either as an input with the internal pull-up enabled, or as an output driven low. Add this to the Stage 7 checklist and make it a measured pass/fail. It is a firmware fix, but it is recorded here because it is a power-path issue and it will be forgotten otherwise.
- Notes: In the respin, tie genuinely unused pins to ground through pads so the state is defined by hardware rather than by remembering.

---

### HW-036 — Carbon-film ½ W resistors throughout
- Severity: MINOR
- Status: OPEN
- Component / net: R1–R6
- Problem: All six resistors are ½ W axial carbon film (±5 % typical, and a temperature coefficient in the −200 to −1000 ppm/°C region). They are also physically large for a board that should be moving to SMD.
- Impact: Low, in this circuit — none of these resistors is in a precision path. R6 (the 1-Wire pull-up) and R3/R4 (the RC networks) all tolerate ±5 % easily. The real cost is size and assembly method.
- Recommended fix: Move to **0603 1 % metal-film** in the respin, for placement by machine and for a defined tempco. Not urgent on its own — bundle it with HW-026.
- Notes: Recorded for completeness; this is the lowest-priority item on the list.

---

### HW-037 — CD4013BE in a plastic DIP
- Severity: MINOR
- Status: OPEN
- Component / net: U1
- Problem: The CD4013BE is a 14-pin plastic DIP. If it is socketed (as the through-hole build style suggests), the socket contacts oxidise over years in a humid, thermally-cycling outdoor enclosure, and vibration can back the part out.
- Impact: An intermittent contact in the power-latch IC means the device randomly turns off, or fails to turn on, in the field.
- Recommended fix: Use the **SOIC-14 version soldered directly** (CD4013BM or equivalent). If U1 survives the architecture decision under HW-021 at all, do not socket it. Confirm the quiescent current and the operating temperature range against your chosen vendor's datasheet — the family covers −55 °C to +125 °C and 3–18 V, which is comfortable here, but I want the specific part's Iq over temperature in the power model rather than an assumption.
- Notes: I also want to check one specification I could not retrieve during this review: most CD4013B datasheets state a **maximum clock input rise/fall time** (I believe around 15 µs at V_DD = 5 V, but treat that as unverified until you or I read the vendor datasheet). As drawn today the *active* rising edge at CLOCK1 is fast, so the spec is probably not violated — but the moment you add the series resistor from HW-014 it will be, which is exactly why HW-014 also calls for a Schmitt-trigger buffer. **Please confirm the number from your part's datasheet.**
- Notes (v2): Partly superseded by **HW-041**. The package question here (SOIC, not socketed DIP) stands regardless of which logic family wins.

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

---

### HW-046 — Check the Pro Mini for a D13 LED; D13 is the LoRa SPI clock
- Severity: MINOR
- Status: NEEDS INFO
- Component / net: U2 D13, J2 pin 5 (SCK)
- Problem: D13 on this design is **SCK for the Ra-02 SPI bus**. Many Arduino-compatible boards fit an LED plus series resistor on D13. If your module has one, it is across the SPI clock line.
- Impact: Two effects, neither fatal but both worth removing. The LED loads the clock edge and adds capacitance to the highest-frequency net on the board, and it draws current on every SPI transaction — roughly a milliamp during each clock high, throughout every transmission, several times per wake for the whole life of the product.
- Recommended fix: Inspect one of your modules. If a D13 LED is fitted, remove it along with the power LED and regulator you have already taken off (HW-002) — same rework step, no extra cost. If it is not fitted, close this issue.
- Notes: The genuine SparkFun Pro Mini is generally fitted with a power LED only, but clone modules vary between batches and this is worth two minutes with a magnifier. **Tell me what you find and I will close or action it.**

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


## CHANGELOG

| Version | Date | Change |
|---|---|---|
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
