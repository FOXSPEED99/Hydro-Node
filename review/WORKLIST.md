# Hydro Node — worklist

**Is the PCB ready to export Gerbers? Not yet — but the gap is finite, and most of it is half an
hour in Altium.**

Reframed around the actual goal: not *is this repo production-ready*, but *can you hit export
Gerbers*. IDs match `HYDRO-NODE-PRODUCTION-REVIEW-v2.md`, which keeps the evidence — the extracted
netlist, pad tables, design rules and the power budget. This file is the working list; that one is
the reference.

| Group | Count |
|---|---:|
| A · Decide first | 6 |
| B · Before Gerbers | 11 |
| C · Same pass | 16 |
| D · Not a PCB change | 6 |
| E · Dropped | 4 |

---

## A · Decide first

These six change what gets drawn, so they come before any layout work.

### `HN-06` — Pro Mini, or ATmega directly on the board?

**Problem.** Keeping the module means removing three things on every single unit: the power LED, the regulator, and the D13 LED — and D13 is your SCK line to the Ra-02, so that one isn't optional. Clone Pro Minis also vary in regulator, LED resistor and fuse settings between batches.

**Fix.** Decide now. On-board ATmega costs roughly eight extra parts (MCU, 8 MHz crystal, two load caps, ISP header, decoupling) and gives you fuse control, a known sleep floor and zero rework. Keeping the module is fine for a first batch — but then write the rework down as a documented step, not tribal knowledge. *(changes the whole layout)*

### `HN-03 + HN-04` — Battery architecture: one cell plus an HLC, or two cells plus diodes?

**Problem.** Two LS14500 in parallel with nothing between them means a stronger cell can charge a weaker one, and Li-SOCl₂ must never be charged. Separately, neither arrangement solves the pulse problem above.

**Fix.** Run the scope test first. If you need the HLC anyway, go single cell + HLC + series Schottky + fuse — that closes both findings at once and is the standard arrangement. If the test passes, you still need a blocking diode per cell and a fuse or PTC in the pack lead. *(schematic change either way)*

### `HN-05` — Is there a breakout adapter under the Ra-02?

**Problem.** The footprint is a 2×8 header at 2.54 mm pitch with 25.4 mm between rows. A bare Ra-02 is SMD-16 at 2.0 mm pitch on roughly 17 mm rows. Those are not the same part.

**Fix.** If you're buying an adapter board, say so and I'll close this — it just needs a BOM line. If you're soldering the module straight down, the footprint has to be redrawn to the real pad geometry. *(either a BOM line or a new footprint)*

### `HN-07` — Which country is this deployed in?

**Problem.** The Ra-02 is a 433 MHz part at +18 dBm, which is 63 mW. In ITU Region 1 the 433 MHz band is generally capped at 10 mW ERP.

**Fix.** Doesn't touch the PCB — the antenna is on an IPEX pigtail. But it fixes your legal TX power, which fixes the spreading factor, which is two-thirds of your active energy budget. Answer it before firmware is finished. *(no layout impact)*

### `HN-23 + HN-41` — Low-side switching, or move to high-side?

**Problem.** Q1 switches ground, so with the node off, VCC stays applied to the ATmega and the SX1278 while their ground floats up to meet it. I traced it and the off state is genuinely clean — but it's clean by accident, and neither datasheet characterises that condition.

**Fix.** A P-channel high-side switch removes the question entirely for the same cost and the same part count. It's the biggest topology change on this list, so it's your call — the current arrangement does work. *(significant re-route if you change it)*

### `HN-29` — All components on one side?

**Problem.** C4, C7, C8, C9, LS1 and BATT1 are on the bottom, everything else on top. That's two separate hand-solder operations and rules out wave or selective soldering.

**Fix.** Moving them all to the top costs board area — you have 80 × 60 mm and going to MLCC bulk (item C1) frees up a lot of it. Worth doing if you'll ever build more than a handful. *(area trade-off)*

---

## B · Must change before Gerbers mean anything

Defects in the files as they stand. Nine of the eleven are quick mechanical edits.

### `HN-09` — Layer stack is still Altium's default

**Problem.** Dielectric 1 is 12.6 mil, giving a 0.39 mm finished board. That board carries a TO-220, a DIP-14, three 11 mm radial cans, four JST connectors and two plug-in modules.

**Fix.** Design → Layer Stack Manager, set Dielectric 1 to 1.5 mm so the total lands at 1.6 mm. Most fabs assume 1.6 mm anyway, but don't rely on them catching it. *(2 minutes)*

### `HN-38` — No board outline on a mechanical layer

**Problem.** The board shape exists only as Altium's intrinsic shape. The Keep-Out layer is empty and Mechanical 1 has no primitives. Your fab needs an outline layer in the Gerber set.

**Fix.** Draw the outline on Mechanical 1 (or whichever layer your fab asks for) and include it in the output job. *(5 minutes)*

### `HN-28` — Silkscreen falls off the board in three places

**Problem.** C4's “100nF” sits 8.7 mm past the right edge, “3.6v” starts past it, and C5's “100nF” is 2.4 mm above the top edge — all will be clipped. Separately, several value legends have drifted 20–40 mm from their part; C2's is 39 mm away.

**Fix.** Reposition every designator and comment back onto its own component, then run Tools → Silkscreen check before you export. On a hand-built board the legend is the assembly instruction. *(30 minutes)*

### `HN-30` — Mounting holes have no annular ring

**Problem.** All four corner holes have pad diameter equal to hole diameter, 2.70 mm. That's a plated hole with zero copper ring — most fabs will query it, some will just build it wrong.

**Fix.** Make them non-plated (uncheck Plated, pad size = hole size) if they're just screw clearance, or give them 0.5 mm of annulus if you want them plated for chassis grounding. *(5 minutes)*

### `HN-40` — No revision or date on the board

**Problem.** Nothing on the physical board distinguishes this from revision 1. When you have three prototypes on the bench you will want to know which is which.

**Fix.** Add “HYDRO NODE v2.0 · 2026-09” to the top overlay in a clear area. *(2 minutes)*

### `HN-21` — C7 / C8 / C9 footprint origin is 1.27 m off the board

**Problem.** All three use the same Vault footprint, whose origin sits at roughly (−46500, −48800) mil. The pads are placed correctly, so the copper is fine — but the bounding box, courtyard, 3D body and pick-and-place origin are all a metre away.

**Fix.** Open the footprint, select all primitives, Edit → Set Reference → Center (or Pin 1), and re-place. If you move to MLCC bulk this disappears anyway. *(10 minutes)*

### `HN-20` — C7 won't physically fit its footprint

**Problem.** The footprint is Ø5 mm on a 2.0 mm lead pitch. A 100 µF 25 V WCAP-ATLL is a Ø6.3 mm can on 2.5 mm pitch. The leads won't reach.

**Fix.** Either pick a 100 µF part that actually is Ø5 mm / 2.0 mm, or use the right footprint. Moot if you take item C1 and go MLCC. *(5 minutes, or free)*

### `HN-31` — Two unnamed free pads used as vias

**Problem.** BATT+ changes layer through two loose pads at (2066.9, 3159.4) and (1496.1, 1122.0) mil, Ø2.00 mm with Ø0.80 mm holes. Every one of your 33 real vias belongs to GND.

**Fix.** Replace both with normal vias. Free pads carry no designator and will show up as unexplained holes in DFM review. *(5 minutes)*

### `HN-32` — Two different hole sizes for the same connector family

**Problem.** J1 has Ø1.00 mm holes; J2 and J3 have Ø0.90 mm. All three are JST-XH, and JST's drawing specifies Ø1.0 mm.

**Fix.** Standardise all three on Ø1.0 mm. 0.9 mm will go together but it's tight on a 0.64 mm square pin with plating tolerance. *(5 minutes)*

### `HN-34` — No net-class rules, so DRC isn't telling you anything

**Problem.** Every rule in the file is an Altium default. BATT+, BATT− and GND use the same 19.685 mil width as signal nets, and there's no minimum-annular-ring rule at all — which is why HN-30 passed DRC.

**Fix.** Add a POWER net class (BATT+, BATT−, GND) at 1.0 mm minimum, add a Minimum Annular Ring rule at 0.15 mm, and set Hole To Board Edge. Then re-run DRC and believe the result. *(15 minutes)*

### `HN-35` — U1 footprint is missing the A4–A7 pads

**Problem.** The Pro Mini footprint has 30 pads: two 12-pin rows and the 6-pin FTDI header. The 2×2 block carrying A4, A5, A6 and A7 isn't there.

**Fix.** Add the four pads even though those nets are unconnected today. The moment you want I²C — a second sensor, an RTC, a display — you'll need them, and it costs nothing now. *(10 minutes)*

---

## C · Fold into the same pass

The board is open anyway. The first two move hot-case battery life from 2.02 to 4.6 years.

### `HN-12` — Aluminium electrolytics → MLCC

**Problem.** C7, C8 and C9 leak continuously, roughly doubling every 10 °C, lose ESR performance at low temperature exactly when the cell is weakest, and eventually dry out. Together they're about 31 µA of your hot sleep budget.

**Fix.** C7 → 100 µF 10 V 1210 X7R (or two 47 µF). C8, C9 → 10 µF 0805 X7R. Leakage drops below 10 nA and the wear-out mechanism goes away. Also frees the board area you need for item A6. *(half of the 2.02 → 4.6 year gain)*

### `HN-10` — 74HC74 → 74AUP1G74

**Problem.** The 74HC74 is specified at 8 µA quiescent at 25 °C and 80 µA at 85 °C. That's 57 % of your hot sleep budget from one always-on logic chip, and it's more than everything else on the board combined. Revision 1's CD4013B was 1 µA — this was a step backwards.

**Fix.** 74AUP1G74 in SOT-353: 0.9 µA maximum. You only use half the 74HC74 anyway, so a single flip-flop is the right part. Also replaces a DIP-14 with a 2 mm package. *(the other half — and the single cheapest fix here)*

### `HN-11` — All nine 100 nF caps are Z5U

**Problem.** SR215E104MARTR1 is a Z5U dielectric, specified from +10 °C to +85 °C with −56 % tolerance. Your device sits outdoors. Below +10 °C these parts are simply outside their specification, and at −10 °C a Z5U keeps a small fraction of its rating. That hits every bypass plus C11 (power-on reset), C12 (reed debounce) and C2 (flow filter).

**Fix.** X7R throughout — 100 nF 50 V X7R, ideally 0603 or 0805 MLCC rather than radial. Same price, −55 to +125 °C, ±15 %. *(BOM change, footprint change if you go SMD)*

### `HN-14` — Bias resistors are too high for the flip-flop's leakage spec

**Problem.** R14 (2.2 MΩ) holds the clock low and R12 (1 MΩ) holds reset high, against an input leakage spec of ±1 µA. Worst case R14 develops 2.2 V on a pin whose V_IL limit is 0.99 V. It works at room temperature because real leakage is picoamps; it's the classic thing that fails hot.

**Fix.** R12 → 100 kΩ, R14 → 220 kΩ, and R11 → 22 kΩ so the software-off divider still pulls reset to 0.60 V against a 0.99 V limit. Costs microamps only while the reed is actually closed. If you take item C2, the AUP part's 0.1 µA leakage makes this less urgent — but do it anyway. *(3 resistor values)*

### `HN-13` — The clock input sees a 220 ms edge

**Problem.** Releasing the magnet lets C12 decay through R14 with a 220 ms time constant, into a clock input that is not Schmitt-triggered. The family's maximum input transition rate is about a microsecond. It's the inactive edge so it shouldn't clock — but the input sits in its linear region for a fifth of a second, drawing through-current and free to oscillate. If it does, the node toggles when the user pulls the magnet away.

**Fix.** R14 → 220 kΩ (item C4) brings it to 22 ms, which is better but still far outside spec. The clean fix is a 74LVC1G17 Schmitt buffer between the reed node and the clock — under a microamp, five cents, non-inverting so the toggle still happens on magnet approach. *(one extra part)*

### `HN-15` — No reverse-polarity protection

**Problem.** D1 protects U2 only. A reversed battery pigtail takes out U1, U3 and the ultrasonic simultaneously — and on a production line with hand-crimped leads, reversed packs happen.

**Fix.** P-channel MOSFET ideal diode in the BATT+ line: source to the battery, drain to the rail, gate to BATT− through 100 kΩ, with a small Zener gate clamp. About 20 mΩ, essentially zero quiescent current, and unlike a Schottky it doesn't eat 0.3 V you can't spare off a 3.6 V plateau. *(2–3 parts)*

### `HN-16 + HN-46` — Battery and flow connectors are interchangeable

**Problem.** BATT1 and J1 are both 2-pin JST-XH. Plugging the pack into J1 puts 3.6 V onto the flow node and into D5 and A2 through 100 Ω and 330 Ω.

**Fix.** Move the battery to a different family — JST-PH 2-pin, or a locking connector — so the mistake becomes physically impossible. Keep XH for the three sensors. *(footprint swap)*

### `HN-17` — No ESD protection on three cables leaving the box

**Problem.** Three harnesses run across a rooftop to a tank and a fill pipe. D3, D4, D5, D6, D8 and A2 go straight to the MCU behind nothing but a series resistor. The 1-Wire line is the classic victim — long, high impedance, pulled up.

**Fix.** A low-capacitance TVS array to GND at each connector, under 5 pF on the echo and 1-Wire lines so you don't slow the edges. Put the TVS ground straight into the pour, close to the connector — not routed halfway across the board. *(3 small parts)*

### `HN-18 + HN-43` — No way to program or probe the board

**Problem.** U1's carrier pads for DTR, TXO and RXI have no net, and the module's own FTDI pins are consumed by the carrier. There is no way to reflash a sealed rooftop unit, and no test points anywhere.

**Fix.** Route DTR, TXO and RXI to a 6-pin 0.1 in header. Add test points on BATT+, BATT−, GND, the clock node, the reset node and the Q1 gate — 1 mm pads cost nothing and save you an afternoon every time something misbehaves. *(1 header, 6 pads)*

### `HN-19 + HN-44` — No battery telemetry

**Problem.** A0 and A3–A7 are unconnected and there's no divider to any ADC. Li-SOCl₂ has a famously flat discharge curve so open-circuit voltage tells you little — but the loaded voltage during a transmit burst is an excellent health signal, and it's how you'd catch the HN-03 problem in the field instead of guessing.

**Fix.** Two 1 MΩ resistors from BATT+ to an ADC pin, with the bottom of the divider switched by a GPIO so it draws nothing in sleep. Sample it mid-transmission. *(3 parts, 1 spare pin)*

### `HN-33` — The flow signal lands on two MCU pins

**Problem.** The flow node goes to D5 through R5 (100 Ω) and also to A2 through R3 (330 Ω). With a plain 1 MΩ pull-up, A2 can't distinguish an open cable from an open switch — both read high — so it buys you no diagnostic. And if firmware ever drives either pin, they fight through 430 Ω.

**Fix.** Delete R3 and free A2. Or, if you want cable diagnostics, replace R6 with a divider so a healthy open switch reads a mid-scale voltage and a broken cable reads full rail — then A2 earns its place. *(delete one part)*

### `HN-36 + HN-42` — Decoupling is too far from what it feeds

**Problem.** The nearest bypass to U1's VCC pin is C5 at 16.7 mm. C9 — the flip-flop's hold-up reservoir, the entire point of isolating it behind D1 — is 26.3 mm away and on the opposite layer. Separately, the always-on supply net runs 114 mm across the board to reach the reed switch, beside a 100 mW transmitter.

**Fix.** Move a 100 nF within a few mm of U1's VCC and U2's VCC, move C9 next to U2, and add 100 pF at the flip-flop's clock pin for RF immunity. Shorten the reed net while you're re-routing. *(placement, no new parts)*

### `HN-22` — IRLZ44N is a 47 A TO-220 switching 130 mA

**Problem.** It works, but V_GS(th) runs up to 2.0 V and R_DS(on) isn't characterised at 3.3 V gate drive. Mostly it's just enormous — a TO-220 with an M3 screw pad on a board where the load is a tenth of an amp.

**Fix.** Any small logic-level FET in SOT-23 rated an amp or more with V_GS(th) under 1 V. If you go high-side per item A5, a dedicated load-switch IC is even simpler. *(frees real board area)*

### `HN-26` — The buzzer will be quieter than the datasheet suggests

**Problem.** CPT-1255C-090 is an externally-driven piezo transducer, and its 80 dB rating is at 20 V peak-to-peak. At 3.3 V p-p you get roughly 64 dB — through a PETG wall, outdoors, on a roof. Also: firmware has to generate the tone. A static high on D7 produces silence.

**Fix.** Test it by ear through the actual enclosure first. If it's too quiet, drive it push-pull from two GPIOs in antiphase for +6 dB, or add a small inductor-based step-up. Don't fix it until you've heard it. *(test, then decide)*

### `HN-25` — The flow switch is being run as a dry contact

**Problem.** The WY-90 is rated DC 12–24 V and you're switching it at 3.6 µA through R6's 1 MΩ. Sealed reed contacts usually cope, but the vendor doesn't qualify dry-circuit operation, so it's unqualified rather than known-bad.

**Fix.** If it proves intermittent in the field, drop R6 to 100 kΩ — 36 µA, and only while the tank is actually filling, so the cost to the budget is negligible. Cheap insurance you can decide later. *(one resistor value)*

### `HN-01` — Put the J3 signal names on the silkscreen

**Problem.** Your crossed harness is deliberate and electrically correct — but it exists only in your head. A cable with the right colours and straight-through wiring looks identical at inspection and puts BATT+ on the module's trigger input.

**Fix.** Print GND / VCC / ECHO / TRIG next to J3 on the overlay, and write the wire list down as a one-page controlled drawing. Alternative worth ten seconds' thought: reorder J3 to GND · Trig · Echo · VCC to match the module, and then a straight cable gives you black-on-ground and red-on-power with no crossing at all. *(silkscreen edit)*

---

## D · Real, but not a PCB change

Firmware, harness, sensor assembly and mechanical. None of it blocks the export.

### `HN-45` — Write the firmware energy contract first

**Problem.** With the hardware as drawn, energy-aware firmware gives 5.7 years and naive Arduino firmware gives 1.5. Firmware is now the deciding variable for battery life, not the board.

**Fix.** Budget it explicitly: ≤ 6.6 mAs per wake cycle, ≤ 62 ms airtime, DS18B20 at 9-bit resolution not 12, MCU in idle-sleep during the conversion rather than delay(), every unused pin configured, BOD off, and SCK driven low before sleep. *(before firmware, not before Gerbers)*

### `HN-27` — 100 nF at the DS18B20 itself

**Problem.** The sensor is pin-powered from D3 down about a metre of cable with no local bypass. Maxim's datasheet asks for 100 nF at the device.

**Fix.** Put the capacitor inside the sensor's heatshrink, across its VDD and GND pins. It doesn't belong on the PCB — a metre of cable away is not “local”. *(harness assembly)*

### `HN-24` — The ultrasonic module is a bare uncoated PCB

**Problem.** Its transducers are soldered to an uncoated board that will sit in a headspace at or near 100 % RH, condensing every time the tank refills or the roof cools. Corrosion on uncoated copper in condensing conditions is months, not years. (The supply voltage is fine — the module is specified 3–5 V.)

**Fix.** Either conformally coat or pot the module, leaving the transducer faces clear, or keep the electronics inside the sealed enclosure and bring only the transducers into the headspace through a potted feedthrough. A small drip shield over the faces helps either way. *(mechanical design)*

### `HN-39` — Bare glass reed switch on long unsupported leads

**Problem.** S1 is a 4 × 29 mm glass body on a 35 mm lead span, 3.75 mm from the board edge, going onto a roof that thermally cycles daily.

**Fix.** Specify a plastic-encapsulated reed, or bond the glass body down with a flexible adhesive so the leads aren't carrying it. *(part choice)*

### `HN-37` — Flip-flop part number mismatch

**Problem.** The parts list says 74HC74AP, which is the Toshiba part. The schematic uses NXP 74HC74N,652.

**Fix.** Moot if you take item C2 and move to the AUP part — but whichever you pick, make the list and the schematic agree. *(BOM tidy)*

### `HN-49` — Consider a gate pin for the ultrasonic — for recovery, not power

**Problem.** At 1.5 µA standby there's no power argument for gating it. But it also means firmware can never power-cycle the module if it latches up, and these modules do occasionally hang.

**Fix.** Optional. If field experience shows it hanging, one GPIO through a small high-side FET solves it. Not worth spending board area on speculatively. *(optional)*

---

## E · Dropped

Two because I was wrong, two because I judged a working repo as if it were a release.

### `HN-02` — “The ultrasonic supply is not gated”

**Problem.** I estimated ~2 mA continuous standby and built the headline verdict on it. That estimate came from JSN-SR04T and HC-SR04 class boards, which is the wrong reference class for an RCWL-16xx part.

**Fix.** Withdrawn. The RCWL-1670 is specified at 1.5 µA at 3.3 V and 3.5 µA at 5 V standby. Leaving it on BATT+ costs about 2 µA. There is no reason to gate it. *(my error)*

### `HN-47` — “Settle the ultrasonic's supply voltage; consider boosting to 5 V”

**Problem.** I read the +5V on the module's silkscreen as a requirement.

**Fix.** Withdrawn. The module is specified 3–5 V, so 3.6 V is comfortably in range, and its 2 cm blind zone and 4 m range suit a rooftop tank well. The +5V marking is just a pad label. *(my error)*

### `HN-08` — “No manufacturing data set”

**Problem.** I judged the repository as if it were a release, and listed the absence of Gerbers, drill files, an OutJob, a DRC report, a project file and the libraries as a blocker.

**Fix.** Withdrawn as a finding. This is a working share so I can read the design, and the Gerbers aren't exported yet by design — that's the decision this list exists to inform. It stays on the checklist as an output step, not a defect. *(my framing error)*

### `HN-48` — “Restore the deleted issue tracker”

**Problem.** Same reason — I treated a scratch repository as a controlled record.

**Fix.** Withdrawn. Worth keeping the design rationale somewhere you'll find it in a year, but that's housekeeping, not a review finding. *(my framing error)*

---
