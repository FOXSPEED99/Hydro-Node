# Hydro Node — Production-Readiness Review (design revision 2)

Reviewer: independent hardware review
Date: 2026-09-01
Scope: full repository at commit `5f529a0`
Revision 2 of this review, 2026-09-01 — issued after designer feedback. Two findings were
corrected against vendor data: **HN-01 is downgraded to MAJOR** (the crossed harness is a
deliberate, colour-driven build convention) and **HN-02 is withdrawn** (the RCWL-1670's standby
current is 1.5 µA @ 3.3 V, not the milliamp figure originally assumed). The power verdict changes
materially as a result — see §6.

Source of truth: the Altium files in this repository. Every finding below cites a
designator, net, pad coordinate, layer, rule or file path extracted programmatically
from `Hydro_Node_Schematic.SchDoc` and `Hydro_Node_PCB.PcbDoc`, not read off a render.

---

## 1. Executive summary

**Verdict: NO-GO for manufacturing. Conditional GO for firmware.**

- **NO-GO for manufacturing** — unconditional. There is no manufacturing data set in the
  repository at all, and seven blocking design defects remain open.
- **Conditional GO for firmware** — start now. Three parameters are still open (band/SF,
  pulse-support part, Pro Mini vs. on-board ATmega), but none of them blocks writing the sensor
  drivers, the sleep state machine or the packet format. This is an upgrade from the first issue
  of this review, and it follows directly from the two corrections below.

### Corrections against the first issue

**HN-01 — downgraded from BLOCKER to MAJOR.** The original finding assumed a straight-through
1:1 harness between J3 and the RCWL-1670. The designer has confirmed the harness is deliberately
crossed and wired by conductor colour (red → +5V, black → GND), which is a valid resolution: the
connection is electrically correct and the build convention is intentional. What survives is a
documentation finding, not a functional one — the harness is not a controlled part anywhere in the
repository, and a correctly-coloured but straight-wired cable will look right and be wrong. See
HN-01 for the residual, and for a cheaper alternative that removes the crossing entirely.

**HN-02 — withdrawn.** The original finding rested on an estimate of ~2 mA continuous standby for
the RCWL-1670. That estimate was wrong: it was anchored on JSN-SR04T / HC-SR04-class boards, which
is the wrong reference class. The RCWL-1670 is an RCWL-16xx-family part specified at
**1.5 µA @ 3.3 V and 3.5 µA @ 5 V standby**, 6 mA working, 3–5 V supply, 50 ms measurement cycle.
Leaving it permanently powered on `BATT+` costs roughly **2 µA** — a rounding error against a
297 µA ceiling. There is no reason to gate it, and the arithmetic that follows from the ceiling was
never the problem; the input to it was.

### What still decides the verdict

1. **Nothing in the design supports a Li-SOCl₂ pulse.** C7 (100 µF) supplies **0.4 %** of the
   charge in one LoRa TX burst. Holding the rail within 0.3 V across a 62 ms SF7 burst needs
   ≈ 25,000 µF, or — correctly — a hybrid-layer capacitor. There is neither, and no depassivation
   strategy for cells that will sit at ~14 µA for weeks between transmissions.
2. **Two LS14500 primary lithium cells are hard-paralleled** through a single 2-pin `BATT1`, with
   no per-cell blocking diode and no fuse. A stronger cell charges a weaker one, and Li-SOCl₂ must
   never be charged. This is a safety and certification blocker, not an engineering preference.
3. **The board as drawn cannot be built.** U3's footprint is 2.54 mm pitch on 25.4 mm rows — that
   is not the Ra-02, and no adapter appears in the BOM. The layer stack is Altium's untouched
   12.6 mil default, giving a 0.39 mm board. And there is no Gerber, drill, OutJob, DRC report,
   assembly drawing or MPN-level BOM anywhere in the repository.

### The power verdict, corrected

**With the RCWL-1670's real standby figure, the two-year target is met — but only with
energy-aware firmware, and at temperature it is met with essentially zero margin.**

| Scenario | Average | Life | |
|---|---:|---:|---|
| Energy-aware firmware, 25 °C | 69 µA | **5.7 years** | PASS |
| Energy-aware firmware, hot (60–85 °C) | 196 µA | **2.02 years** | PASS, no margin |
| Naive Arduino firmware, hot | 391 µA | 1.01 years | **FAIL** |
| Energy-aware + 74AUP1G74 + MLCC bulk, hot | 86 µA | **4.6 years** | PASS with margin |

The single dominant term is now **U2**. At its 85 °C limit the 74HC74 draws 80 µA — **57 % of the
hot sleep budget**, more than everything else on the board combined. HN-10 and HN-12 are what buy
the margin back, and they are the cheapest fixes on the list.

**Credit where due.** This is design revision 2. A prior review (`HYDRO-NODE-HARDWARE-ISSUES.md`,
40 issues, dated 2026-08-19, recoverable from commit `890c7df` but deleted at HEAD) drove real
fixes that I independently verified as landed: the ground pours, the Ra-02's four ground pins,
Echo moved onto D8/ICP1, the reed switch moved to the board edge, per-line series resistors,
proper local decoupling, and the connector function names on the silkscreen. §8 lists what was
verified sound.

---

## 2. Repository inventory

| Path | What it is | Reviewed |
|---|---|---|
| `Hydro Node Parts & Schematic/Components-List.txt` | 25-line parts list, no MPNs | Yes |
| `.../Schematic/Hydro_Node_Schematic.SchDoc` | Altium schematic, 1 sheet, 1,665 records: 37 components, 138 pins, 79 wires, 51 junctions, 3 net labels | Yes — netlist extracted in full |
| `.../Schematic/Hydro_Node_PCB.PcbDoc` | Altium PCB, 2 layers, 80.0 × 60.0 mm, 37 components, 140 pads, 33 vias, 2 polygons, 910 track primitives, 29 nets | Yes — pads, tracks, vias, rules, stackup, polygons, silkscreen extracted |
| `.../Components Images/` | 25 product photos | Yes — used as primary evidence for HN-01, HN-24, HN-25, HN-26 |
| `.../Schematic/__Previews/` | Altium render cache | N/A |

**Absent from the repository** (each is a gate on manufacturing):

- No firmware, and no interface/register contract for the Hub.
- No `.PrjPcb` project file — the schematic and PCB are not formally linked. I verified they are
  in sync independently: both carry 29 multi-pin nets with identical membership.
- No `.SchLib` / `.PcbLib`. The documents reference `Ra-02.SchLib`, `Ra-02.PcbLib`,
  `IRLZ44N.SchLib`, `ReedSwitch.PcbLib`, `CPT-1255C-090.PcbLib`, `ARDUINO_PRO_MINI.IntLib` and
  several Altium Vault items — **none are in the repository**, so the design cannot be rebuilt
  or re-verified by anyone else.
- No Gerbers, drill files, ODB++, OutJob, assembly drawing, pick-and-place or DRC report.
- No BOM with manufacturer part numbers, tolerances, dielectrics, temperature ratings or
  approved sources.
- No enclosure model, no antenna design files, no test plan, no requirements document.

**Design intent vs. repository — discrepancies flagged:**

| Brief says | Repository shows |
|---|---|
| "Arduino Pro Mini with the onboard voltage regulator and power LED removed" | Nothing anywhere records this rework. `Components-List.txt` says only "1x Arduino Pro Mini 3.3v". See HN-06. |
| "custom-designed antenna" | Nothing. `Components-List.txt` says "1x IPEX to SMA Cable + Antenna". No design files, no band, no VSWR. See HN-07. |
| "a short buzzer beep as power-up feedback" | LS1 is `CPT-1255C-090`, an externally-driven piezo transducer, not a self-driving buzzer. See HN-26. |
| flow switch, unspecified | `Components Images/Flow-Switch.png` shows a **WY-90, DC 12–24 V** reed flow switch. See HN-25. |
| "2× LS14500 in parallel" | Confirmed — a single 2-pin `BATT1` (JST B2B-XH-A), so the cells are hard-paralleled off-board. See HN-04. |

---

## 3. How the board actually works (extracted netlist)

Recovered from `Hydro_Node_Schematic.SchDoc` by geometric netlist extraction (pin endpoints,
wire segments, junctions, net labels). Three power domains:

**`BATT-`** — battery negative, the always-on reference.
`BATT1.1, C4.1, C9.2, C10.2, C11.2, C12.2, Q1.3(S), R8.1, R14.2, U2.7(GND), U2.11(2CP), U2.12(2D)`

**`BATT+`** — battery positive, permanently connected to every load's supply pin.
`BATT1.2, C1.1, C3.2, C4.2, C5.2, C6.1, C7.1, C8.1, D1.2(A), J3.2, R6.1, U1.VCC, U1.VCC_1, U3.3(3.3V)`

**`GND`** — the *switched* return, downstream of Q1. Poured solid on both layers, 33 stitching vias.
`C1.2, C2.2, C3.1, C5.1, C6.2, C7.2, C8.2, J1.1, J2.2, J3.1, LS1.N, Q1.2(D), U1.GND×2, U1.GND_1, U1.GND_2, U3.1, U3.2, U3.9, U3.16`

**Master power latch** (always-on domain, powered through D1 from `BATT+`):
U2 (74HC74) is wired as a T flip-flop — `1D` tied to `1~Q`, clock `1CP` from the reed switch S1
through R13 (100 Ω) with C12 (100 nF) and R14 (2.2 MΩ) to `BATT-`; `1Q` drives Q1's gate through
R10 (1 kΩ), with R8 (1 MΩ) holding the gate down. `1~RD` carries a power-on reset (R12 1 MΩ / C11
100 nF, τ ≈ 100 ms) **and** a software-off path from the MCU's A1 pin through R11 (100 kΩ).
The unused second flip-flop is correctly tied off.

**Load switch:** Q1 (IRLZ44N) is a **low-side** switch — drain on `GND`, source on `BATT-`. Body
diode correctly oriented to block load current.

**Sensors:**
- **J3 (4-pin, silkscreen "Ultrasonic"):** `1=GND`, `2=BATT+`, `3→R1(100 Ω)→D8`, `4→R2(100 Ω)→D6`.
- **J2 (3-pin, "Temp"):** `1=DQ→R4(100 Ω)→D4`, `2=GND`, `3=VDD`, driven **directly from D3** —
  the DS18B20 is pin-powered and genuinely gated. R7 (4.7 kΩ) is the 1-Wire pull-up, correctly
  referenced to the gated VDD.
- **J1 (2-pin, "Flow"):** `1=GND`, `2` = a node with R6 (1 MΩ) pull-up to `BATT+`, C2 (100 nF)
  filter, and **two** MCU connections — R5 (100 Ω) to D5 and R3 (330 Ω) to A2.
- **LS1 (piezo):** `P→R9(100 Ω)→D7`, `N=GND`.

**Radio:** U3 (Ra-02) — SPI on D13/D12/D11 (SCK/MISO/MOSI), NSS on D10, DIO0 on D2, RESET on D9.
DIO1–DIO5 unconnected. All four GND pins on the pour.

**Unconnected MCU pins:** A0, A3, A4, A5, A6, A7, RST (both), and **DTR, TXO, RXI** — the three
signals that make the FTDI header useful.

---

## 4. Findings by domain

### 4.1 Power architecture

The topology — a reed-toggled flip-flop driving a load switch, with a software-off path — is
correct and I traced it in full (§8). Three things break it.

**The ultrasonic is not switched — and does not need to be.** `J3.2` sits directly on `BATT+`,
so between samples the RCWL-1670 stays powered while the MCU and the SX1278 sleep. At the module's
specified standby of 1.5 µA @ 3.3 V / 3.5 µA @ 5 V, that costs roughly **2 µA at 3.6 V**. Against a
297 µA ceiling this is negligible and gating it would save nothing. The original blocker here was
based on a wrong assumption about the part and is withdrawn.

One small consequence is worth recording: because the module is never power-cycled independently,
firmware has no way to recover it if it latches up. That is a recommendation (HN-49), not a defect.

**Low-side switching leaves the loads half-connected when "off".** With Q1 open, the ATmega and
the SX1278 keep `VCC` on `BATT+` while their ground floats up to meet it. That is an
uncharacterised bias condition for both parts. I traced the leakage paths and the OFF state is in
fact clean (§8) — but it is clean by accident, and a high-side switch removes the ambiguity
entirely and costs the same.

**Li-SOCl₂ pulse behaviour is unaddressed.** See §6.

### 4.2 RF

There is no RF on the carrier board — the Ra-02 carries the SX1278, its matching network and an
IPEX connector — which is the right call and eliminates a whole class of risk. What remains:

- **No band, region, ERP or antenna decision exists in the repository.** The Ra-02 is a 433 MHz
  part (410–525 MHz, +18 dBm). In ITU Region 1 the 433.05–434.79 MHz band is generally limited to
  10 mW ERP; +18 dBm is 63 mW. The brief's "custom-designed antenna" has no design files and no
  return-loss data. An unknown-VSWR antenna on a PA with no isolator is both a link-budget and a
  device-lifetime risk.
- **The footprint is not the Ra-02.** `RA-02_BREAKOUT_THT_2X8` places 2×8 pads at 2.54 mm pitch
  with **25.4 mm** between rows (pads at X = 2748.0 and X = 3748.0 mil). The Ra-02 is an SMD-16
  module, 17 × 16 mm, 2.0 mm pitch. The design assumes a breakout adapter that is not in the BOM.
- **Duty cycle is fine.** 30 transmissions/hour × 62 ms (SF7) = 0.05 %.
- The always-on flip-flop supply `NetC9_1` runs **114 mm** of track across the board (2310 mil top
  + 2169 mil bottom) to reach the reed switch, adjacent to a 100 mW transmitter. R13/C12 form a
  16 kHz low-pass at the clock input, which attenuates 433 MHz heavily — adequate, but a 100 pF
  bypass at U2 pin 3 would remove the question.

### 4.3 Layout and stackup

**Good:** solid `GND` pour on both layers (net 26 on both polygons), 33 stitching vias, all four
Ra-02 ground pins tied to it, the SPI nets kept to 224 mil each, and the reed switch moved to the
board edge (S1 at Y = 1151.6 mil, 3.75 mm from the 1003.9 mil edge).

**Broken:** the layer stack is Altium's untouched default — `Dielectric 1` at **12.6 mil**, giving
a **0.39 mm** finished board. That board carries a TO-220, a DIP-14, three 11 mm radial
electrolytics, four JST connectors and two plug-in modules.

**Design rules are all Altium defaults**: clearance 11.811 mil, width min/pref/max
11.811/19.685/39.37 mil, vias 50/28 mil, hole-to-hole 10 mil, solder-mask expansion 4 mil. There
are no net-class rules — `BATT+`, `BATT-` and `GND` use the same 0.5 mm width as signal nets. At
1 oz copper that carries the 130 mA burst with about 7 mV of drop, so it is not a current problem;
it is a missing-rule problem, and the reviewer after you cannot tell the difference.

**Decoupling proximity**, measured pad-to-pad:

| Rail | Nearest bypass | Distance |
|---|---|---|
| U3 pin 3 (Ra-02 3.3 V) | C6 (100 nF) | **5.6 mm** — good |
| U3 pin 3 | C8 (10 µF) / C7 (100 µF) | 10.8 / 13.0 mm — acceptable bulk |
| U2 pin 14 (74HC74 VCC) | C10 (100 nF) | **6.8 mm** — good |
| U2 pin 14 | C9 (10 µF hold-up) | 26.3 mm, opposite layer — the reservoir is far from what it feeds |
| U1 VCC (ATmega) | C5 (100 nF) | **16.7 mm** — poor, mitigated only by the Pro Mini's own on-board bypass |

**Mechanical:** four corner mounting holes on a 74 × 54 mm pattern, Ø2.70 mm — but with pad
diameter set equal to hole diameter, giving **zero annular ring**. Two unnamed free pads
(Ø2.00 mm pad / Ø0.80 mm hole) on `BATT+` at (2066.9, 3159.4) and (1496.1, 1122.0) are being used
as layer transitions in place of vias; all 33 real vias belong to `GND`.

**Silkscreen** carries the connector functions — "Ultrasonic", "Temp", "Flow", "BAT 3.6v",
"Reed Switch", "BUZZER" — and polarity marks on LS1, C7, C8 and C9. That is genuinely good work.
But the value legends have drifted: C4's "100nF" sits at X = 4496.2 mil against a board edge at
4153.5 (8.7 mm off the board), "3.6v" starts at X = 4155.7 (past the edge), C5's "100nF" sits at
Y = 3460.0 against a 3366.1 mil edge, and C2's "100nF" is 39 mm from C2. On a hand-assembled board
the legend is the work instruction.

**Assembly:** every part is through-hole, and they are on **both** sides — C4, C7, C8, C9, LS1 and
BATT1 on the bottom, everything else on top. That forces two hand-solder operations and rules out
wave or selective soldering entirely.

### 4.4 Component library integrity

- `WCAP-ATLL_D5H11` (used by **C7, C8, C9** — same Vault item GUID `89F9E2F2-…`) has its footprint
  origin roughly **1.27 m off the board**: C7 at (−46535.4, −48799.2) mil, C8 at (−46279.5,
  −48799.2), C9 at (−47095.4, −46929.1), against a board spanning (1003.9 … 4153.5) × (1003.9 …
  3366.1). The *pads* are correctly placed. The consequence is that these three parts' bounding
  boxes, courtyards and 3D bodies are 1.27 m away, so Altium's inside/outside-board and
  component-clearance checks are meaningless for them, and any centroid export is wrong.
- That same footprint is Ø5 mm with a 2.0 mm lead pitch (pads 78.8 mil apart). A 100 µF/25 V
  WCAP-ATLL is a Ø6.3 mm can on a 2.5 mm pitch. **C7 will not fit its footprint.**
- The U1 footprint omits the A4–A7 pads. Harmless today (those nets are unconnected) but it will
  bite whenever someone needs I²C.
- J1 uses Ø1.00 mm holes while J2 and J3 use Ø0.90 mm, for the same JST-XH family. JST's own
  drawing calls for Ø1.0 mm.

### 4.5 Signal integrity and logic levels

**The 74HC74's bias network fails at the datasheet limit.** R14 (2.2 MΩ) holds `1CP` low and R12
(1 MΩ) holds `1~RD` high. The 74HC74's input leakage is specified at **±1 µA max**. Worst case,
R14 develops 2.2 V on a pin whose V_IL max at 3.35 V supply is 0.99 V, and R12 loses 1.0 V from a
V_IH min of 2.31 V. Typical HC leakage is picoamps at room temperature, which is exactly why this
passes on a bench and fails on a roof at 70 °C.

**The clock input sees a 220 ms edge.** Releasing the magnet lets C12 (100 nF) decay through R14
(2.2 MΩ) — τ = 220 ms. The 74HC74's `1CP` is **not** Schmitt-triggered; the family's maximum input
transition rate at 3.3 V is on the order of 1 µs. This edge is ~200,000× over spec. It is the
inactive edge, so it should not clock — but the input sits in its linear region for a fifth of a
second, drawing through-current and free to oscillate. Spurious toggling on magnet *removal* means
the user cannot tell whether the node is on. The prior review recommended a Schmitt buffer
(74LVC1G17); it was not fitted.

**The closure edge, though, is fine.** S1 charges C12 through R13 (100 Ω) in ~50 µs, and a 1 ms
bounce-open decays only ~0.5 % through 2.2 MΩ. The asymmetric RC does debounce the closure
correctly. R13 also limits the reed's hot-switching current to 36 mA peak instead of a bare
capacitive discharge — a real improvement over revision 1.

**D13 is both SCK and the Pro Mini's on-board LED.** `U1.JP6_9 (SCK)` drives U3 pin 12 and, on a
stock Pro Mini, also the D13 status LED through its series resistor. That is milliamps on every
clock edge, and milliamps *forever* if firmware ever leaves SCK high in sleep. The brief mentions
removing the power LED; it does not mention this one.

### 4.6 Environmental and reliability

- **All nine 100 nF capacitors are Z5U** (`SR215E104MARTR1`, described in the schematic as
  "Z5U Radial Lead"). Z5U is specified **+10 °C to +85 °C** with a −56 % capacitance tolerance.
  This device sits on a rooftop. Below +10 °C the parts are simply out of specification, and a
  Z5U at −10 °C retains a small fraction of its rated value. Every bypass, the POR timing (C11),
  the reed debounce (C12) and the flow filter (C2) are affected. **These must be X7R.**
- **Three aluminium electrolytics on a 2-year battery budget.** C7 (100 µF/25 V), C8 and C9
  (10 µF/50 V) leak continuously — typically single-digit µA each at 3.6 V/25 °C, and leakage
  roughly doubles every 10 °C. They also lose ESR performance at low temperature exactly when the
  cell is weakest, and they wear out. MLCCs leak under 10 nA and never dry out.
- **No ESD or surge protection** on any of the three cables leaving the enclosure. R1–R5 and R9
  (100 Ω / 330 Ω series) help with fault current but do nothing for an ESD strike or an induced
  surge; D3, D4, D5, D6, D8 and A2 all go straight to the MCU.
- **No reverse-polarity protection.** D1 protects U2 only. A reversed battery pigtail destroys
  U1, U3 and the ultrasonic at once.
- **BATT1 and J1 are both 2-pin JST-XH** and mechanically interchangeable. Plugging the pack into
  J1 puts 3.6 V onto the flow node and into D5/A2 through 100 Ω/330 Ω.
- **S1 is a bare glass reed**, 4 × 29 mm, on a 35 mm unsupported lead span, 3.75 mm from a board
  edge, going onto a roof.
- **The RCWL-1670 is an uncoated PCB** (`Components Images/RCWL-1670.jpg`) with its transducers
  soldered on, going into a tank headspace that will condense. The **supply voltage is fine** —
  the module is specified 3–5 V, so 3.6 V is comfortably in range and the `+5V` on the silkscreen
  is a pad label, not a requirement. Its 2 cm blind zone and 4 m range also suit a rooftop tank
  well. The open risk is corrosion on an uncoated board in a condensing headspace, not the rail.

### 4.7 Test, service and telemetry

- **There is no programming or debug access.** U1's carrier pads for DTR, TXO and RXI (at
  (2274.4, 2686.2), (2374.4, 2686.2), (2474.4, 2686.2) mil) have **no net**. The Pro Mini's own
  FTDI header pins are consumed by the carrier board. There is no way to reflash a sealed rooftop
  unit, and no test points anywhere on the board.
- **No battery telemetry.** A0 and A3–A7 are unconnected; there is no divider to any ADC. Li-SOCl₂
  has a famously flat discharge curve, so open-circuit voltage is a weak gauge — but the *loaded*
  voltage during a TX burst is an excellent health and passivation proxy, and it is unavailable.
- **The MCU cannot read the reed switch.** S1 lives entirely in the always-on domain. Firmware
  cannot implement press-and-hold, cannot debounce, and cannot offer any second user gesture —
  which also means there is no local recovery or unpair path on a sealed device.

---

## 5. BOM review (against `Components-List.txt`)

The list matches the schematic exactly on quantities — 9 × 100 nF, 3 × electrolytic, 14 resistors
(1 × 1 k, 3 × 1 M, 6 × 100 R, 1 × 100 k, 1 × 330 R, 1 × 4.7 k, 1 × 2.2 M), 4 JST headers. That is
worth noting; BOM/schematic mismatches are common and there are none here.

It is not, however, a production BOM. It has **no manufacturer part numbers, no tolerances, no
dielectrics, no temperature ratings, no package specifications and no approved sources.** Every
part is described in a way that permits a purchaser to buy the wrong thing.

| Line | Issue |
|---|---|
| `9x 0.1uF - (104) Ceramic Capacitor` | Schematic part is Z5U. Must be specified X7R (HN-11). |
| `1x 100uf 25V Electrolytic Capacitor` | Footprint is Ø5 mm / 2.0 mm pitch; a 100 µF/25 V ATLL is Ø6.3 mm / 2.5 mm. Will not fit (HN-20). |
| `2x 10uf 50V Electrolytic Capacitor` | Leakage on a 2-year budget; should be MLCC (HN-12). |
| `1x 74HC74AP flip-flop` | "AP" is the Toshiba part; the schematic uses NXP `74HC74N,652`. Either is fine, but the family's quiescent current is the wrong choice regardless (HN-10). |
| `1x IRLZ44N N-Channel MOSFET` | 47 A TO-220 switching ~130 mA; V_GS(th) up to 2.0 V and R_DS(on) not characterised at 3.3 V gate drive (HN-22). |
| `1x Buzzer` | LS1 is `CPT-1255C-090`, an externally-driven transducer rated 80 dB **at 20 V p-p** (HN-26). |
| `2x B2B-XH-A JST Connector` | Battery and flow use the same 2-pin XH — interchangeable (HN-16). |
| `1x RCWL-1670 Waterproof Ultrasonic` | Electrically well matched: 3–5 V supply, 6 mA working, 1.5 µA standby, 2 cm–4 m, 50 ms cycle. The open risk is an uncoated PCB in a condensing headspace (HN-24). |
| `1x Water Flow switch` | Photo shows a WY-90 rated DC 12–24 V, operated dry at 3.6 µA (HN-25). |
| `2x LS14500 … ( Parallel Connection )` | Hard-paralleled primary lithium cells, no blocking diodes, no fuse (HN-04). |
| `1x IPEX to SMA Cable + Antenna` | No part, no band, no gain, no VSWR (HN-07). |
| `1x Arduino Pro Mini 3.3v` | Required rework (regulator, power LED, D13 LED) is undocumented (HN-06). |
| — | No resistor tolerance/type, no capacitor voltage ratings for the 100 nF parts, no magnet grade for the reed actuation, no cable specification for the three sensor harnesses. |

---

## 6. Power budget and battery-life projection

### 6.1 Energy available

2 × Saft LS14500 = 5.2 Ah nominal (2.6 Ah each, rated at low continuous current, +20 °C, to 2.0 V).

| | Ah | Average current for 2 years (17,520 h) |
|---|---|---|
| Nominal, no derating — the absolute ceiling | 5.20 | **297 µA** |
| × 0.85 pulse/temperature derate × 0.98 self-discharge × 0.80 design margin | 3.47 | **198 µA** |

Every number below is measured against the 297 µA ceiling, which is generous to the design.

### 6.2 Sleep current (≈ 119.9 s of every 120 s cycle)

| Item | Typ, 25 °C | Max / hot (60–85 °C) |
|---|---:|---:|
| U1 ATmega328P, power-down + WDT, BOD off | 5 µA | 15 µA |
| U3 SX1278 sleep | 1 µA | 3 µA |
| **U2 74HC74 quiescent** | 1 µA | **80 µA** |
| C7 100 µF/25 V electrolytic leakage | 2 µA | 15 µA |
| C8 10 µF/50 V | 1 µA | 8 µA |
| C9 10 µF/50 V | 1 µA | 8 µA |
| U2 input leakage through R12 / R14 | <1 µA | 2 µA |
| R6 1 MΩ pull-up (only while the tank is filling) | 0 | 3.6 µA |
| RCWL-1670 standby on `BATT+`, interpolated to 3.6 V | 2 µA | 6 µA |
| **Total sleep current** | **~14 µA** | **~141 µA** |

The RCWL-1670 line is the corrected one. Vendor specification is **1.5 µA @ 3.3 V, 3.5 µA @ 5 V**;
interpolated to 3.6 V that is ~1.9 µA, and the hot figure is not specified by the vendor, so 6 µA
is a conservative CMOS-leakage allowance. Leaving the module permanently powered is fine.

With the ultrasonic corrected, **U2 is now the dominant sleep term by a wide margin**. At 80 µA
— the family's 85 °C limit — a single always-on logic IC is **57 % of the hot sleep budget**, more
than everything else on the board combined, and 27 % of the entire average-current ceiling. The
revision-1 design used a CD4013B, specified at 1 µA max at 25 °C. **The CD4013B → 74HC74
substitution was a power regression.** A 74AUP1G74 (0.9 µA max) removes the line item entirely and
is the single cheapest improvement available to this design.

### 6.3 Active energy per 2-minute cycle

**Case A — energy-aware firmware.** DS18B20 at 9-bit (94 ms), MCU idle-sleeps during conversion,
SF7/BW125/CR4-5, 16-byte payload (~62 ms airtime), +17 dBm (90 mA).

| Phase | Charge |
|---|---:|
| Wake / setup, 20 ms @ 4.0 mA | 0.08 mAs |
| DS18B20 power-on + conversion, 100 ms | 0.25 mAs |
| Ultrasonic measurement, 50 ms cycle @ 6 mA (vendor figures) | 0.30 mAs |
| Ra-02 wake + config + FIFO, 15 ms | 0.08 mAs |
| **TX, 62 ms @ 94 mA** | **5.83 mAs** |
| Shutdown, 10 ms | 0.04 mAs |
| **Total** | **6.6 mAs → 55 µA average** |

**Case B — naive Arduino firmware.** DS18B20 at 12-bit (750 ms) inside `delay()`, SF9 (205 ms
airtime), +20 dBm (120 mA): **30 mAs → 250 µA average**.

The spreading factor is not a detail. SF7 → SF9 alone moves the TX contribution from 49 µA to
207 µA. Since SF is set by the link budget, and the link budget is set by the antenna and the band
— neither of which is decided (HN-07) — **the power budget cannot actually be closed until the RF
decision is made.**

### 6.4 Verdict against the 2-year target

| Scenario | Average | Life (3.47 Ah derated) | Verdict |
|---|---:|---:|---|
| **As drawn**, Case A energy-aware firmware, 25 °C | 69 µA | **5.7 years** | **PASS** |
| **As drawn**, Case A firmware, hot (60–85 °C) | 196 µA | **2.02 years** | **PASS — zero margin** |
| As drawn, Case B naive firmware, hot | 391 µA | 1.01 years | **FAIL — 2.0×** |
| As drawn, Case B naive firmware, 25 °C | 264 µA | 1.50 years | **FAIL — 1.3×** |
| **+ 74AUP1G74 + MLCC bulk, Case A firmware, hot** | **86 µA** | **4.6 years** | **PASS with margin** |

Two conclusions follow, and they are different from the first issue of this review.

**The architecture meets the target.** With the corrected ultrasonic figure the design closes at
5.7 years typical, and it is firmware — not hardware — that decides whether it stays there. SF7 →
SF9 alone moves the transmit contribution from 49 µA to 207 µA, and a 12-bit DS18B20 conversion
inside `delay()` costs more than the entire sleep budget. HN-45 (the firmware energy contract) is
therefore promoted from a nice-to-have to the load-bearing requirement.

**At temperature there is no margin.** 2.02 years against a 2-year minimum is not a pass anyone
should ship. Two component changes — U2 to a 74AUP1G74 (HN-10) and the three electrolytics to MLCC
(HN-12) — remove 110 µA of hot sleep current and take the hot case to 4.6 years. Both are cheaper
than any other item on the issue list.

### 6.5 Li-SOCl₂ pulse behaviour — a separate FAIL

Peak load: Ra-02 at +20 dBm (120 mA) + MCU (4 mA) + ultrasonic (8 mA) ≈ **132 mA**.

Per-cell limits are 50 mA continuous and ~100 mA pulse; two cells in parallel nominally cover
132 mA at +20 °C on a **depassivated** cell. That is the only condition under which this works.

- **Passivation.** After weeks at ~14 µA the LiCl film raises cell impedance into the tens of ohms.
  A 20 Ω passivated source at 132 mA drops **2.6 V**. Even a mild 5 Ω costs 0.65 V, taking a
  3.6 V rail to 2.95 V — and the ATmega328P requires **≥ 2.7 V to run at 8 MHz**. Add the
  impedance rise at 0 °C and the first transmission after a quiet period lands below the part's
  safe operating area.
- **With BOD disabled** — which the sleep budget requires — nothing detects that. The MCU executes
  at an invalid voltage, with flash and EEPROM exposed.
- **C7 cannot help.** At 120 mA, 100 µF sustains the load for **0.25 ms** before dropping 0.3 V.
  A 62 ms SF7 burst needs 7.44 mC; C7 supplies 30 µC of it — **0.4 %**. Holding ΔV ≤ 0.3 V across
  that burst requires ≈ **25,000 µF**, and ≈ 82,000 µF at SF9.

**The battery supplies the pulse, not the capacitor.** The correct answer is a hybrid-layer
capacitor (e.g. Tadiran TLI-1550A) or a supercapacitor charged through a current-limiting path —
and once one is fitted, a *single* cell plus that HLC is likely better than two paralleled cells,
which also disposes of HN-04.

**No depassivation strategy exists.** Firmware should draw a defined depassivation load on a
schedule, and the design should give it a way to measure the result (HN-19).

### 6.6 HN-03 explained, and how to close it

**What passivation is.** Li-SOCl₂ cells get their energy density and 20-year shelf life from a
chemical trick: a thin insulating film of lithium chloride grows on the lithium anode. That film is
what stops the cell self-discharging. It is a feature, not a defect.

**What it costs you.** The film also has resistance, and it thickens the longer the cell sits at low
current — which is exactly what your node does for 119.9 s out of every 120 s, and for months if the
reed switch is off. A fresh, exercised LS14500 has roughly 1–2 Ω of internal impedance. One that has
been idling at ~14 µA for a month can be 10–50 Ω.

**Then Ohm's law does the rest.** The LoRa burst pulls ~130 mA:

| Cell state | Impedance (2 cells in parallel) | Drop at 130 mA | Rail |
|---|---:|---:|---|
| Fresh / exercised | ~1 Ω | 0.13 V | 3.47 V — fine |
| Mildly passivated | ~5 Ω | 0.65 V | 2.95 V — marginal |
| Passivated after a month idle | ~10 Ω | 1.30 V | **2.30 V — below the ATmega's 2.7 V floor at 8 MHz** |
| Same, at −5 °C | ~25 Ω | 3.25 V | **rail collapses** |

**Why this is the nastiest class of bug.** It passes on your bench every time, because the cell you
are testing with was used recently and is depassivated. It fails in the field, after the node has
been quiet — and because BOD must be disabled to hit the sleep budget, the MCU does not reset
cleanly. It executes at 2 V, which is how EEPROM gets corrupted.

**Why C7 cannot fix it.** The intuition that "a big capacitor covers the pulse" does not survive the
arithmetic:

- Charge one transmission needs: 130 mA × 62 ms = **8.1 mC**
- Charge C7 can give up for a 0.3 V sag: 100 µF × 0.3 V = **0.03 mC**
- C7 therefore covers **0.4 %** of the burst, and holds the load for 0.25 ms out of 62 ms
- To actually hold ΔV ≤ 0.3 V you would need ≈ **27,000 µF** — not a part that belongs on this board

**The battery supplies the pulse. A capacitor only rounds off the leading edge.**

#### Three ways to close it

**Option A — hybrid layer capacitor (HLC). The industry standard answer, and my recommendation.**
An HLC (Tadiran TLI-1550A or equivalent) is a low-impedance rechargeable lithium cell in an AA-sized
can. Wire it: `LS14500 → series Schottky → HLC in parallel → load`. The primary cell trickle-charges
the HLC at microamps between transmissions; the HLC delivers the 130 mA burst from an impedance of
well under an ohm. **This closes HN-04 at the same time** — with an HLC you need only *one*
LS14500, so the two-cells-in-parallel safety problem disappears entirely. Check the HLC's cycle
rating against your 525,600 cycles over two years before committing.

**Option B — supercapacitor plus a current limiter.** A 0.1–0.5 F supercap across the rail, charged
through a series resistor sized so the cell never sees more than ~15 mA. For 0.22 F and 220 Ω the
recharge time constant is ~48 s, which fits inside a 120 s cycle. Cheaper and easier to source than
an HLC, but supercap leakage is 5–50 µA and would eat a large part of the 14 µA sleep budget you
have — so pick a low-leakage part and *measure* it.

**Option C — manage it in firmware, change no hardware.** Have firmware draw a defined depassivation
load on a schedule. Your 2-minute cycle may in fact keep the cell exercised in steady state; the
exposures are the *first* transmission after shipping or storage, and the first after any long
reed-switch power-off. This is free and it may genuinely be sufficient — but it must be measured,
not assumed, and it does not cover a node that sat switched off for a month.

#### The test that tells you which option you need

Do this before choosing. It is one afternoon and it settles the question.

1. Take an LS14500 that has been idle at sleep current — or simply on a shelf — for **at least two
   to four weeks**. This is the whole point; a freshly-used cell will pass and tell you nothing.
2. Scope across the battery terminals, DC-coupled, 500 mV/div, 20 ms/div, single-shot trigger.
3. Fire one LoRa transmission at full power and capture the dip.
4. Repeat at **−5 °C and +50 °C** — impedance roughly triples at the cold end.

| Minimum rail during the burst | What it means |
|---|---|
| **above ~3.0 V** | Option C. Document the result and move on. |
| **2.7–3.0 V** | Option B. Add the supercapacitor. |
| **below 2.7 V** | Option A. You need the HLC — and you can drop to one cell. |

Add the battery-sense divider from HN-44 while you are at it, so the shipped product can report this
number to the Hub instead of you having to guess at it two years from now.

---

## 7. Tagged issue list

### BLOCKER (7)

| ID | Finding | Reference |
|---|---|---|
| **HN-03** | No Li-SOCl₂ pulse support and no depassivation strategy. C7 supplies 0.4 % of a TX burst; ~25,000 µF or an HLC/supercap is required. Rail can collapse below the ATmega's 2.7 V @ 8 MHz limit with BOD disabled. | `C7`, `BATT1`; §6.5 |
| **HN-04** | Two LS14500 primary lithium cells hard-paralleled through a single 2-pin `BATT1` — no per-cell blocking diode, no fuse or PTC. A stronger cell charges a weaker one; Li-SOCl₂ must never be charged. Safety and certification blocker. | `BATT1` (2 pads only); `Components-List.txt` |
| **HN-05** | U3 footprint `RA-02_BREAKOUT_THT_2X8` is 2.54 mm pitch with 25.4 mm row spacing (pads at X = 2748.0 / 3748.0 mil). The Ra-02 is SMD-16, 2.0 mm pitch, ~17 mm rows. No adapter board in the BOM. | `U3`; Components6 `PATTERN` |
| **HN-06** | The Pro Mini rework the design depends on is documented nowhere. The power LED alone (~4 mA) exhausts the pack in ~54 days. Additionally **D13 is used as SCK** and also drives the module's on-board status LED. | `U1`; `Components-List.txt`; net `NetU1_JP6_9` |
| **HN-07** | No antenna design, no band/region decision, no ERP budget, no VSWR data. 433 MHz at +18 dBm is 63 mW against a 10 mW ERP limit in ITU Region 1. This also blocks the SF choice, which blocks the power budget. | repository-wide; `U3` |
| **HN-08** | No manufacturing data set: no Gerbers, drill, ODB++, OutJob, assembly drawing, pick-and-place, DRC report or `.PrjPcb`; no MPN-level BOM; and the referenced `.SchLib`/`.PcbLib` files are absent, so the design cannot be rebuilt by anyone else. | repository-wide |
| **HN-09** | Layer stack left at Altium's default 12.6 mil dielectric → **0.39 mm** finished board, carrying a TO-220, DIP-14, three 11 mm radial cans, four JST connectors and two modules. | `Board6`, `LAYER_V8_4DIELHEIGHT=12.6mil` |

### MAJOR (21)

| ID | Finding | Reference |
|---|---|---|
| **HN-01** | The J3 → RCWL-1670 harness is deliberately crossed and wired by conductor colour, which is electrically correct — but it is not a controlled part anywhere in the repository. A straight-wired cable with correct colours looks right at incoming inspection and is wrong. Add a wire-list drawing and put the four signal names on the silkscreen beside J3. **Cheaper alternative:** reorder J3 to `1=GND, 2=Trig, 3=Echo, 4=VCC` to match the module. A straight-through cable then gives black on GND *and* red on +5V with no crossing at all — both goals, one net swap in the respin. | `J3`, nets `BATT+`/`NetJ3_3`/`NetJ3_4` |

| ID | Finding | Reference |
|---|---|---|
| HN-10 | 74HC74 quiescent (8 µA max @ 25 °C, 80 µA @ 85 °C) is up to 27 % of the whole current ceiling. The CD4013B → 74HC74 change was a power regression. Use 74AUP1G74 (0.9 µA max). | `U2` |
| HN-11 | All nine 100 nF caps are **Z5U** — specified only +10 °C…+85 °C, −56 % tolerance — in an outdoor rooftop device. Affects every bypass plus C11 (POR), C12 (debounce), C2 (flow filter). Must be X7R. | `C1–C6, C10–C12`, `SR215E104MARTR1` |
| HN-12 | C7/C8/C9 aluminium electrolytics: µA-class leakage that doubles per 10 °C, ESR rise at low temperature, and wear-out. Replace with MLCC. | `C7`, `C8`, `C9` |
| HN-13 | `1CP` sees a 220 ms fall (C12 × R14) into a non-Schmitt HC clock input — ~200,000× the family's max input transition rate. Linear-region through-current and possible spurious toggling on magnet removal. | `U2.3`, `C12`, `R14` |
| HN-14 | R14 = 2.2 MΩ and R12 = 1 MΩ against the 74HC74's ±1 µA max input leakage: worst case 2.2 V on a pin whose V_IL max is 0.99 V. Bias network fails at the datasheet limit; passes on a bench, fails hot. | `R12`, `R14`, `U2.1`, `U2.3` |
| HN-15 | No reverse-polarity protection on `BATT+`. D1 protects U2 only. Recommend a P-channel ideal-diode rather than a Schottky — a 3.6 V plateau cannot spare 0.3 V. | net `BATT+`, `D1` |
| HN-16 | `BATT1` and `J1` are both 2-pin JST-XH and interchangeable. Plugging the pack into J1 drives 3.6 V into D5/A2. | `BATT1`, `J1` |
| HN-17 | No TVS/ESD protection on any of the three external cables; series resistors only. D3, D4, D5, D6, D8, A2 go straight to the MCU. | `J1`, `J2`, `J3` |
| HN-18 | No programming or test access. U1's DTR/TXO/RXI carrier pads have no net; the module's own header is consumed by the carrier. No test points. | `U1` pads (2274.4/2374.4/2474.4, 2686.2) mil |
| HN-19 | No battery telemetry. A0, A3–A7 unconnected, no divider to any ADC. Loaded-voltage-during-TX is the only useful Li-SOCl₂ health signal and it is unavailable. | `U1` |
| HN-20 | C7's footprint is Ø5 mm on 2.0 mm pitch; a 100 µF/25 V WCAP-ATLL is Ø6.3 mm on 2.5 mm pitch. The specified part does not fit. | `C7`, `WCAP-ATLL_D5H11` |
| HN-21 | `WCAP-ATLL_D5H11` footprint origin is ~1.27 m off-board for C7/C8/C9, breaking bounding boxes, courtyards, 3D bodies, inside/outside-board DRC and centroid export. | `C7` (−46535.4, −48799.2) mil etc. |
| HN-22 | IRLZ44N: a 47 A TO-220 switching ~130 mA. V_GS(th) up to 2.0 V, R_DS(on) not characterised at 3.3 V drive, and a needlessly large package. A small SOT-23 logic-level FET or a load-switch IC is correct. | `Q1` |
| HN-23 | Low-side switching leaves `VCC` applied to the ATmega and SX1278 with their ground floating — an uncharacterised bias state. Move to a high-side switch. | `Q1`, nets `GND`/`BATT-` |
| HN-24 | RCWL-1670 is an uncoated PCB with its transducers soldered on, pointed into a condensing tank headspace. Corrosion and dendrite growth on an uncoated board in condensing conditions is a matter of months. *(The supply-voltage concern in the first issue is withdrawn — the module is specified 3–5 V, so 3.6 V is in range.)* | `Components Images/RCWL-1670.jpg` |
| HN-25 | WY-90 flow switch is rated DC 12–24 V and is being operated dry at 3.6 µA (R6 = 1 MΩ) — far below any wetting current the vendor qualifies. | `J1`, `R6`; `Components Images/Flow-Switch.png` |
| HN-26 | LS1 (`CPT-1255C-090`) is an externally-driven piezo transducer rated 80 dB **at 20 V p-p**. At 3.3 V p-p expect roughly 64 dB, and firmware must generate the tone — a static level produces silence. | `LS1`, `R9`, `U1.D7` |
| HN-27 | DS18B20 is pin-powered from D3 down ~1 m of cable with **no bypass capacitor** at the sensor. Maxim requires a local 100 nF. | `J2.3`, net `NetJ2_3` |
| HN-28 | Silkscreen legends outside the board outline (C4's "100nF" at X = 4496.2 mil vs a 4153.5 edge; "3.6v" at 4155.7; C5's "100nF" at Y = 3460.0 vs a 3366.1 edge) and several values 20–40 mm from their part. | Texts6, layers 33/34 |
| HN-29 | All-through-hole with parts on **both** sides (C4, C7, C8, C9, LS1, BATT1 bottom). Two hand-solder operations; no wave or selective path. | Components6 `LAYER` |

### MINOR (11)

| ID | Finding |
|---|---|
| HN-30 | Four corner mounting holes have pad Ø = hole Ø = 2.70 mm → zero annular ring. Make them proper NPTH or give them an annulus. |
| HN-31 | Two unnamed free pads on `BATT+` (2066.9, 3159.4) and (1496.1, 1122.0) are used as layer transitions instead of vias; they carry no designator and will confuse DFM review. |
| HN-32 | J1 uses Ø1.00 mm holes, J2/J3 Ø0.90 mm, for the same JST-XH family. Standardise on Ø1.0 mm. |
| HN-33 | The flow node lands on **both** D5 (R5 100 Ω) and A2 (R3 330 Ω). A2 adds no diagnostic value with a plain 1 MΩ pull-up, and the two pins would fight through 430 Ω if either is ever driven. Delete R3, or turn R6 into a divider so A2 can distinguish open-cable from open-switch. |
| HN-34 | No net-class width rules; `BATT+`/`BATT-`/`GND` use the same 19.685 mil default as signals. Current capacity is fine — the missing rule is the problem. |
| HN-35 | U1 footprint omits the A4–A7 pads. |
| HN-36 | Nearest bypass to U1's VCC is C5 at 16.7 mm; C9 (the flip-flop reservoir) is 26.3 mm from U2 pin 14 and on the opposite layer. |
| HN-37 | `Components-List.txt` says "74HC74AP" (Toshiba); the schematic uses NXP `74HC74N,652`. |
| HN-38 | No board outline drawn on a mechanical layer and the Keep-Out layer is unused; the shape exists only as the intrinsic board shape. |
| HN-39 | S1 is a bare glass reed with a 35 mm unsupported lead span, 3.75 mm from the board edge, on a roof. Specify a plastic-encapsulated part or add strain relief. |
| HN-40 | No revision, date or board identifier on the silkscreen. |

### RECOMMENDATION (7)

| ID | Recommendation |
|---|---|
| HN-41 | Move the master switch **high-side** (P-FET or a load-switch IC) and add a **second** gate for the ultrasonic. This is the single change that fixes HN-02 and HN-23 together and removes the floating-ground question. |
| HN-42 | Add a 100 pF RF bypass at U2 pin 3 and shorten `NetC9_1` — 114 mm of always-live, high-impedance net runs beside a 100 mW transmitter. |
| HN-43 | Bring DTR/TXO/RXI to a 6-pin programming header and add test points on `BATT+`, `BATT-`, `GND`, `1CP`, `1~RD` and the Q1 gate. |
| HN-44 | Add a battery-sense divider (high value, gated by a GPIO so it costs nothing in sleep) so firmware can report loaded rail voltage during TX — the only meaningful Li-SOCl₂ health signal. |
| HN-45 | Write the firmware against an explicit **energy contract**: ≤ 6.6 mAs per cycle, ≤ 62 ms airtime, DS18B20 at 9-bit, MCU idle-sleep during conversions, all unused pins configured, BOD off, and SCK driven low before sleep. |
| HN-46 | Give the battery a distinct connector family (JST-PH or a locking 2-pin) so it can never be swapped with J1. |
| HN-49 | The ultrasonic can never be power-cycled independently, so firmware cannot recover it from a latch-up. If field experience shows the module hanging, a gate pin becomes worth adding — not for power, but for recovery. |
| HN-48 | Restore `HYDRO-NODE-HARDWARE-ISSUES.md` (or a successor) to the repository. Deleting the issue tracker loses the design rationale for revision 2's changes. |

### WITHDRAWN

| ID | Finding | Why |
|---|---|---|
| ~~HN-02~~ | *“The ultrasonic supply is not gated.”* | The RCWL-1670 is specified at 1.5 µA @ 3.3 V standby, so leaving it on `BATT+` costs ~2 µA. The original ~2 mA estimate was anchored on the wrong module class. No gating is needed. |
| ~~HN-47~~ | *“Settle the ultrasonic's supply voltage; consider boosting to 5 V.”* | The module is specified 3–5 V. 3.6 V is in range; the `+5V` silkscreen is a pad label. Superseded by HN-49. |

---

## 8. Verified sound

Reviewed and found correct — recorded so the next reviewer does not re-litigate them:

- **The toggle latch topology is correct.** U2 is a proper T flip-flop: `1D` ← `1~Q`, `1CP` ← the
  reed node, `1Q` → R10 (1 kΩ) → Q1 gate, R8 (1 MΩ) gate pulldown. `1~SD` tied high. POR via R12/C11
  (τ ≈ 100 ms) means the node powers up **off** when cells are first fitted. Software-off via R11
  from A1 divides to 0.30 V against a V_IL max of 0.99 V — comfortable. If the MCU resets, A1
  reverts to a high-Z input and the node stays **on**, which is the safe default.
- **The unused half of U2 is correctly tied off:** `2D` and `2CP` to `BATT-`, `2~SD` and `2~RD` to
  VCC, `2Q` and `2~Q` left open.
- **Q1's orientation is correct** — drain on `GND`, source on `BATT-`, so the body diode blocks
  load current rather than bypassing the switch.
- **The OFF state is genuinely off.** The only cross-domain path with Q1 open is R11 (100 kΩ) from
  A1 to `1~RD`, carrying ≤ 2.5 µA and biasing `1~RD` **high** — the safe direction.
- **Pour-to-clock coupling is not a hazard.** The `GND` pour swings 3.6 V at power transitions, but
  the coupled charge into `NetC12_1` (~68 mm of track, a few pF) is ~18 pC into C12's 100 nF —
  0.18 mV. C11 protects `1~RD` the same way. Both capacitors are load-bearing; do not delete them.
- **Reed debounce works.** Closure charges C12 through R13 (100 Ω) in ~50 µs; a 1 ms bounce-open
  decays only ~0.5 % through R14. The asymmetric RC turns a bouncing closure into one clean clock.
- **Ra-02 grounding is fixed.** All four GND pins (1, 2, 9, 16) are on the pour, resolving the
  prior single-pin return.
- **Echo is on D8 = ICP1**, so hardware input capture is available for the ultrasonic timing.
- **Ground pour is real:** solid polygons on Top and Bottom, both on net `GND`, with 33 stitching
  vias (50/28 mil).
- **Thermal reliefs are not a bottleneck.** Four 0.25 mm spokes contribute ~0.25 mΩ on the TX
  return path.
- **Track current capacity is adequate.** 0.5 mm at 1 oz carries the 130 mA burst with ~7 mV total
  drop; the concern on these nets is inductance and rules, not heating.
- **Silkscreen carries the connector functions** — "Ultrasonic", "Temp", "Flow", "BAT 3.6v" — plus
  polarity marks on LS1, C7, C8 and C9. Keep this.
- **Schematic and PCB agree.** Both carry 29 multi-pin nets with identical membership, despite the
  absence of a `.PrjPcb` linking them.
- **The BOM quantities match the schematic exactly** on all passive counts.

---

## 9. Open questions

Answers to these change the design, so they are listed rather than guessed.

1. **Deployment country / region?** Sets the band, the legal ERP, the SF, and therefore the power
   budget. Blocks HN-07 and §6.3.
2. ~~How is the ultrasonic harness wired?~~ **Answered.** Deliberately crossed, by conductor
   colour. The remaining question is whether that convention gets written down as a controlled
   drawing before anyone else builds one (HN-01).
3. ~~What is the RCWL-1670's idle current?~~ **Answered.** Vendor specification: 1.5 µA @ 3.3 V,
   3.5 µA @ 5 V standby; 6 mA working; 3–5 V supply; 50 ms measurement cycle; 2 cm–4 m range.
   Worth one bench measurement at 3.6 V and 60 °C to confirm, since the hot figure is unspecified —
   but nothing in the verdict now turns on it.
4. **Is a breakout/adapter board intended under the Ra-02**, or is the U3 footprint simply wrong?
5. **Has the Pro Mini rework been done, and on what variant?** Clone Pro Minis differ in regulator,
   LED resistor and fuse settings between batches — this is why HN-06 recommends moving the
   ATmega onto the carrier board for production.
6. **Where does the RCWL-1670 PCB physically sit** relative to the tank lid, and how is it
   protected from condensation?
7. **Expected tank depth and geometry?** Sets the minimum ultrasonic range, which determines
   whether 3.6 V operation is acceptable at all.
8. **Intended finished board thickness** — 1.6 mm is assumed, but the file says 0.39 mm.
9. **What is the target packet size and interval budget on the Hub side?** Airtime is the single
   largest active-energy term.
10. **Is there a service model?** If a sealed rooftop unit can never be reflashed (HN-18), that
    must be a deliberate decision, not an oversight.

---

## 10. Next steps

### Day 0 — firmware can start; three parameters stay open

Firmware is unblocked. The sensor drivers, sleep state machine and packet format can all be written
now. Three parameters need answers before the firmware is finished, none before it is started:

1. **Band and region**, which fixes the maximum SF (Q1, HN-07). Sets the transmit energy term,
   which is two-thirds of the active budget.
2. **The pulse-support part** — HLC or supercapacitor, and therefore one cell or two (HN-03,
   HN-04). Determines whether firmware must run a depassivation routine.
3. **Pro Mini or on-board ATmega** (HN-06). Determines the fuse settings, the sleep floor, and
   whether D13 can be used as SCK.

**Write the energy contract first (HN-45).** With the hardware as drawn, energy-aware firmware
gives 5.7 years and naive firmware gives 1.5 — firmware is now the deciding variable, not the
board. Budget ≤ 6.6 mAs per cycle, ≤ 62 ms airtime, DS18B20 at 9-bit, MCU idle-sleep during
conversions, all unused pins configured, BOD off, SCK driven low before sleep.

### Before the respin

- Fix the layer stack to 1.6 mm (HN-09) and set net-class width rules (HN-34).
- **Replace U2 with a 74AUP1G74 and C7/C8/C9 with MLCC first** — those two changes alone remove
  110 µA of hot sleep current and move the hot-case life from 2.02 to 4.6 years (HN-10, HN-12).
- Replace: all nine caps with X7R (HN-11); Q1 with a small logic-level FET or load switch, moved
  high-side (HN-22, HN-23, HN-41).
- Add: reverse-polarity protection (HN-15), a battery fuse/PTC (HN-04), TVS on all three cable
  interfaces (HN-17), a local 100 nF at the DS18B20 (HN-27), a programming header and test points
  (HN-18, HN-43), a battery-sense divider (HN-19, HN-44).
- Change the battery to a non-XH connector (HN-16, HN-46).
- Fix the U3 footprint or add the adapter to the BOM (HN-05).
- Fix the `WCAP-ATLL_D5H11` footprint origin and C7's body size (HN-20, HN-21).
- Clean up the silkscreen legend placement (HN-28) and add a revision marker (HN-40).
- Consolidate all parts onto one side (HN-29).

### Before manufacturing

- Produce the full data set: Gerbers, drill, ODB++, netlist, assembly drawing, pick-and-place,
  a clean DRC report, and a `.PrjPcb` (HN-08).
- Commit the `.SchLib` / `.PcbLib` files to the repository (HN-08).
- Produce an MPN-level BOM with tolerances, dielectrics, temperature ratings and approved sources
  (§5).
- **Measure, then approve.** Scope `BATT+` at the module during a real TX burst, on a cell that has
  been idle at sleep current for at least two weeks, at −5 °C and at +50 °C. This single test
  validates or kills the pulse design (HN-03).
- Measure sleep current on a built board against the §6.2 table, line by line.
- Verify the reed pull-in distance through the production enclosure wall with the production
  magnet, and specify a minimum magnet grade.
- Range-test the ultrasonic at the real tank depth, and confirm its standby current at 3.6 V and
  60 °C — the only unverified term left in the sleep budget (HN-24, Q3).
- Produce a controlled harness drawing for the crossed J3 cable before anyone else builds one
  (HN-01).

---

*Findings derived programmatically from the Altium binary files in this repository. Where a claim
rests on a component datasheet rather than a repository file, the datasheet value is named so it
can be checked against the exact vendor and revision you intend to buy.*
