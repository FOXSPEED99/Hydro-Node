# HYDRO NODE — BUILD SHEET

Work top to bottom. Every step connects only to something already on the board. Nothing is left loose to come back to.

---

## PARTS

### Remove — do not fit
CD4013BE · C3 2200 µF electrolytic · blue LED · the 220 Ω / 330 Ω LED resistor · the 10 kΩ

### Reuse
Pro Mini 3.3 V (LED + regulator already removed) · Ra-02 · reed switch · IRLZ44N · 1 MΩ · 1 kΩ · 4.7 kΩ · 100 kΩ · 4 × 100 nF · JST connectors · RCWL-1670 · WY-90 · DS18B20 · 2 × LS14500

### Buy
3 × 1N5819 · 0.5 A fuse + holder · 1 × DS18B20 · 1 red LED · 1 × 470 kΩ · 2 × 1 MΩ · 1 × 1 kΩ · 6 × 100 Ω · 1 × 330 Ω · 5 × 100 nF · 2 × 10 µF · 1 × 100 µF

---

# STAGE 1 — BATTERY PACK

Build this off the board. Nothing else exists yet.

**1.1** — Take a **1N5819**. One end has a **stripe**.
- plain end → **Cell 1 positive**
- striped end → **fuse leg 1**

**1.2** — Take the second **1N5819**.
- plain end → **Cell 2 positive**
- striped end → **fuse leg 1** — the same blob as 1.1

**1.3** — Join **Cell 1 negative** and **Cell 2 negative** together. Solder a **black wire** to that blob.

**1.4** — Solder a **red wire** to **fuse leg 2**.

**TEST NOW:** meter across red and black. Should read about **3.6 V**, red positive. If it reads 0, a diode is backwards.

Put the pack aside.

---

# STAGE 2 — RAILS AND HOLES

**2.1** — Run **4 bare wires** across the board. Label each with tape.

| Rail | Where it goes |
|---|---|
| **RAIL-BAT+** | one long edge |
| **RAIL-BAT−** | next to it |
| **RAIL-SW** | the opposite edge |
| **RAIL-LATCH** | short, near where the 74HC74 will sit |

**RAIL-BAT− and RAIL-SW must never touch.**

**2.2** — Pick **5 empty holes** and mark them with a pen: **H1, H2, H3, H4, H5**.

Each will end up with this many legs. Count them at the end:

| Hole | Final leg count | Put it near |
|---|---|---|
| H1 | 3 | the MOSFET |
| H2 | 4 | the 74HC74 |
| H3 | 5 | the reed switch |
| H4 | 3 | the temperature connector |
| H5 | 5 | the flow connector |

---

# STAGE 3 — 74HC74 CHIP

**3.1** — Place the chip. It has a **notch at one end**. With the notch at the top, **pin 1 is top-left**, and pins count **down the left side** (1–7), then **up the right side** (8–14).

Now every pin, one at a time:

| Step | Pin | Solder it to |
|---|---|---|
| 3.2 | pin 14 | **RAIL-LATCH** |
| 3.3 | pin 7 | **RAIL-BAT−** |
| 3.4 | pin 4 | **RAIL-LATCH** |
| 3.5 | pin 10 | **RAIL-LATCH** |
| 3.6 | pin 13 | **RAIL-LATCH** |
| 3.7 | pin 11 | **RAIL-BAT−** |
| 3.8 | pin 12 | **RAIL-BAT−** |
| 3.9 | pin 2 | **pin 6** — a short wire across the chip |
| 3.10 | pin 3 | **H3** |
| 3.11 | pin 1 | **H2** |
| 3.12 | pins 8 and 9 | leave empty |
| 3.13 | pin 5 | leave for now — stage 5 brings a wire to it |

Pins **4, 10, 13** go to **RAIL-LATCH**. Not to ground. Check this before moving on.

> **H2 now has 1 leg** (pin 1). **H3 now has 1 leg** (pin 3).

---

# STAGE 4 — POWER FOR THE LATCH

**4.1** — Third **1N5819**.
- plain end → **RAIL-BAT+**
- striped end → **RAIL-LATCH**

**4.2** — **10 µF ceramic**. No polarity.
- one leg → **RAIL-LATCH**
- other leg → **RAIL-BAT−**

**4.3** — **100 nF**.
- one leg → **RAIL-LATCH**
- other leg → **RAIL-BAT−**

---

# STAGE 5 — MOSFET

**5.1** — **IRLZ44N**. Hold it with the **printed face toward you** and the **legs pointing down**.
- **left leg** = gate
- **middle leg** = drain — also connected to the metal tab
- **right leg** = source

**5.2** — **middle leg** → **RAIL-SW**

**5.3** — **right leg** → **RAIL-BAT−**

**5.4** — **left leg** → **H1**

**5.5** — **1 MΩ**.
- one leg → **H1**
- other leg → **RAIL-BAT−**

**5.6** — **1 kΩ**.
- one leg → **H1**
- other leg → **74HC74 pin 5**

> **H1 is now finished — 3 legs.** Count them: MOSFET left leg, 1 MΩ, 1 kΩ.

---

# STAGE 6 — RESET PARTS

**6.1** — **1 MΩ**.
- one leg → **H2**
- other leg → **RAIL-LATCH**

**6.2** — **100 nF**.
- one leg → **H2**
- other leg → **RAIL-BAT−**

> **H2 now has 3 legs.** One more arrives in stage 14.

---

# STAGE 7 — REED SWITCH

**7.1** — reed **leg 1** → **RAIL-LATCH**

**7.2** — **100 Ω**.
- one leg → reed **leg 2**
- other leg → **H3**

**7.3** — **470 kΩ**.
- one leg → **H3**
- other leg → **RAIL-BAT−**

**7.4** — **100 nF**.
- one leg → **H3**
- other leg → **RAIL-BAT−**

> **H3 now has 4 legs.** One more arrives in stage 14.

---

# STAGE 8 — TEST 1: does the power switch work?

Nothing else is on the board yet. This tests the latch on its own.

**Meter first, no power:**

| Probes on | Must read |
|---|---|
| RAIL-BAT− and RAIL-SW | **no beep** |
| RAIL-BAT+ and RAIL-BAT− | no beep |
| 74HC74 pin 4 and RAIL-LATCH | beep |
| 74HC74 pin 10 and RAIL-LATCH | beep |
| 74HC74 pin 13 and RAIL-LATCH | beep |
| H3 and RAIL-BAT− | 470 kΩ |

**Then power.** Bench supply 3.6 V, limit 50 mA — or the battery pack from stage 1.
Red to **RAIL-BAT+**, black to **RAIL-BAT−**.

| Do this | Should happen |
|---|---|
| Connect power | **RAIL-SW reads ≈ 3.6 V** (device off) |
| Magnet near the reed | **RAIL-SW drops to ≈ 0 V** (device on) |
| Take the magnet away | stays at 0 V |
| Magnet near again | back to ≈ 3.6 V |
| Repeat 10 times | one change per approach, every time |

**Do not carry on until this passes.** Everything after this depends on it.

Disconnect power.

---

# STAGE 9 — TWO RAIL CAPACITORS

**9.1** — **100 nF**: one leg → **RAIL-BAT+**, other leg → **RAIL-BAT−**

**9.2** — **100 nF**: one leg → **RAIL-BAT+**, other leg → **RAIL-SW**

---

# STAGE 10 — ULTRASONIC CONNECTOR (4-pin)

The RCWL-1670 module's pads read left to right: **GND, RX, TX, +5V**.
Decide now which connector pin carries which, and write it on tape. Below they are called by function.

**10.1** — Place the 4-pin connector.

**10.2** — the **+5V** pin → **RAIL-BAT+**

**10.3** — the **GND** pin → **RAIL-SW**

**10.4** — **100 nF**: one leg → the **+5V** pin, other leg → the **GND** pin

> The **RX** and **TX** pins stay empty. Stage 14 brings wires to them.

---

# STAGE 11 — TEMPERATURE CONNECTOR (3-pin)

Three pins: **VCC**, **GND**, **DATA**. Write them on tape.

**11.1** — Place the 3-pin connector.

**11.2** — the **GND** pin → **RAIL-SW**

**11.3** — the **DATA** pin → **H4**

**11.4** — **4.7 kΩ**.
- one leg → **H4**
- other leg → the **VCC** pin

> **H4 now has 2 legs.** One more in stage 14. The **VCC** pin stays otherwise empty until stage 14.

---

# STAGE 12 — FLOW CONNECTOR (2-pin)

**12.1** — Place the 2-pin connector. Call the pins **A** and **B**.

**12.2** — pin **B** → **RAIL-SW**

**12.3** — pin **A** → **H5**

**12.4** — **1 MΩ**.
- one leg → **H5**
- other leg → **RAIL-BAT+**

**12.5** — **100 nF**.
- one leg → **H5**
- other leg → **RAIL-SW**

> **H5 now has 3 legs.** Two more in stage 14.

---

# STAGE 13 — RADIO HEADERS

**13.1** — Place the two 8-pin sockets for the Ra-02.
**J1** is the row with **3V3** on it. **J2** is the row with **NSS, MOSI, MISO, SCK**.

**13.2** — J1 **pin 1** (GND) → **RAIL-SW**

**13.3** — J1 **pin 2** (GND) → **RAIL-SW**

**13.4** — J2 **pin 1** (GND) → **RAIL-SW**

**13.5** — J2 **pin 8** (GND) → **RAIL-SW**

Four separate wires. Not one shared.

**13.6** — J1 **pin 3** (3V3) → **RAIL-BAT+**

Now three capacitors, legs cut **as short as you can**, sitting right against the socket:

**13.7** — **100 nF**: one leg → J1 pin 3, other leg → J2 pin 8

**13.8** — **10 µF**: one leg → J1 pin 3, other leg → J2 pin 8

**13.9** — **100 µF**: one leg → J1 pin 3, other leg → J2 pin 8

**13.10** — J1 pins **6, 7, 8** and J2 pins **6, 7** stay empty, permanently.

> J1 pins 4 and 5, and J2 pins 2, 3, 4, 5 stay empty until stage 14.

---

# STAGE 14 — PRO MINI

Everything this stage connects to is already on the board.

**14.1** — Place the Pro Mini.

### Power

**14.2** — **VCC** → **RAIL-BAT+**

**14.3** — **GND** → **RAIL-SW**

**14.4** — **100 nF**: one leg → the **VCC** pin, other leg → the **GND** pin

**14.5** — **RAW** stays empty, permanently.

### To the holes

**14.6** — **100 Ω**: one leg → **A0**, other leg → **H3**
> **H3 is now finished — 5 legs.**

**14.7** — **100 kΩ**: one leg → **A1**, other leg → **H2**
> **H2 is now finished — 4 legs.**

**14.8** — **330 Ω**: one leg → **A2**, other leg → **H5**

**14.9** — **100 Ω**: one leg → **D4**, other leg → **H4**
> **H4 is now finished — 3 legs.**

**14.10** — **100 Ω**: one leg → **D5**, other leg → **H5**
> **H5 is now finished — 5 legs.**

### To the ultrasonic connector

**14.11** — **100 Ω**: one leg → **D6**, other leg → the **RX** pin

**14.12** — **100 Ω**: one leg → **D8**, other leg → the **TX** pin

Echo is on **D8**, not D7.

### To the temperature connector

**14.13** — **D3** → the **VCC** pin of the temperature connector

### To the radio

**14.14** — **D2** → J1 **pin 5**

**14.15** — **D9** → J1 **pin 4**

**14.16** — **D10** → J2 **pin 2**

**14.17** — **D11** → J2 **pin 3**

**14.18** — **D12** → J2 **pin 4**

**14.19** — **D13** → J2 **pin 5**

### LED

**14.20** — **1 kΩ**: one leg → **D7**, other leg → the LED's **long leg**

**14.21** — LED's **short leg** → **RAIL-SW**

### Empty

**14.22** — A3, A4, A5, A6, A7, D0, D1, RST, DTR stay empty.

---

# STAGE 15 — SENSOR CABLES

**15.1 — Ultrasonic.** Cable from the connector to the module:
- **GND** pin → module **GND** pad
- **RX** pin → module **RX** pad
- **TX** pin → module **TX** pad
- **+5V** pin → module **+5V** pad

**15.2 — Temperature, both sensors in parallel** on the same 3 pins:
- both **red** wires → **VCC** pin
- both **black** wires → **GND** pin
- both **yellow** wires → **DATA** pin

Check each probe's colours with a meter before soldering.
Sensor 1 goes at the transducer. Sensor 2 on a longer lead, low in the tank headspace.

**15.3 — Flow switch.** No polarity, either wire to either pin.
Install it in the pipe with the **yellow arrow pointing the way the water flows**.

---

# STAGE 16 — FINAL CHECKS

**Meter, power disconnected:**

| Probes on | Must read |
|---|---|
| RAIL-BAT− and RAIL-SW | **no beep** |
| RAIL-BAT+ and RAIL-BAT− | no beep |
| RAIL-BAT+ and RAIL-SW | no beep |
| Ra-02 J1 pin 1 and RAIL-SW | beep |
| Ra-02 J1 pin 2 and RAIL-SW | beep |
| Ra-02 J2 pin 1 and RAIL-SW | beep |
| Ra-02 J2 pin 8 and RAIL-SW | beep |
| Pro Mini RAW and anything | no beep |

**Hole leg count** — count the legs physically in each:

| Hole | Must have |
|---|---|
| H1 | 3 |
| H2 | 4 |
| H3 | 5 |
| H4 | 3 |
| H5 | 5 |

**Then repeat the stage 8 power test** with everything fitted. Ten magnet approaches, ten state changes.

---

# STAGE 17 — MEASURE BEFORE SEALING

| Measure | Should be |
|---|---|
| Sleep current | 25 µA or less |
| Current with a magnet resting on the reed | about 7.7 µA |
| Voltage at 74HC74 pin 14 during a transmission, cells idle a week | above 2.0 V |
| Ultrasonic blind zone — flat target, 2 cm outward in 5 mm steps | write it down |
| Distance between the two transducer centres, callipers | write it down |
