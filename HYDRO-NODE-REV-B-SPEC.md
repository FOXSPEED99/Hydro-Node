# HYDRO NODE — REVISION B BUILD SPECIFICATION
Derived from `HYDRO-NODE-HARDWARE-ISSUES.md` v15. Every item traces to an issue ID.

This document is written so it can be executed by someone who has not read the whole tracker — a PCB designer, or you following it step by step. Work through it in order; the sections are sequenced as the work has to happen.

---

## 0. OPEN DECISIONS — settle these before starting

| # | Decision | Options | Blocks |
|---|---|---|---|
| D1 | **MCU: module or chip-down?** | (a) Keep the Pro Mini as a module — faster, but every unit needs manual LED + LDO removal and fuse programming. (b) Put the ATmega328P directly on the board. | HW-026. Changes the schematic substantially — **decide first** |
| D2 | Clock source | Crystal ±30 ppm (recommended) or ceramic resonator | HW-024. ±0.5 % resonator = ±5 mm at 1 m |
| D3 | Reed or Hall | Glass reed (have it) or micropower Hall latch | HW-039, HW-015 |
| D4 | Board layers | 2-layer with solid bottom pour (adequate) or 4-layer | HW-004 §11.9 |

**Recommendation: D1 = (b) chip-down, D2 = crystal, D3 = reed for now, D4 = 2-layer.** The rest of this spec assumes those, and flags where (a) differs.

---

## 1. SCHEMATIC CHANGES

### 1.1 Power path

| Change | Detail | Issue |
|---|---|---|
| Cell isolation | `1N5819` in **each** cell's + leg, inside the pack | HW-003 |
| Pack fuse | 0.5 A fast-blow (`0451.500` or 5×20 mm glass) in the pack lead | HW-003 |
| Pack build | Sealed, non-serviceable, single JST connector | HW-003 |
| Latch hold-up | Schottky `BAT54`/`1N5819` from VBAT to U1 pin 14, **10 µF X7R** U1 pin 14 → GND_RAW, at the IC | HW-042 |
| Q1 replace | IRLZ44N → **SOT-23 logic-level N-FET specified at V_GS = 2.5 V**, mounted flat inside the outline | HW-017 |
| Q1 keep | R1 1 MΩ gate pulldown, R2 1 kΩ gate series — unchanged, both correct | — |

### 1.2 Power latch (replaces the current reed → CLOCK connection)

Full circuit and truth table in `HYDRO-NODE-REFERENCE.md` §9.

| Node | Connection | Issue |
|---|---|---|
| U1 | CD4013BE → **TC74HC74AP** (SOIC-14, not DIP, not socketed) | HW-041, HW-037 |
| Reed S1 | **Reverse it**: node → S1 → **1 kΩ** → GND_RAW | HW-043 |
| Pull-up | **1 MΩ** from VLATCH to the reed node | HW-043 |
| Filter | C1 100 nF → **1 µF**, reed node to GND_RAW | HW-043 |
| Buffer | **74LVC1G14 inverting Schmitt** between reed node and U1 pin 3 (1CLK) | HW-014, HW-043 |
| Delete | **R3 (10 kΩ)** — no longer needed | HW-043 |
| POR | Swap C2 and R4 positions for the active-low CLR: R4 VBAT→pin 1, C2 pin 1→GND_RAW | ref §7.3 |
| Toggle | U1 pin 2 (1D) ← U1 pin 6 (1Q̄) | ref §9.1 |
| Output | U1 pin 5 (1Q) → R2 → Q1 gate | ref §9.1 |
| Tie HIGH | U1 pins 4, 10, 13 (PRE/CLR, **active low**) → VLATCH | ref §9.1 |
| Tie LOW | U1 pins 11, 12 → GND_RAW. **Do not leave floating** | ref §9.1 |
| Sense | Buffer output → 100 Ω → MCU **A0** (magnet sense, input) | HW-043, HW-022 |
| Shutdown | MCU **A1** → 100 kΩ → U1 pin 1 (1CLR). Low-battery auto-shutdown only, normally hi-Z | HW-021, HW-025 |

### 1.3 Connectors

| Change | Detail | Issue |
|---|---|---|
| **J5 re-order** | To **1=GND, 2=Trig, 3=Echo, 4=VCC** so a 1:1 harness is correct | HW-001 |
| Battery connector | Different family/key from the sensor connectors | HW-011 |
| Silkscreen | Function + pin-1 signal name at every connector | HW-038 |

### 1.4 Signal protection — all externally exposed pins

| Change | Detail | Issue |
|---|---|---|
| Series R | **100 Ω** at the MCU end of D4 (1-Wire), D5 (flow), D6 (Trig), D7 (Echo) | HW-012 |
| TVS | Low-capacitance (**< 5 pF**) ESD array to GND_SW on those four lines, placed **at the connectors** | HW-012 |
| Supply TVS | One across the ultrasonic supply feed | HW-012 |

### 1.5 Sensors and I/O

| Change | Detail | Issue |
|---|---|---|
| **Echo → D8** | Swap Echo to D8 (**ICP1**, hardware input capture) and the LED to D7 | HW-018 |
| **2nd DS18B20** | Second sensor on the **same 1-Wire bus** — no extra pin. One at the transducer, one low in the headspace | HW-023 |
| Flow pull-up | External **1 MΩ** to GND_SW + **100 nF** at the connector. Internal pull-up stays **off** | HW-020 |
| Flow wetting | Spare GPIO → **330 Ω** → flow node. 5 ms high before each sample | HW-019 |
| LED | Blue → **red or yellow** (V_f 1.8–2.1 V). R5 = 680 Ω–1 kΩ for ~2 mA | HW-016 |
| Battery sense | **No parts.** Internal 1.1 V bandgap vs V_CC in firmware | HW-025 |

### 1.6 Decoupling — every one placed within a few mm of its pin

| Location | Value | Issue |
|---|---|---|
| Ra-02 3V3 (J1.3) | 100 nF + 10 µF ceramic | HW-013 |
| MCU V_CC | 100 nF | HW-013 |
| U1 pin 14 | 100 nF (plus the 10 µF hold-up from §1.1) | HW-013, HW-042 |
| Ultrasonic supply at J5 | 100 nF + 10 µF | HW-013 |
| Bulk near Ra-02 | 100–220 µF **X5R/X7R ceramic** | HW-009 |
| **Delete C3** | 2200 µF aluminium electrolytic — leaky, poor cold ESR, and cannot buffer a TX burst anyway | HW-009 |

### 1.7 Test and programming

| Add | Detail | Issue |
|---|---|---|
| Test points | VBAT, GND_RAW, GND_SW, VLATCH, Q1 gate, reed node, 1-Wire — fixture-probeable pads | HW-029 |
| Programming | ICSP header, UART TX/RX brought out | HW-029 |
| Serial number | QR/DataMatrix label + value in EEPROM | HW-029 |

---

## 2. PCB FLOORPLAN

Do this **before** any routing. A pour will not rescue bad placement (ref §11.6).

```
   +--------------------------------------------------------------+
   |  [ANTENNA/IPEX]                                   [REED S1]   |  <- reed at the wall,
   |                                                    marked     |     magnet target
   |   +-------------+                                             |
   |   |   Ra-02     |   solid GND_SW under the whole footprint    |
   |   |  (soldered) |   decoupling AT the 3V3 pin                 |
   |   +-------------+                                             |
   |                                                               |
   |                    +-----------+        +----------+          |
   |                    |   MCU     |        | U1 74HC74|          |
   |                    | + crystal |        | U3 Schmitt|         |
   |                    +-----------+        +----------+          |
   |                                                               |
   |  [ICSP] [TEST POINTS]                    [Q1] [BATTERY CONN]  |  <- Q1 ADJACENT to
   |                                                               |     battery connector
   |  [J3 TEMP] [J4 FLOW] [J5 ULTRASONIC]  <- TVS + 100R here      |
   +--------------------------------------------------------------+
        ^ sensor cables exit this edge, away from the RF corner
```

**Rules behind that layout:**

| Rule | Why | Issue |
|---|---|---|
| Q1 **adjacent** to the battery connector | Keeps the GND_RAW segment a few mm instead of crossing the board | HW-004 §11.5 |
| Ra-02 next to the antenna connector, solid pour beneath | Its matching network needs a real ground reference | HW-004, HW-007 |
| Sensor connectors on the **opposite edge** from the RF | Cables are the ESD collector and the interference path | HW-012 |
| Reed at a **board edge**, with an enclosure target mark | Magnet must actuate through the wall — currently it is board-centre | HW-015 |
| Decoupling within a few mm of its pin | Distance defeats the capacitor | HW-013 |
| TVS + 100 Ω **at the connectors** | Transient must not travel across the board | HW-012 |

---

## 3. LAYER AND ROUTING STRATEGY

Current board: 227 tracks on **Bottom**, 6 on **Top**, **0 vias**, **0 polygons**, all tracks 1.0 mm. This has to invert.

| Item | Revision B |
|---|---|
| Signals | **Top layer** |
| Pour | **Bottom layer**, solid, net **GND_SW** |
| GND_RAW | Small **separate** local pour around U1 / S1 / Q1, plus one short wide link Q1 source → battery connector |
| Vias | Every GND pin gets **its own** via. Stitch ~5 mm around the board edge and around the Ra-02 footprint |
| Track width, signal | 0.25 mm |
| Track width, VBAT / GND_RAW link | **≥ 1.5 mm** |

**Never merge the GND_SW and GND_RAW pours** — they are separated by Q1, and merging them bypasses the master power switch.

### Traps that make a pour useless (ref §11.8)

- **No long slots.** A signal routed across the bottom layer splits the pour and forces return current to detour around it, recreating the loop the plane was meant to remove. A slotted plane can be worse than none at that spot. Where a signal must cross to the bottom, keep it short and place ground vias either side.
- **Nothing under the Ra-02 footprint.** Keep that area completely solid.
- **Don't rely on a single thermal-relief spoke** for an RF ground pin — use Direct Connect there.

---

## 4. ALTIUM STEPS

| Step | Menu / action | Settings |
|---|---|---|
| 1 | `Design → Layer Stack Manager` | Confirm 2 layers |
| 2 | Move signal routing to Top | Select bottom tracks → `Edit → Move → Move to Layer` |
| 3 | `Place → Polygon Pour` (**P, G**) | Fill Mode **Solid**, Layer **Bottom**, Connect to Net **GND_SW**, tick **Remove Dead Copper** |
| 4 | Second, smaller pour | Same dialog, net **GND_RAW**, around U1 / S1 / Q1 only |
| 5 | `Design → Rules → Plane → Polygon Connect Style` | **Direct Connect** for Ra-02 ground pins; **Relief Connect** for hand-soldered through-hole |
| 6 | `Design → Rules → Routing → Width` | Signal 0.25 mm; VBAT / GND_RAW ≥ 1.5 mm |
| 7 | `Design → Rules → Electrical → Clearance` | 0.2 mm minimum (confirm against your fab's capability) |
| 8 | `Tools → Via Stitching/Shielding → Add Stitching to Net` | Net **GND_SW**, ~5 mm grid |
| 9 | `Tools → Polygon Pours → Repour All` | — |
| 10 | `Tools → Design Rule Check` | Fix everything, then visually scan for isolated islands and slots |

---

## 5. BOM DELTA

### Add

| Ref | Part | Purpose | Issue |
|---|---|---|---|
| D2, D3 | **`1N5819`** ×2 | Cell isolation | HW-003 |
| F1 | 0.5 A fast-blow (`0451.500`) | Pack protection | HW-003 |
| D1 | `BAT54` or `1N5819` | Latch rail hold-up | HW-042 |
| C_hold | 10 µF X7R | Latch rail hold-up | HW-042 |
| U3 | **`74LVC1G14`** inverting Schmitt | Reed debounce. **Select on I_CC — verify at 3.6 V / 60 °C** | HW-043 |
| R_pu | 1 MΩ | Reed pull-up (drops standby 360 µA → 3.6 µA) | HW-043 |
| R_s | 1 kΩ | Reed series | HW-043 |
| R_sn, R_mcu | 100 Ω, 100 kΩ | MCU sense / shutdown | HW-043, HW-021 |
| — | 100 Ω ×4 | Series protection D4–D8 | HW-012 |
| — | Low-cap TVS array ×2–3 | ESD/surge | HW-012 |
| R_wet | 330 Ω | Flow wetting pulse | HW-019 |
| — | 1 MΩ + 100 nF | Flow pull-up + filter | HW-020 |
| — | Second DS18B20 | Thermal gradient | HW-023 |
| — | Bulk 100–220 µF ceramic + local decoupling | | HW-013, HW-009 |
| — | Crystal ±30 ppm | Timing accuracy | HW-024 |
| — | ICSP header, test points | Production test | HW-029 |

### Change

| Ref | From | To | Issue |
|---|---|---|---|
| U1 | CD4013BE (DIP) | **TC74HC74AP** → SOIC-14 for production | HW-041, HW-037 |
| Q1 | IRLZ44N TO-220 | SOT-23 logic-level, spec'd at V_GS 2.5 V | HW-017 |
| DS1 | Blue LED | **Red or yellow**, R5 680 Ω–1 kΩ | HW-016 |
| C1 | 100 nF | **1 µF** | HW-043 |
| R4 | 100 kΩ | 1 MΩ (or keep 100 kΩ with 1 µF C_por) | ref §7.3 |
| R1–R6 | Carbon film ½ W axial | 0603 1 % metal film | HW-036 |

### Delete

| Ref | Why | Issue |
|---|---|---|
| **C3** | 2200 µF electrolytic — leaks, poor cold ESR, cannot buffer a TX burst | HW-009 |
| **R3** | Superseded by the new reed network | HW-043 |

### Add to BOM documentation (currently missing)

Ra-02 headers, battery holder/tabs, PCB, cable glands, enclosure hardware, conformal coating, desiccant, gasket, antenna, harness drawings. Every line needs a **manufacturer part number and lifecycle status** — four documentation-vs-hardware mismatches have already been found (HW-033).

---

## 6. VERIFICATION BEFORE RELEASE

| # | Check | Pass criterion | Issue |
|---|---|---|---|
| 1 | Sleep current, bench supply | **≤ 25 µA** | HW-002, HW-035 |
| 2 | Standby with magnet held on reed | ~3.6 µA | HW-043 |
| 3 | VBAT droop at U1 pin 14 during real TX, week-idle cells, cold | Stays above 2.0 V | HW-042 |
| 4 | `1N5819` V_F at 50 mA, measured | Confirms ref §10.4 headroom | HW-003 |
| 5 | Toggle determinism, 200 magnet approaches through the enclosure wall | **Zero** double-toggles | HW-043 |
| 6 | MCU RESET pulled low while on | Latch holds, device recovers | HW-043 |
| 7 | Blind-zone sweep, 2 cm outward in 5 mm steps | Sets the mounting standoff — **do this first, it is cheap** | HW-051 |
| 8 | Transducer spacing *s* measured with callipers | Feeds the parallax correction | HW-052 |
| 9 | RSSI at a real 50 m concrete installation | Sets SF and TX power | HW-047 |
| 10 | Every Ra-02 GND pin connected | J1.1, J1.2, J2.1, J2.8 | HW-007 |
| 11 | Isolated-copper-island scan after repour | None | HW-004 |
| 12 | LED visible through the enclosure wall in daylight **at 3.0 V** | Not just at 3.6 V | HW-016, HW-044 |

**Test 7 needs no new parts and gates the mechanical design. Run it first.**
