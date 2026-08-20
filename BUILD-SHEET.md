# HYDRO NODE — BUILD SHEET

Work top to bottom. Every step connects to something already on the board.

---

## PARTS

**Do not fit:** CD4013BE · the 2200 µF electrolytic · the blue LED · the 220 Ω / 330 Ω LED resistor · the 10 kΩ

**Reuse:** Pro Mini 3.3 V (LED + regulator already removed) · Ra-02 · reed switch · IRLZ44N · 1 MΩ · 1 kΩ · 4.7 kΩ · 100 kΩ · 4 × 100 nF · JST connectors · RCWL-1670 · WY-90 · DS18B20 · 2 × LS14500

**Buy:** 3 × 1N5819 · 0.5 A fuse + holder · 1 × DS18B20 · 1 red LED · 1 × 470 kΩ · 2 × 1 MΩ · 1 × 1 kΩ · 6 × 100 Ω · 1 × 330 Ω · 5 × 100 nF · 2 × 10 µF · 1 × 100 µF

**Also get:** wire in **4 colours** — red, black, blue, green.

### The capacitors — not all the same type

| Value | Type | Polarity |
|---|---|---|
| **100 nF** × 9 | **Ceramic.** Small blue or yellow disc, printed **104** | None. Either leg either way |
| **10 µF** × 2 | Ceramic, aluminium electrolytic **or tantalum** — any is fine | **Yes**, unless it is ceramic |
| **100 µF** × 1 | Ceramic, aluminium electrolytic **or tantalum** — any is fine | **Yes**, unless it is ceramic |

**Reading the code on a ceramic disc:** the first 2 digits, then that many zeros, in picofarads.
**104** = 10 + 4 zeros = 100000 pF = **100 nF** ← the one you already have
**106** = 10 + 6 zeros = **10 µF**
**107** = **100 µF**

### Three kinds of capacitor, and the stripe means different things

| Looks like | Type | The stripe / bar marks |
|---|---|---|
| Small blue or yellow disc, two straight legs | **Ceramic** | no polarity at all |
| Metal can, stripe down one side with **−** signs | **Aluminium electrolytic** | **MINUS** |
| Yellow or orange block, bar across one end | **Tantalum** | **PLUS** |

**The stripe means the opposite thing on tantalum.** Same marking, reversed meaning. Read the type first, then the stripe.

**Other ways to tell plus from minus:**
- On a **can**, the **longer** leg is plus (only on a new, uncut part).
- On a **tantalum**, the bar end is plus. There is no other marking.

Backwards fails in every case. A **tantalum fails short and can burn** — it is the one part here where getting it wrong does more than just not work.

**Surface-mount parts** (a flat block with no legs, like most tantalums) need short wires soldered to each end before they can go on perfboard. If you have a through-hole part of the same value, use that instead.

---

## THE 4 WIRES

You will run 4 wires across the board first. Everything else solders onto one of them.

| Wire | What it is |
|---|---|
| **RED** | battery plus. Powers the Pro Mini, the radio, the ultrasonic |
| **BLACK** | battery minus |
| **BLUE** | the ground that switches off. When the device is off, BLUE is disconnected from BLACK |
| **GREEN** | a small power wire. Only the 74HC74 and the reed use it |

**RED, BLACK, BLUE, GREEN.** That is all the naming in this document. Everything else is a component leg.

> **BLACK and BLUE must never touch each other.** That is what makes the on/off switch work.

---

# STAGE 1 — BATTERY PACK

Off the board. Nothing else exists yet.

**1.1** — Take a **1N5819** diode. One end has a **stripe** painted on it.
- the **plain end** → solder to **Cell 1 plus**
- the **striped end** → solder to **one leg of the fuse**

**1.2** — Take the **second 1N5819**.
- the **plain end** → solder to **Cell 2 plus**
- the **striped end** → solder to **that same fuse leg** — the same solder blob as 1.1

**1.3** — Twist **Cell 1 minus** and **Cell 2 minus** together and solder. Solder a **BLACK wire** onto that joint.

**1.4** — Solder a **RED wire** onto **the fuse's other leg**.

**TEST:** meter across the red and black wires. About **3.6 V**, red is plus.
Reads 0 V? A diode is in backwards.

Put the pack aside.

---

# STAGE 2 — RUN THE 4 WIRES

Cut 4 lengths of wire and solder each one across the board, along a row of holes. Solder it into every 4th or 5th hole so it stays put.

**2.1** — **RED wire** along one long edge.

**2.2** — **BLACK wire** next to it.

**2.3** — **BLUE wire** along the opposite edge.

**2.4** — **GREEN wire**, short, wherever you plan to put the 74HC74 chip.

Leave one end of the RED and BLACK wires free — the battery pack plugs in there later.

Check with a meter: **no beep** between any two of the four wires.

---

# STAGE 3 — 74HC74 CHIP

**3.1** — Place the chip on the board and solder its legs down.

The chip has a **notch** at one end. Turn it so the notch is at the top.
**Pin 1 is the top-left leg.** Count **down the left side**: 1, 2, 3, 4, 5, 6, 7.
Then jump to the **bottom-right leg**, which is pin 8, and count **up the right side**: 8, 9, 10, 11, 12, 13, 14.
So **pin 14 is top-right**.

Now, one leg at a time:

**3.2** — pin 14 → **GREEN**

**3.3** — pin 7 → **BLACK**

**3.4** — pin 4 → **GREEN**

**3.5** — pin 10 → **GREEN**

**3.6** — pin 13 → **GREEN**

**3.7** — pin 11 → **BLACK**

**3.8** — pin 12 → **BLACK**

**3.9** — pin 2 → **pin 6**. A short wire from one side of the chip to the other.

**3.10** — pins 8 and 9 → nothing. Leave them.

Pins **4, 10, 13** go to **GREEN**. Not to BLACK. Check this now.

Pins **1, 3, 5** stay empty for now. Other parts will be soldered onto them in later stages.

---

# STAGE 4 — POWER FOR THE CHIP

**4.1** — The **third 1N5819** diode.
- **plain end** → **RED**
- **striped end** → **GREEN**

**4.2** — A **10 µF** capacitor. **Plus goes to GREEN, minus goes to BLACK.**

| If it is | Plus is |
|---|---|
| Ceramic (small blob, no stripe) | no polarity — either way round |
| Aluminium electrolytic (metal can) | the leg **away** from the stripe |
| Tantalum (yellow block) | the **bar** end |

**4.3** — A **100 nF** capacitor.
- one leg → **GREEN**
- other leg → **BLACK**

---

# STAGE 5 — MOSFET

**5.1** — Take the **IRLZ44N**. Hold it with the **printed face toward you** and the **legs pointing down**.

- **left leg** = gate
- **middle leg** = drain. The metal tab at the back is also the drain
- **right leg** = source

Solder it to the board.

**5.2** — **middle leg** → **BLUE**

**5.3** — **right leg** → **BLACK**

**5.4** — A **1 MΩ** resistor.
- one leg → **the MOSFET's left leg**
- other leg → **BLACK**

**5.5** — A **1 kΩ** resistor.
- one leg → **the MOSFET's left leg**
- other leg → **74HC74 pin 5**

> The MOSFET's **left leg** now has 3 things on it: the resistor from 5.4, the resistor from 5.5, and the MOSFET leg itself.

---

# STAGE 6 — TWO PARTS ONTO PIN 1

**6.1** — A **1 MΩ** resistor.
- one leg → **74HC74 pin 1**
- other leg → **GREEN**

**6.2** — A **100 nF** capacitor.
- one leg → **74HC74 pin 1**
- other leg → **BLACK**

> **Pin 1** now has 2 things on it. One more comes in stage 14.

---

# STAGE 7 — REED SWITCH

**7.1** — reed **leg 1** → **GREEN**

**7.2** — A **100 Ω** resistor.
- one leg → **reed leg 2**
- other leg → **74HC74 pin 3**

**7.3** — A **470 kΩ** resistor.
- one leg → **74HC74 pin 3**
- other leg → **BLACK**

**7.4** — A **100 nF** capacitor.
- one leg → **74HC74 pin 3**
- other leg → **BLACK**

> **Pin 3** now has 3 things on it. One more comes in stage 14.

---

# STAGE 8 — TEST: does the on/off switch work?

Only the battery, the chip, the MOSFET and the reed are on the board. Test them before adding anything else.

**Meter, no power connected:**

| Touch probes to | Should |
|---|---|
| BLACK and BLUE | **not beep** |
| RED and BLACK | not beep |
| 74HC74 pin 4 and GREEN | beep |
| 74HC74 pin 10 and GREEN | beep |
| 74HC74 pin 13 and GREEN | beep |

**Now connect power.** Battery pack from stage 1, or a bench supply at 3.6 V with the limit set to 50 mA.
Red wire to **RED**, black wire to **BLACK**.

Put the meter's black probe on **BLACK** and the red probe on **BLUE**. Watch that reading.

| Do this | Meter should show |
|---|---|
| Connect the power | **about 3.6 V** — device off |
| Bring the magnet to the reed | **about 0 V** — device on |
| Take the magnet away | stays at 0 V |
| Bring the magnet again | back to about 3.6 V |
| Repeat 10 times | changes once every time |

**Do not go further until this works.** Everything after depends on it.

Disconnect the power.

---

# STAGE 9 — TWO CAPACITORS

**9.1** — **100 nF**: one leg → **RED**, other leg → **BLACK**

**9.2** — **100 nF**: one leg → **RED**, other leg → **BLUE**

---

# STAGE 10 — ULTRASONIC CONNECTOR

The RCWL-1670 module has 4 pads. Left to right they are printed: **GND, RX, TX, +5V**.

**10.1** — Solder the 4-pin connector to the board. Put a piece of tape next to it and write **GND · RX · TX · +5V** so you know which pin is which.

**10.2** — the **+5V** pin → **RED**

**10.3** — the **GND** pin → **BLUE**

**10.4** — **100 nF**: one leg → the **+5V** pin, other leg → the **GND** pin

> The **RX** and **TX** pins stay empty. Stage 14 fills them.

---

# STAGE 11 — TEMPERATURE CONNECTOR

**11.1** — Solder the 3-pin connector. Write on tape: **VCC · GND · DATA**.

**11.2** — the **GND** pin → **BLUE**

**11.3** — A **4.7 kΩ** resistor.
- one leg → the **DATA** pin
- other leg → the **VCC** pin

> The **DATA** pin has 1 thing on it. One more comes in stage 14.
> The **VCC** pin has 1 thing on it. One more comes in stage 14.

---

# STAGE 12 — FLOW CONNECTOR

**12.1** — Solder the 2-pin connector. Write on tape: **A · B**.

**12.2** — pin **B** → **BLUE**

**12.3** — A **1 MΩ** resistor.
- one leg → pin **A**
- other leg → **RED**

**12.4** — A **100 nF** capacitor.
- one leg → pin **A**
- other leg → **BLUE**

> Pin **A** now has 2 things on it. Two more come in stage 14.

---

# STAGE 13 — RADIO SOCKETS

**13.1** — Solder the two 8-pin sockets that the Ra-02 plugs into.

**J1** is the row printed with **3V3**.
**J2** is the row printed with **NSS, MOSI, MISO, SCK**.

**13.2** — J1 **pin 1** (GND) → **BLUE**

**13.3** — J1 **pin 2** (GND) → **BLUE**

**13.4** — J2 **pin 1** (GND) → **BLUE**

**13.5** — J2 **pin 8** (GND) → **BLUE**

Four separate wires, one per pin. Do not join them together first.

**13.6** — J1 **pin 3** (3V3) → **RED**

Now 3 capacitors. Cut their legs **as short as possible** so they sit right against the socket.

**13.7** — **100 nF**: one leg → J1 pin 3, other leg → J2 pin 8

**13.8** — **10 µF**. **Plus → J1 pin 3. Minus → J2 pin 8.**

**13.9** — **100 µF**. **Plus → J1 pin 3. Minus → J2 pin 8.**

For both: ceramic has no polarity. On an **aluminium can** the stripe is **minus**. On a **tantalum block** the bar is **plus**. If it is surface-mount with no legs, solder a short wire to each end first.

**13.10** — J1 pins 6, 7, 8 and J2 pins 6, 7 → nothing, ever.

> J1 pins 4 and 5, and J2 pins 2, 3, 4, 5 stay empty until stage 14.

---

# STAGE 14 — PRO MINI

Everything in this stage connects to something already fitted.

**14.1** — Solder the Pro Mini onto the board.

### Power

**14.2** — **VCC** → **RED**

**14.3** — **GND** → **BLUE**

**14.4** — **100 nF**: one leg → the **VCC** pin, other leg → the **GND** pin

**14.5** — **RAW** → nothing, ever.

### Onto the 74HC74

**14.6** — **100 Ω**: one leg → **A0**, other leg → **74HC74 pin 3**
> Pin 3 is now finished — 4 things on it.

**14.7** — **100 kΩ**: one leg → **A1**, other leg → **74HC74 pin 1**
> Pin 1 is now finished — 3 things on it.

### Onto the flow connector

**14.8** — **330 Ω**: one leg → **A2**, other leg → flow connector pin **A**

**14.9** — **100 Ω**: one leg → **D5**, other leg → flow connector pin **A**
> Flow pin A is now finished — 4 things on it.

### Onto the temperature connector

**14.10** — **D3** → the temperature connector's **VCC** pin
> VCC pin finished — 2 things on it.

**14.11** — **100 Ω**: one leg → **D4**, other leg → the **DATA** pin
> DATA pin finished — 2 things on it.

### Onto the ultrasonic connector

**14.12** — **100 Ω**: one leg → **D6**, other leg → the **RX** pin

**14.13** — **100 Ω**: one leg → **D8**, other leg → the **TX** pin

The echo goes to **D8**. Not D7.

### Onto the radio sockets

**14.14** — **D2** → J1 **pin 5**

**14.15** — **D9** → J1 **pin 4**

**14.16** — **D10** → J2 **pin 2**

**14.17** — **D11** → J2 **pin 3**

**14.18** — **D12** → J2 **pin 4**

**14.19** — **D13** → J2 **pin 5**

### LED

**14.20** — **1 kΩ**: one leg → **D7**, other leg → the LED's **long leg**

**14.21** — the LED's **short leg** → **BLUE**

### Leave empty

**14.22** — A3, A4, A5, A6, A7, D0, D1, RST, DTR → nothing.

---

# STAGE 15 — SENSOR CABLES

**15.1 — Ultrasonic.** Run a 4-wire cable from the connector to the module:
GND → GND pad · RX → RX pad · TX → TX pad · +5V → +5V pad

**15.2 — Temperature.** Both DS18B20 probes go on the **same 3 pins**:
- both **red** wires → **VCC** pin
- both **black** wires → **GND** pin
- both **yellow** wires → **DATA** pin

Check each probe's wire colours with a meter before soldering.
One probe sits at the ultrasonic transducer. The other on a longer lead, hanging lower in the tank.

**15.3 — Flow switch.** No plus or minus — either wire to either pin.
Fit it in the pipe with the **yellow arrow pointing the way the water flows**.

---

# STAGE 16 — FINAL CHECKS

**Meter, power disconnected:**

| Touch probes to | Should |
|---|---|
| BLACK and BLUE | **not beep** |
| RED and BLACK | not beep |
| RED and BLUE | not beep |
| Ra-02 J1 pin 1 and BLUE | beep |
| Ra-02 J1 pin 2 and BLUE | beep |
| Ra-02 J2 pin 1 and BLUE | beep |
| Ra-02 J2 pin 8 and BLUE | beep |
| Pro Mini RAW and anything | not beep |

**Count the legs** on these five points:

| Point | Should have |
|---|---|
| MOSFET left leg | 3 |
| 74HC74 pin 1 | 3 |
| 74HC74 pin 3 | 4 |
| Temperature connector DATA pin | 2 |
| Flow connector pin A | 4 |

**Then repeat the stage 8 magnet test** with everything fitted. Ten approaches, ten changes.

---

# STAGE 17 — MEASURE BEFORE SEALING

| Measure | Should be |
|---|---|
| Sleep current | 25 µA or less |
| Current with a magnet sitting on the reed | about 7.7 µA |
| Voltage on 74HC74 pin 14 while it transmits, cells left idle a week | above 2.0 V |
| Ultrasonic blind zone — flat target, from 2 cm outward in 5 mm steps | write it down |
| Distance between the two transducer centres, with callipers | write it down |
