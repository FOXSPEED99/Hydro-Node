# HYDRO NODE — BUILD SHEET

---

## 1. REMOVE — do not fit these

| Part | |
|---|---|
| CD4013BE | replaced by 74HC74 |
| C3 — 2200 µF electrolytic | not fitted at all |
| Blue LED | replaced by red |
| R5 — 220 Ω / 330 Ω | replaced by 1 kΩ |
| R3 — 10 kΩ | replaced by 470 kΩ |

## 2. REUSE from the old build

Arduino Pro Mini 3.3 V (LED + regulator already removed) · Ra-02 · reed switch · IRLZ44N · R1 1 MΩ · R2 1 kΩ · R6 4.7 kΩ · R4 100 kΩ · C1 100 nF · C2 100 nF · C4 100 nF · C5 100 nF · all JST connectors · RCWL-1670 · WY-90 · DS18B20 · 2 × LS14500

## 3. BUY / FIND

| Qty | Part |
|---|---|
| 1 | 74HC74 (you have it) |
| 3 | 1N5819 |
| 1 | 0.5 A fuse + holder |
| 1 | DS18B20 — second one |
| 1 | Red or yellow LED |
| 1 | 470 kΩ |
| 2 | 1 MΩ |
| 1 | 1 kΩ |
| 6 | 100 Ω |
| 1 | 330 Ω |
| 5 | 100 nF ceramic |
| 2 | 10 µF ceramic |
| 1 | 100 µF ceramic |

---

## 4. RAIL NAMES

| Name | |
|---|---|
| VBAT | battery + after fuse |
| GND_RAW | battery − |
| GND_SW | MOSFET drain |
| VLATCH | VBAT through D1 |
| REED_N | reed node |
| TEMP_BUS | 1-Wire data |
| TEMP_VCC | Pro Mini D3 |
| FLOW_N | flow switch node |
| U_TRIG / U_ECHO | ultrasonic signals |

**GND_RAW and GND_SW never touch.**

---

## 5. PART NUMBERING USED BELOW

| Ref | Value |
|---|---|
| R1 | 1 MΩ |
| R2 | 1 kΩ |
| R3 | 470 kΩ |
| R5 | 1 kΩ |
| R6 | 4.7 kΩ |
| R7 | 100 kΩ |
| R8–R13 | 100 Ω |
| R14 | 330 Ω |
| R15 | 1 MΩ |
| R16 | 1 MΩ |
| C1, C2, C4, C5, C11, C12, C13, C16, C17 | 100 nF |
| C10, C14 | 10 µF |
| C15 | 100 µF |
| D1, D2, D3 | 1N5819 |
| U1 | 74HC74 |
| U2 | Pro Mini |
| S1 | reed |
| Q1 | IRLZ44N |

---

## 6. BATTERY PACK

| From | To |
|---|---|
| Cell 1 + | D2 anode |
| D2 cathode (band) | FUSE end A |
| Cell 2 + | D3 anode |
| D3 cathode (band) | FUSE end A |
| FUSE end B | VBAT |
| Cell 1 − | GND_RAW |
| Cell 2 − | GND_RAW |

---

## 7. POWER

| From | To |
|---|---|
| VBAT | D1 anode |
| D1 cathode (band) | VLATCH |
| VLATCH | C10 (10 µF) → GND_RAW |
| VLATCH | C11 (100 nF) → GND_RAW |
| VBAT | C4 (100 nF) → GND_RAW |
| VBAT | C5 (100 nF) → GND_SW |

### Q1 IRLZ44N — label side facing you, legs down: **left = Gate, middle = Drain, right = Source**

| From | To |
|---|---|
| Q1 Gate (left) | R2 (1 kΩ) → U1 pin 5 |
| Q1 Gate (left) | R1 (1 MΩ) → GND_RAW |
| Q1 Drain (middle + tab) | GND_SW |
| Q1 Source (right) | GND_RAW |

---

## 8. U1 — 74HC74

| Pin | To |
|---|---|
| 1 | R16 (1 MΩ) → VLATCH |
| 1 | C2 (100 nF) → GND_RAW |
| 1 | R7 (100 kΩ) → U2 A1 |
| 2 | pin 6 |
| 3 | REED_N |
| 4 | **VLATCH** |
| 5 | R2 (1 kΩ) → Q1 Gate |
| 6 | pin 2 |
| 7 | GND_RAW |
| 8 | open |
| 9 | open |
| 10 | **VLATCH** |
| 11 | GND_RAW |
| 12 | GND_RAW |
| 13 | **VLATCH** |
| 14 | VLATCH |

Pins 4, 10, 13 → VLATCH. Not ground.

---

## 9. REED

| From | To |
|---|---|
| VLATCH | S1 leg 1 |
| S1 leg 2 | R8 (100 Ω) → REED_N |
| REED_N | R3 (470 kΩ) → GND_RAW |
| REED_N | C1 (100 nF) → GND_RAW |
| REED_N | U1 pin 3 |
| REED_N | R9 (100 Ω) → U2 A0 |

---

## 10. U2 — ARDUINO PRO MINI

| Pin | To |
|---|---|
| VCC | VBAT |
| GND | GND_SW |
| RAW | nothing |
| D2 | J1 pin 5 |
| D3 | TEMP_VCC |
| D4 | R10 (100 Ω) → TEMP_BUS |
| D5 | R11 (100 Ω) → FLOW_N |
| D6 | R12 (100 Ω) → U_TRIG |
| D7 | R5 (1 kΩ) → LED anode |
| D8 | R13 (100 Ω) → U_ECHO |
| D9 | J1 pin 4 |
| D10 | J2 pin 2 |
| D11 | J2 pin 3 |
| D12 | J2 pin 4 |
| D13 | J2 pin 5 |
| A0 | R9 (100 Ω) → REED_N |
| A1 | R7 (100 kΩ) → U1 pin 1 |
| A2 | R14 (330 Ω) → FLOW_N |
| A3–A7, D0, D1, RST, DTR | nothing |

Plus: C12 (100 nF) across VCC ↔ GND_SW, at the pins.

---

## 11. Ra-02

| Ra-02 pin | To |
|---|---|
| J1 pin 1 (GND) | GND_SW |
| J1 pin 2 (GND) | GND_SW |
| J1 pin 3 (3V3) | VBAT |
| J1 pin 4 (RST) | U2 D9 |
| J1 pin 5 (DIO0) | U2 D2 |
| J1 pin 6, 7, 8 | open |
| J2 pin 1 (GND) | GND_SW |
| J2 pin 2 (NSS) | U2 D10 |
| J2 pin 3 (MOSI) | U2 D11 |
| J2 pin 4 (MISO) | U2 D12 |
| J2 pin 5 (SCK) | U2 D13 |
| J2 pin 6, 7 | open |
| J2 pin 8 (GND) | GND_SW |

At the J1 pin 3 / J2 pin 8 corner, as close as possible:

| From | To |
|---|---|
| VBAT | C13 (100 nF) → GND_SW |
| VBAT | C14 (10 µF) → GND_SW |
| VBAT | C15 (100 µF) → GND_SW |

---

## 12. ULTRASONIC — RCWL-1670

Module pads left → right: **GND, RX, TX, +5V**

| Module pad | To |
|---|---|
| GND | GND_SW |
| RX | U_TRIG |
| TX | U_ECHO |
| +5V | VBAT |

Plus: C16 (100 nF) VBAT ↔ GND_SW at the connector.

---

## 13. TEMPERATURE — 2 × DS18B20 in parallel

| From | To |
|---|---|
| Sensor 1 red | TEMP_VCC |
| Sensor 1 black | GND_SW |
| Sensor 1 yellow | TEMP_BUS |
| Sensor 2 red | TEMP_VCC |
| Sensor 2 black | GND_SW |
| Sensor 2 yellow | TEMP_BUS |
| TEMP_BUS | R6 (4.7 kΩ) → TEMP_VCC |

Check each probe's wire colours with a meter before soldering.
Sensor 1 at the transducer. Sensor 2 on a longer lead, low in the tank headspace.

---

## 14. FLOW — WY-90

| From | To |
|---|---|
| Flow wire 1 | FLOW_N |
| Flow wire 2 | GND_SW |
| FLOW_N | R15 (1 MΩ) → VBAT |
| FLOW_N | C17 (100 nF) → GND_SW |

No polarity. Install with the yellow arrow pointing downstream.

---

## 15. LED

| From | To |
|---|---|
| U2 D7 | R5 (1 kΩ) → LED anode |
| LED cathode | GND_SW |

---

## 16. SOLDER ORDER

1. Ground mesh (GND_SW only) — before any component
2. Battery pack, test with meter alone
3. VBAT, GND_RAW, GND_SW, Q1, D1, all capacitors
4. U1 + reed network
5. **Stop, run section 17**
6. U2, then Ra-02, then sensor connectors

---

## 17. CHECKS — meter, no power

| # | Measure | Must read |
|---|---|---|
| 1 | GND_RAW ↔ GND_SW | **OPEN** |
| 2 | VBAT ↔ GND_RAW | no short |
| 3 | VBAT ↔ GND_SW | no short |
| 4 | Pack output polarity | + on expected pin |
| 5 | U1 pin 14 ↔ VLATCH | continuity |
| 6 | U1 pin 7 ↔ GND_RAW | continuity |
| 7 | U1 pins 4, 10, 13 ↔ VLATCH | continuity |
| 8 | U1 pins 11, 12 ↔ GND_RAW | continuity |
| 9 | REED_N ↔ GND_RAW | 470 kΩ |
| 10 | J1.1, J1.2, J2.1, J2.8 ↔ GND_SW | all four continuity |
| 11 | U2 RAW pin | open |

## 18. FIRST POWER-UP

Bench supply 3.6 V, current limit 50 mA.

| Step | Expect |
|---|---|
| 1. Connect power | Device OFF. GND_SW ≈ VBAT |
| 2. Magnet to reed | Device ON. GND_SW ≈ 0 V |
| 3. Remove magnet | Stays ON |
| 4. Magnet again | OFF |
| 5. Repeat ×10 | Exactly one state change per approach |

If step 5 fails: confirm R3 = 470 kΩ and R8 = 100 Ω fitted.

---

## 19. MEASURE BEFORE SEALING

| Measure | Expect |
|---|---|
| Sleep current | ≤ 25 µA |
| Magnet held on reed | ~7.7 µA |
| VBAT at U1 pin 14 during TX, week-old cells | > 2.0 V |
| Ultrasonic blind zone — flat target, 2 cm out in 5 mm steps | record |
| Transducer spacing, callipers | record |
