# HYDRO NODE — BUILD SHEET

---

## 1. REMOVE — do not fit

| Part |
|---|
| CD4013BE |
| C3 — 2200 µF electrolytic |
| Blue LED |
| R5 — 220 Ω / 330 Ω |
| R3 — 10 kΩ |

## 2. REUSE from the old build

Arduino Pro Mini 3.3 V (LED + regulator already removed) · Ra-02 · reed switch · IRLZ44N · 1 MΩ · 1 kΩ · 4.7 kΩ · 100 kΩ · 4 × 100 nF · all JST connectors · RCWL-1670 · WY-90 · DS18B20 · 2 × LS14500

## 3. BUY

| Qty | Part |
|---|---|
| 3 | 1N5819 diode |
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

## 4. FIRST — BUILD 4 RAILS

Run 4 bare wires across the board. Everything else solders onto one of these. Label them with tape.

| Rail | What it is |
|---|---|
| **RAIL-BAT+** | battery plus, after the fuse |
| **RAIL-BAT−** | battery minus |
| **RAIL-SW** | switched ground — this is the MOSFET's middle leg |
| **RAIL-LATCH** | a small rail, only 6 things connect to it |

> **RAIL-BAT− and RAIL-SW must never touch each other.**

---

## 5. THEN — 5 JUNCTION POINTS

These are not rails. Each is one solder blob on an empty hole where several legs meet. Make the blob first, then bring legs to it.

| Junction | Legs that meet here (count them when done) |
|---|---|
| **J-GATE** | 3 legs — MOSFET gate, 1 MΩ, 1 kΩ |
| **J-RESET** | 4 legs — 74HC74 pin 1, 1 MΩ, 100 nF, 100 kΩ |
| **J-REED** | 5 legs — 100 Ω from reed, 470 kΩ, 100 nF, wire to 74HC74 pin 3, 100 Ω to A0 |
| **J-TEMP** | 3 legs — 4.7 kΩ, 100 Ω from D4, wire to temp connector DATA pin |
| **J-FLOW** | 5 legs — wire from flow connector, 1 MΩ, 100 nF, 100 Ω from D5, 330 Ω from A2 |

---

## 6. WIRE LIST

Tick each one. "Leg 1 / leg 2" just means the two ends of that part — resistors and ceramic caps have no polarity.

### Battery pack

| # | Solder this | To this |
|---|---|---|
| 1 | Cell 1 **+** | 1N5819 **plain end** (no stripe) |
| 2 | that diode's **striped end** | fuse leg 1 |
| 3 | Cell 2 **+** | second 1N5819 **plain end** |
| 4 | that diode's **striped end** | fuse leg 1 — same blob as #2 |
| 5 | fuse leg 2 | **RAIL-BAT+** |
| 6 | Cell 1 **−** | **RAIL-BAT−** |
| 7 | Cell 2 **−** | **RAIL-BAT−** |

### Latch rail

| # | Solder this | To this |
|---|---|---|
| 8 | **RAIL-BAT+** | third 1N5819 **plain end** |
| 9 | that diode's **striped end** | **RAIL-LATCH** |
| 10 | 10 µF leg 1 | **RAIL-LATCH** |
| 11 | 10 µF leg 2 | **RAIL-BAT−** |
| 12 | 100 nF leg 1 | **RAIL-LATCH** |
| 13 | 100 nF leg 2 | **RAIL-BAT−** |

### MOSFET — IRLZ44N, printed side facing you, legs pointing down

Left leg = **gate**. Middle leg + metal tab = **drain**. Right leg = **source**.

| # | Solder this | To this |
|---|---|---|
| 14 | MOSFET **middle** leg | **RAIL-SW** |
| 15 | MOSFET **right** leg | **RAIL-BAT−** |
| 16 | MOSFET **left** leg | **J-GATE** |
| 17 | 1 MΩ leg 1 | **J-GATE** |
| 18 | 1 MΩ leg 2 | **RAIL-BAT−** |
| 19 | 1 kΩ leg 1 | **J-GATE** |
| 20 | 1 kΩ leg 2 | **74HC74 pin 5** |

### 74HC74 — notch at the top, pin 1 is top-left, count down the left side

| # | Solder this | To this |
|---|---|---|
| 21 | pin 14 | **RAIL-LATCH** |
| 22 | pin 7 | **RAIL-BAT−** |
| 23 | pin 4 | **RAIL-LATCH** |
| 24 | pin 10 | **RAIL-LATCH** |
| 25 | pin 13 | **RAIL-LATCH** |
| 26 | pin 11 | **RAIL-BAT−** |
| 27 | pin 12 | **RAIL-BAT−** |
| 28 | pin 2 | **pin 6** — short wire across the chip |
| 29 | pin 3 | **J-REED** |
| 30 | pin 1 | **J-RESET** |
| 31 | 1 MΩ leg 1 | **J-RESET** |
| 32 | 1 MΩ leg 2 | **RAIL-LATCH** |
| 33 | 100 nF leg 1 | **J-RESET** |
| 34 | 100 nF leg 2 | **RAIL-BAT−** |
| 35 | 100 kΩ leg 1 | **J-RESET** |
| 36 | 100 kΩ leg 2 | **Pro Mini A1** |
| — | pins 8 and 9 | leave empty |

Pins 4, 10 and 13 go to **RAIL-LATCH**. Not to ground.

### Reed switch

| # | Solder this | To this |
|---|---|---|
| 37 | reed leg 1 | **RAIL-LATCH** |
| 38 | reed leg 2 | 100 Ω leg 1 |
| 39 | that 100 Ω leg 2 | **J-REED** |
| 40 | 470 kΩ leg 1 | **J-REED** |
| 41 | 470 kΩ leg 2 | **RAIL-BAT−** |
| 42 | 100 nF leg 1 | **J-REED** |
| 43 | 100 nF leg 2 | **RAIL-BAT−** |
| 44 | 100 Ω leg 1 | **J-REED** |
| 45 | that 100 Ω leg 2 | **Pro Mini A0** |

### Arduino Pro Mini — power

| # | Solder this | To this |
|---|---|---|
| 46 | Pro Mini **VCC** | **RAIL-BAT+** |
| 47 | Pro Mini **GND** | **RAIL-SW** |
| 48 | 100 nF leg 1 | Pro Mini **VCC** pin |
| 49 | 100 nF leg 2 | Pro Mini **GND** pin |
| — | Pro Mini **RAW** | leave empty |

### Two more capacitors on the main rails

| # | Solder this | To this |
|---|---|---|
| 50 | 100 nF leg 1 | **RAIL-BAT+** |
| 51 | 100 nF leg 2 | **RAIL-BAT−** |
| 52 | 100 nF leg 1 | **RAIL-BAT+** |
| 53 | 100 nF leg 2 | **RAIL-SW** |

### Ra-02 radio

J1 is the row with 3V3 on it. J2 is the row with NSS, MOSI, MISO, SCK.

| # | Solder this | To this |
|---|---|---|
| 54 | J1 pin 1 (GND) | **RAIL-SW** |
| 55 | J1 pin 2 (GND) | **RAIL-SW** |
| 56 | J2 pin 1 (GND) | **RAIL-SW** |
| 57 | J2 pin 8 (GND) | **RAIL-SW** |
| 58 | J1 pin 3 (3V3) | **RAIL-BAT+** |
| 59 | J1 pin 4 (RST) | Pro Mini **D9** |
| 60 | J1 pin 5 (DIO0) | Pro Mini **D2** |
| 61 | J2 pin 2 (NSS) | Pro Mini **D10** |
| 62 | J2 pin 3 (MOSI) | Pro Mini **D11** |
| 63 | J2 pin 4 (MISO) | Pro Mini **D12** |
| 64 | J2 pin 5 (SCK) | Pro Mini **D13** |
| — | J1 pins 6, 7, 8 and J2 pins 6, 7 | leave empty |

All four ground pins get their own wire. Not one shared.

Three capacitors, legs as short as you can make them, right at the radio:

| # | Solder this | To this |
|---|---|---|
| 65 | 100 nF leg 1 | J1 pin 3 |
| 66 | 100 nF leg 2 | J2 pin 8 |
| 67 | 10 µF leg 1 | J1 pin 3 |
| 68 | 10 µF leg 2 | J2 pin 8 |
| 69 | 100 µF leg 1 | J1 pin 3 |
| 70 | 100 µF leg 2 | J2 pin 8 |

### Ultrasonic connector

Module pads, left to right: **GND, RX, TX, +5V**. RX is the trigger. TX is the echo.

| # | Solder this | To this |
|---|---|---|
| 71 | 100 Ω leg 1 | Pro Mini **D6** |
| 72 | that 100 Ω leg 2 | connector pin going to module **RX** |
| 73 | 100 Ω leg 1 | Pro Mini **D8** |
| 74 | that 100 Ω leg 2 | connector pin going to module **TX** |
| 75 | connector pin going to module **+5V** | **RAIL-BAT+** |
| 76 | connector pin going to module **GND** | **RAIL-SW** |
| 77 | 100 nF leg 1 | the **+5V** connector pin |
| 78 | 100 nF leg 2 | the **GND** connector pin |

Echo goes to **D8**. Not D7.

### Temperature connector — two DS18B20 in parallel

| # | Solder this | To this |
|---|---|---|
| 79 | Pro Mini **D3** | connector **VCC** pin |
| 80 | connector **GND** pin | **RAIL-SW** |
| 81 | 100 Ω leg 1 | Pro Mini **D4** |
| 82 | that 100 Ω leg 2 | **J-TEMP** |
| 83 | connector **DATA** pin | **J-TEMP** |
| 84 | 4.7 kΩ leg 1 | **J-TEMP** |
| 85 | 4.7 kΩ leg 2 | connector **VCC** pin |

Both sensors: red → VCC pin, black → GND pin, yellow → DATA pin. Same three pins for both.
Check each probe's colours with a meter first.

### Flow connector

| # | Solder this | To this |
|---|---|---|
| 86 | connector pin 1 | **J-FLOW** |
| 87 | connector pin 2 | **RAIL-SW** |
| 88 | 100 Ω leg 1 | Pro Mini **D5** |
| 89 | that 100 Ω leg 2 | **J-FLOW** |
| 90 | 1 MΩ leg 1 | **J-FLOW** |
| 91 | 1 MΩ leg 2 | **RAIL-BAT+** |
| 92 | 100 nF leg 1 | **J-FLOW** |
| 93 | 100 nF leg 2 | **RAIL-SW** |
| 94 | 330 Ω leg 1 | Pro Mini **A2** |
| 95 | that 330 Ω leg 2 | **J-FLOW** |

Flow switch has no polarity — either wire to either connector pin.
Install it with the yellow arrow pointing the way the water flows.

### LED

| # | Solder this | To this |
|---|---|---|
| 96 | 1 kΩ leg 1 | Pro Mini **D7** |
| 97 | that 1 kΩ leg 2 | LED **long leg** |
| 98 | LED **short leg** | **RAIL-SW** |

---

## 7. PINS LEFT EMPTY

Pro Mini: A3, A4, A5, A6, A7, D0, D1, RST, DTR, RAW
74HC74: pins 8, 9
Ra-02: J1 pins 6, 7, 8 and J2 pins 6, 7

---

## 8. SOLDER ORDER

1. Ground mesh, if you are doing one — before any component
2. The 4 rails
3. Battery pack (wires 1–7), test it alone with a meter
4. Wires 8–45
5. **Stop. Run section 9.**
6. Wires 46–98

---

## 9. CHECKS — meter, power disconnected

| # | Put probes on | Must read |
|---|---|---|
| 1 | RAIL-BAT− and RAIL-SW | **no beep** |
| 2 | RAIL-BAT+ and RAIL-BAT− | no beep |
| 3 | RAIL-BAT+ and RAIL-SW | no beep |
| 4 | 74HC74 pin 14 and RAIL-LATCH | beep |
| 5 | 74HC74 pin 7 and RAIL-BAT− | beep |
| 6 | 74HC74 pin 4 and RAIL-LATCH | beep |
| 7 | 74HC74 pin 10 and RAIL-LATCH | beep |
| 8 | 74HC74 pin 13 and RAIL-LATCH | beep |
| 9 | 74HC74 pin 11 and RAIL-BAT− | beep |
| 10 | 74HC74 pin 12 and RAIL-BAT− | beep |
| 11 | J-REED and RAIL-BAT− | 470 kΩ on resistance range |
| 12 | Ra-02 J1 pin 1 and RAIL-SW | beep |
| 13 | Ra-02 J1 pin 2 and RAIL-SW | beep |
| 14 | Ra-02 J2 pin 1 and RAIL-SW | beep |
| 15 | Ra-02 J2 pin 8 and RAIL-SW | beep |
| 16 | Pro Mini RAW and anything | no beep |
| 17 | Battery pack output | + on the pin you expect |

## 10. FIRST POWER-UP

Bench supply 3.6 V, current limit 50 mA.

| Step | Should happen |
|---|---|
| 1. Connect power | Device OFF. RAIL-SW reads about 3.6 V |
| 2. Magnet near reed | Device ON. RAIL-SW drops to about 0 V |
| 3. Take magnet away | Stays ON |
| 4. Magnet near again | OFF |
| 5. Repeat 10 times | Exactly one change per approach |

Step 5 failing: check the 470 kΩ and the 100 Ω at the reed are actually fitted.

## 11. MEASURE BEFORE SEALING

| Measure | Should be |
|---|---|
| Sleep current | 25 µA or less |
| Current with magnet resting on the reed | about 7.7 µA |
| Voltage at 74HC74 pin 14 during a transmission, cells idle a week | above 2.0 V |
| Ultrasonic blind zone — flat target, 2 cm outward in 5 mm steps | write it down |
| Distance between the two transducer centres, callipers | write it down |
