# HYDRO NODE — COMPLETE BUILD SHEET
Everything to add, remove, replace, and every wire. Final prototype build.

**Read Section 0 before you pick up the soldering iron.** There is one part you may not have, and it changes what you build.

---

## 0. THE ONE DECISION — READ FIRST

The improved reed circuit needs a **Schmitt-trigger inverter**. Without it, the reed's slow edge feeds the flip-flop's clock input directly and the on/off toggle becomes unreliable.

**Do you have, or can you buy today, a `74HC14` (hex Schmitt inverter, DIP-14)?**

| | Build |
|---|---|
| **YES** | **Circuit A** — the full improved latch. Fixes the double-toggle problem and drops magnet-held current from 360 µA to 3.6 µA. |
| **NO** | **Circuit B** — keep the reed wired as it is now. Everything else on this sheet still applies. Issues HW-014 and HW-043 stay open and get fixed on the PCB. |

Circuit A is section 5A. Circuit B is section 5B. **Do one or the other, not both.**

Any hex Schmitt inverter in DIP works: `74HC14`, `74HCT14` (needs 4.5 V — **no**, do not use), `CD40106` (works, 3–18 V). **`74HC14` or `CD40106`.**

---

## 1. PARTS — WHAT TO HAVE ON THE BENCH

### Keep from the current build

| Part | Note |
|---|---|
| Arduino Pro Mini 3.3 V | Power LED and regulator already removed ✓ |
| Ra-02 SX1278 module | |
| Reed switch S1 | |
| IRLZ44N (Q1) | Fine for the prototype. Gets replaced on the PCB (HW-017) |
| R1 1 MΩ, R2 1 kΩ | Gate pulldown and gate series — unchanged |
| R6 4.7 kΩ | 1-Wire pull-up |
| C4, C5 — 100 nF | |
| RCWL-1670, WY-90, DS18B20 | |
| JST connectors | |
| 2 × LS14500 | |

### Buy / find

| Qty | Part | For |
|---|---|---|
| 1 | **`74HC14`** or `CD40106` (DIP-14) | Reed Schmitt buffer — Circuit A only |
| 3 | **`1N5819`** Schottky | 2 for cell isolation, 1 for the latch rail |
| 1 | **0.5 A fuse** + holder | Battery pack protection |
| 1 | **DS18B20** (second one) | Thermal gradient (HW-023) |
| 1 | **Red or yellow LED** | Blue has no headroom at 3.0 V (HW-016) |
| 2 | 1 MΩ resistor | Reed pull-up, flow pull-up |
| 1 | 1 kΩ resistor | Reed series |
| 1 | 1 kΩ resistor | LED series (replaces 330 Ω) |
| 1 | 100 kΩ resistor | MCU shutdown line |
| 5 | **100 Ω resistors** | Series protection on D4, D5, D6, D8, A0 |
| 1 | 330 Ω resistor | Flow wetting pulse |
| 1 | 1 µF capacitor | Reed filter (replaces C1 100 nF) |
| 1 | 10 µF ceramic | Latch rail hold-up |
| 1 | 100 µF ceramic (or 47 µF) | Bulk near the radio (replaces C3) |
| 5 | 100 nF ceramic | Decoupling |
| 1 | 10 µF ceramic | Decoupling at the radio |

**R4 100 kΩ** from the old build gets reused, but with a **1 µF** cap instead of 100 nF — see §5A. If you have a 1 MΩ spare, use that with the existing 100 nF instead. Either gives the same ~100 ms reset.

### Do NOT fit

| Part | Why |
|---|---|
| **CD4013BE** | Replaced by the 74HC74 — 2 V minimum instead of 3 V (HW-041) |
| **C3, 2200 µF electrolytic** | Leaks, poor cold ESR, and cannot buffer a transmit burst anyway (HW-009) |
| **R3, 10 kΩ** | Circuit A only — superseded by the new reed network |
| **Blue LED, R5 330 Ω** | Replaced by red + 1 kΩ |

---

## 2. THE FIVE RAILS

Everything below refers to these names. Write them on masking tape and stick it to the board.

| Name | What it is |
|---|---|
| **VBAT** | Battery positive, after the diodes and fuse |
| **GND_RAW** | Battery negative |
| **GND_SW** | Switched ground — the MOSFET drain. **Everything that turns off connects here** |
| **VLATCH** | VBAT through a Schottky — powers the flip-flop and Schmitt buffer only |
| **REED_N** | The reed switch node |

> ⚠️ **GND_RAW and GND_SW are two different rails.** They are separated by the MOSFET — that separation *is* the power switch. If you join them anywhere, the device can never turn off. This is the single easiest way to ruin the build.

---

## 3. BATTERY PACK

Build this as a sealed unit with one connector (HW-003).

```
Cell 1 (+) ──|>|── 1N5819 ──┐
                            ├── FUSE 0.5 A ── VBAT
Cell 2 (+) ──|>|── 1N5819 ──┘

Cell 1 (−) ─────────────────┬── GND_RAW
Cell 2 (−) ─────────────────┘
```

Diode **band (cathode) faces the fuse**, away from the cell. Wrong way round and nothing powers up.

---

## 4. POWER PATH

| From | To | Via |
|---|---|---|
| VBAT | Pro Mini **VCC** pin | direct |
| VBAT | Ra-02 **3V3** (J1 pin 3) | direct |
| VBAT | Ultrasonic **VCC** | direct |
| VBAT | **VLATCH** | **1N5819**, band toward VLATCH |
| VLATCH | 74HC74 pin 14 | direct |
| VLATCH | 74HC14 pin 14 | direct (Circuit A) |
| GND_RAW | Q1 **source** (middle pin, IRLZ44N) | short and thick |
| GND_SW | Q1 **drain** (tab / left pin) | |
| GND_RAW | 74HC74 pin 7, 74HC14 pin 7 | |
| GND_SW | Pro Mini **GND** | |
| GND_SW | Ra-02 GND — **all four**: J1.1, J1.2, J2.1, J2.8 | HW-007 — not just one |
| GND_SW | Ultrasonic GND, temp GND, flow GND, LED cathode | |

**IRLZ44N pinout, facing the printed side, legs down:** left = **Gate**, middle = **Drain**, right = **Source**. The metal tab is Drain.
Correction for wiring: **Gate ← R2**, **Drain → GND_SW**, **Source → GND_RAW**.

### Capacitors

| Value | Between | Where physically |
|---|---|---|
| 10 µF ceramic | VLATCH ↔ GND_RAW | at 74HC74 pins 14/7 |
| 100 nF | VLATCH ↔ GND_RAW | at 74HC74 pins 14/7 |
| 100 nF | VLATCH ↔ GND_RAW | at 74HC14 pins 14/7 |
| 100 nF (C4) | VBAT ↔ GND_RAW | near the battery connector |
| 100 nF (C5) | VBAT ↔ GND_SW | |
| 100 nF | VBAT ↔ GND_SW | at the Pro Mini VCC/GND pins |
| **100 µF ceramic + 10 µF + 100 nF** | VBAT ↔ GND_SW | **as close to the Ra-02 3V3 pin as you can get** |
| 100 nF | VBAT ↔ GND_SW | at the ultrasonic connector |

The radio's capacitors matter most. Short legs, right at the pin.

---

## 5A. POWER LATCH — CIRCUIT A (with 74HC14)

### 74HC74 — pin by pin

| Pin | Name | Connect to |
|---|---|---|
| 1 | 1CLR | **R_por** to VLATCH, **C_por** to GND_RAW, **100 kΩ** to Pro Mini **A1** |
| 2 | 1D | **pin 6** |
| 3 | 1CLK | **74HC14 pin 2** |
| 4 | 1PRE | **VLATCH** |
| 5 | 1Q | **R2 (1 kΩ)** → Q1 gate |
| 6 | 1Q̄ | **pin 2** |
| 7 | GND | GND_RAW |
| 8 | 2Q̄ | *leave open* |
| 9 | 2Q | *leave open* |
| 10 | 2PRE | **VLATCH** |
| 11 | 2CLK | **GND_RAW** |
| 12 | 2D | **GND_RAW** |
| 13 | 2CLR | **VLATCH** |
| 14 | VCC | VLATCH |

> Pins **4, 10, 13** go to **VLATCH**, not ground. They are active-LOW. Grounding them holds the chip in reset and nothing will ever switch on. This is the most common mistake when moving from the CD4013.

**R_por / C_por:** either 1 MΩ + 100 nF, or the old R4 100 kΩ + 1 µF. Both give ~100 ms.

### Reed network

| From | To |
|---|---|
| VLATCH | **1 MΩ** → **REED_N** |
| REED_N | **1 µF** → GND_RAW |
| REED_N | reed switch → **1 kΩ** → GND_RAW |
| REED_N | **74HC14 pin 1** |
| 74HC14 **pin 2** | 74HC74 pin 3, **and** 100 Ω → Pro Mini **A0** |

**74HC14 unused inputs — pins 3, 5, 9, 11, 13 → GND_RAW.** Do not leave them floating; floating CMOS inputs draw current continuously and will wreck the sleep budget. Unused outputs (4, 6, 8, 10, 12) stay open.

**Q1 gate:** R1 1 MΩ from gate to GND_RAW. R2 1 kΩ from gate to 74HC74 pin 5.

**Behaviour:** magnet near → reed closes → REED_N goes low → inverter output goes high → rising edge clocks the flip-flop → toggles. Magnet away → REED_N rises slowly over ~1 s → falling edge → ignored. Bounce during the approach is swallowed by the 1 s recovery.

---

## 5B. POWER LATCH — CIRCUIT B (no 74HC14)

Keep the reed exactly as it is now: **reed between VBAT and REED_N**, **10 kΩ (R3) from REED_N to GND_RAW**, **100 nF from REED_N to GND_RAW**.

74HC74 wiring is identical to §5A **except**:

| Pin | Connect to |
|---|---|
| 3 (1CLK) | **REED_N directly** (no buffer) |

No A0 sense line in this version.

Accepted for now: bounce can produce a double-toggle, and a magnet resting on the enclosure draws ~360 µA. Both get fixed on the PCB.

---

## 6. RADIO — Ra-02

| Ra-02 pin | Pro Mini |
|---|---|
| 3V3 (J1.3) | VBAT |
| **GND — J1.1, J1.2, J2.1, J2.8** | **GND_SW — all four** |
| RST (J1.4) | D9 |
| DIO0 (J1.5) | D2 |
| NSS (J2.2) | D10 |
| MOSI (J2.3) | D11 |
| MISO (J2.4) | D12 |
| SCK (J2.5) | D13 |

DIO1, DIO2, DIO3, DIO4, DIO5 — leave open.

---

## 7. SENSORS

### Ultrasonic — RCWL-1670

Module pads left to right are **GND, RX, TX, +5V**. RX is TRIG, TX is ECHO.

| Module pad | Goes to |
|---|---|
| GND | GND_SW |
| **RX (Trig)** | 100 Ω → Pro Mini **D6** |
| **TX (Echo)** | 100 Ω → Pro Mini **D8** |
| +5V | VBAT |

> **Echo is on D8, not D7.** D8 is the hardware input-capture pin (HW-018). Wire it there.
> **Watch the cable order.** The schematic's connector order does not match the module (HW-001). Wire by the function names above, ignore pin numbers.

### Temperature — two DS18B20 in parallel

Both sensors, wired identically:

| Sensor wire | Goes to |
|---|---|
| Red (VDD) | Pro Mini **D3** |
| Black/blue (GND) | GND_SW |
| Yellow/white (DATA) | **TEMP_BUS** |

| From | To |
|---|---|
| TEMP_BUS | **4.7 kΩ (R6)** → D3 |
| TEMP_BUS | 100 Ω → Pro Mini **D4** |

One sensor at the transducer, one on a longer lead reaching low into the tank headspace. **Verify each probe's wire colours with a multimeter before soldering** — clone probes are not consistent, and swapped VDD/DATA destroys the sensor.

### Flow — WY-90

| From | To |
|---|---|
| Flow switch, wire 1 | **FLOW_N** |
| Flow switch, wire 2 | GND_SW |
| FLOW_N | **1 MΩ** → VBAT |
| FLOW_N | **100 nF** → GND_SW |
| FLOW_N | 100 Ω → Pro Mini **D5** |
| FLOW_N | **330 Ω** → Pro Mini **A2** |

No polarity — either wire either way.
**Install with the yellow arrow pointing downstream.** Backwards, it never closes, and the failure looks identical to "nobody used water."

### Status LED

| From | To |
|---|---|
| Pro Mini **D7** | **1 kΩ** → LED anode |
| LED cathode | GND_SW |

Red or yellow. **Not blue** — it stops lighting near 3.0 V, exactly when you need to see it.

---

## 8. FULL PRO MINI PIN MAP

| Pin | Goes to | Via |
|---|---|---|
| VCC | VBAT | |
| GND | GND_SW | |
| RAW | **nothing** | regulator bypassed |
| D2 | Ra-02 DIO0 | |
| D3 | DS18B20 VDD (both) + 4.7 kΩ | |
| D4 | TEMP_BUS | 100 Ω |
| D5 | FLOW_N | 100 Ω |
| D6 | Ultrasonic **Trig** | 100 Ω |
| D7 | LED | 1 kΩ |
| **D8** | Ultrasonic **Echo** | 100 Ω |
| D9 | Ra-02 RST | |
| D10 | Ra-02 NSS | |
| D11 | Ra-02 MOSI | |
| D12 | Ra-02 MISO | |
| D13 | Ra-02 SCK | |
| **A0** | 74HC14 pin 2 (magnet sense) | 100 Ω — Circuit A only |
| **A1** | 74HC74 pin 1 (shutdown) | 100 kΩ |
| **A2** | FLOW_N (wetting pulse) | 330 Ω |
| A3–A7, D0, D1, RST | free | |

---

## 9. BUILD ORDER

1. **Ground mesh first** if you are doing one — before any component goes down. GND_SW only. Keep the latch corner (74HC74, 74HC14, reed, Q1, battery connector) as a separate island.
2. Battery pack: diodes, fuse, connector. Test it on its own with a meter before it goes near the board.
3. Power rails: VBAT, GND_RAW, GND_SW, Q1, VLATCH diode, all decoupling.
4. Latch: 74HC74, 74HC14, reed network, POR.
5. **Stop and test** — §10.
6. Pro Mini, then the radio, then the sensor connectors.

---

## 10. CHECKS BEFORE YOU CONNECT THE BATTERY

Multimeter, no power. **Every one of these has to pass.**

| # | Measure | Must read |
|---|---|---|
| 1 | GND_RAW ↔ GND_SW | **OPEN.** Any continuity means you merged the two grounds — find it before powering up |
| 2 | VBAT ↔ GND_RAW | Open, or a slow rise as caps charge. Never a dead short |
| 3 | VBAT ↔ GND_SW | Same |
| 4 | Battery pack output polarity | + on the pin you expect |
| 5 | 74HC74 pin 14 to VLATCH, pin 7 to GND_RAW | continuity |
| 6 | 74HC74 pins 4, 10, 13 → VLATCH | continuity. **Not ground** |
| 7 | 74HC74 pins 11, 12 → GND_RAW | continuity |
| 8 | 74HC14 pins 3, 5, 9, 11, 13 → GND_RAW | continuity (Circuit A) |
| 9 | All four Ra-02 GND pads → GND_SW | continuity |
| 10 | Pro Mini RAW pin | connected to nothing |

### First power-up

1. If you have a bench supply, use it at 3.6 V with the current limit at **50 mA** before using cells.
2. On connecting power the device should be **OFF** — the power-on reset holds it off. GND_SW should read close to VBAT, not 0 V.
3. Touch the magnet to the reed. The device should switch **on**: GND_SW drops to near 0 V.
4. Remove the magnet. It stays on.
5. Touch again. It switches off.
6. Repeat ten times. Every single approach should change the state exactly once.

If step 6 fails on Circuit B, that is the known bounce problem and it is expected — note it and carry on.

---

## 11. WHAT THIS BUILD DOES NOT FIX

So you know what is still outstanding, and none of it is worth another rebuild:

| Left open | Why it waits |
|---|---|
| Ground plane (HW-004) | Mesh on the prototype is a partial fix; the real one is the PCB |
| SMD, MCU on board (HW-026) | PCB only |
| TVS on the sensor lines (HW-012) | The 100 Ω resistors are the important half and they are in this build |
| Q1 replacement (HW-017) | IRLZ44N works at 3.6 V, just uncharacterised. PCB |
| Test points, ICSP (HW-029) | PCB |
| Crystal vs resonator (HW-024) | Pro Mini is what it is |

---

## 12. RECORD BEFORE YOU SEAL IT

While the board is still open and easy to probe:

- **Sleep current** on a bench supply — target ≤ 25 µA (HW-002)
- **Magnet-held current** — ~3.6 µA on Circuit A, ~360 µA on Circuit B (HW-043)
- **VBAT during a real transmission**, on cells left idle a week — must stay above 2.0 V at the 74HC74 (HW-042)
- **Ultrasonic blind zone** — flat target, 2 cm outward in 5 mm steps (HW-051)
- **Transducer spacing** with callipers (HW-052)

Those five numbers close or escalate five open issues. Take them now; you will not want to open the enclosure again.
