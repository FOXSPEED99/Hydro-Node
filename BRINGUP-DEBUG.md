# THE BOARD DOESN'T SWITCH ON — FINDING IT

## ✅ FOUND IT — 2026-08-24, from your six readings

| Measurement | You got | Should be | |
|---|---|---|---|
| U2 pin 14 | 3.5 V | 3.3–3.5 V | ✅ latch rail is fine — D1 and C9 are correct |
| U2 pin 1 | 3.2 V | 3.3–3.5 V | ✅ reset released |
| U2 pin 4 | 3.5 V | 3.3–3.5 V | ✅ set released |
| **U2 pin 3, no magnet** | **2.4 V** | **0 V** | ❌ **this is the fault** |
| U2 pin 3, magnet | 3.6 V | 3.5 V | ✅ the reed works |
| U2 pin 5 | 0 V always | should flip | consequence, not a cause |
| Q1 gate | 0 V always | follows pin 5 | consequence, not a cause |

**Pin 3 is the clock. It must sit at 0 V and jump up when the magnet arrives.** The chip counts the *jump from low to high*, not the level. Yours is already sitting at 2.4 V, which the 74HC74 reads as **high**, so when the magnet takes it to 3.6 V there is no jump — it was already high. **No edge, no toggle, Q stays at 0, the gate stays at 0, the MOSFET never turns on.**

Every one of your seven readings is explained by that one thing. The chip is fine, the MOSFET is fine, the reed is fine, D1 is fine, and your soldering is fine.

### Why pin 3 is sitting at 2.4 V

**This is a design fault, not a build fault — it is HW-067, and the board can never work as drawn.**

Pin 3 is pulled down to BATT− by **R14, 470 kΩ**. The Pro Mini's **A0** reaches the same pin through **R13, 100 Ω**.

When the device is off, the Pro Mini is in a strange state: its **VCC is on BATT+, which is always live**, while its **ground is the switched ground, which is floating**. It is half-powered. Current leaks out of its A0 pin, through R13's 100 Ω, onto the clock line — and R13 is **4,700 times stronger than R14**, so it wins completely.

Working backwards from your 2.4 V, the leakage path inside the Pro Mini looks like about **235 kΩ** to BATT+:

```
BATT+ 3.6 V ── ~235 kΩ inside the Pro Mini ── A0 ── R13 100 Ω ──┬── U2 pin 3
                                                                 │
                                                          R14 470 kΩ
                                                                 │
                                                              BATT−

3.6 V × 470 / (470 + 235) = 2.4 V     ← exactly what you measured
```

235 kΩ is an ordinary figure for a CMOS pin whose chip is powered but has no ground. The numbers line up.

---

## PROVE IT IN TWO MINUTES

**Unsolder one leg of R13 and lift it clear of the board.** That is the 100 Ω between the Pro Mini's A0 and U2 pin 3.

Then power up and measure U2 pin 3 again:

| Pin 3 now reads | Meaning |
|---|---|
| **about 0 V** | Confirmed. Try the magnet — the device should switch on and stay on. |
| **still 2.4 V** | R13 is innocent; **R14 has a bad joint or is open.** Power off and measure resistance from pin 3 to BATT− — it should read about 470 kΩ. |

With R13 lifted the board will work. The only thing you lose is the MCU's ability to see the magnet, which nothing needs yet.

---

## WHAT "TOGGLE" MEANS — YOUR QUESTION

I explained this badly. A toggle is a switch that **swaps and stays**, like the button on a desk lamp:

| You do | Pin 5 |
|---|---|
| touch the magnet, take it away | goes to 3.5 V and **stays at 3.5 V** |
| touch it again, take it away | goes to 0 V and **stays at 0 V** |
| touch it again | back to 3.5 V |

It does **not** follow the magnet. The magnet is not a button you hold — each touch flips the state and the state stays after you remove it. That is the whole reason the 74HC74 is on the board: the reed alone would only be "on while the magnet is there".

So the right way to watch pin 5 is: leave the meter on it, touch the magnet, **take the magnet away**, and see whether the reading changed and stayed changed. Yours never changed at all, which fits — it was never getting a clock edge.

---

## THE PROPER FIX

Lifting R13 works but throws away the feature. Three resistor changes fix it properly and keep everything:

| Part | Now | Change to | Why |
|---|---|---|---|
| **R13** | 100 Ω | **1 MΩ** | so the Pro Mini can no longer overpower the pull-down |
| **R14** | 470 kΩ | **100 kΩ** | so the pull-down wins with room to spare |
| **C12** | 100 nF | **470 nF** | keeps the debounce at 47 ms with the smaller R14 |

What that does:

| | Pin 3 when off | Valid low needs | Debounce |
|---|---|---|---|
| as built | **2.40 V** | under 1.05 V | 47 ms |
| R13 → 1 MΩ only | 0.99 V | under 1.05 V | 47 ms — only 60 mV of margin, too tight |
| **all three** | **0.27 V** | under 1.05 V | **47 ms** ✅ |

R14 × C12 stays at 47 ms, so the debounce from HW-014 is unchanged.

The one cost: with the magnet held on the reed, current rises from about 7.7 µA to about 35 µA, because R14 is smaller. That only flows while a magnet is actually sitting on the switch, so it does nothing to the two-year budget.

### Also check R9 and A1, for the same reason

**R9, 100 kΩ**, connects the Pro Mini's **A1** to U2 **pin 1** — the reset. R11 holds pin 1 up with 1 MΩ, so R9 is **10 times stronger**. Here the leak happens to push pin 1 *up*, which is the harmless direction, and your 3.2 V reading shows exactly that — 0.3 V below the rail because a little current is flowing out through R9.

It is harmless today and dangerous tomorrow: the moment firmware drives A1 low, or the leak changes with temperature, the same failure appears on the reset instead of the clock. **Change R9 to 1 MΩ** at the same time.

### And a firmware rule, written down now

**A0 and A1 must be inputs with their pull-ups disabled at all times**, except for the few milliseconds when A1 is deliberately commanding a shutdown, after which it goes straight back to being an input. Never leave either as an output.

---


For the hand-built PCB, 2026-08-24. Pad positions are the real ones out of `Hydro_Node_PCB.PcbDoc`.

---

## WHAT YOUR TWO OBSERVATIONS ALREADY TELL US

**The Pro Mini powered up when you bridged Drain to Source.** That is a big result. It means the battery, BATT+, the whole switched-ground net, the Pro Mini's supply and its decoupling are all fine. Everything downstream of the MOSFET works.

**So the fault is inside one small block:** D1 → the latch rail → the 74HC74 → the gate → the MOSFET. Six parts. Nothing else.

**The "3 seconds then off" is not a fault.** A multimeter in continuity mode is not a wire — it sources a small test current and has real resistance in the path. The Pro Mini and radio idle at a few milliamps, and when the firmware first keys the radio it asks for up to 120 mA. The meter cannot supply that, the voltage collapses, and the MCU browns out. Three seconds is about how long boot-plus-first-transmit takes.

> **Bridge Drain to Source with an actual piece of wire, not the meter.** Then it will stay on.

---

## RULE ZERO — BEFORE ANY MEASUREMENT

**This board has two grounds and they are deliberately not connected.** `BATT−` is the real battery minus. `GND` is the switched ground that floats when the device is off. If you measure against the wrong one you get nonsense, and everything below will look broken.

> **Put the black probe on the BATT connector's pin 1 and leave it there for every voltage measurement below.**

On the board: the 2-pin battery connector, **pin 1 is BATT−, pin 2 is BATT+**. Pin 1 is the one further from the board's bottom edge.

Power the board from the pack, or a bench supply at 3.6 V with the current limit set to 100 mA.

---

## THE SIX MEASUREMENTS, IN ORDER

Each one splits the problem in half. Stop at the first one that fails — that is your fault.

### Where to find U2's pins

U2 is the 14-pin chip near the top of the board. Its pin rows run left-to-right.

- **The end nearest R11, R14 and C9** (to its left) is the **pin 1 / pin 14** end. The notch is on that edge.
- **The end nearest R10, R8 and Q1** (to its right) is the **pin 7 / pin 8** end.
- **Pin 1 is on the bottom row, at the left end. Pin 14 is directly above it on the top row.**

| | Bottom row, left → right | Top row, left → right |
|---|---|---|
| Pins | 1 2 3 4 5 6 7 | 14 13 12 11 10 9 8 |

---

### 1. Is the latch chip powered? — **U2 pin 14**

| Expect | About **3.3–3.5 V** |
|---|---|

3.6 V from the battery, less about 0.2 V across D1.

**If it reads 0 V, stop. This is almost certainly your fault**, and it explains the symptom completely: with no supply the chip cannot do anything, so the magnet does nothing, ever. Go to **D1 and C9** in the orientation list below.

---

### 2. Is the chip being held in reset? — **U2 pin 1**

| Expect | About **3.3–3.5 V** — the same as pin 14 |
|---|---|

Pin 1 is `1~RD`, the **reset**, and it is **active low**. Held low, the flip-flop is jammed with Q = 0, the gate stays at 0 V, and the MOSFET can never turn on — **exactly your symptom.**

**If it reads 0 V or anything under 2.3 V:**
- **R11** (1 MΩ, pulls pin 1 up to the rail) — dry joint or not fitted
- **C11** (100 nF, pin 1 to BATT−) — shorted
- **R9** (100 kΩ, goes to the Pro Mini's A1) — the MCU may be holding it down. See *Isolation test 1* below.

---

### 3. Is the set pin released? — **U2 pin 4**

| Expect | About **3.3–3.5 V** |
|---|---|

Pin 4 is `1~SD`, the **set**, also active low, and it should be tied to the rail. If this were low, Q would be stuck **high** and the device would be permanently ON — the opposite of what you have. Measure it anyway; it takes five seconds and it confirms the chip is sitting in a sane state.

---

### 4. Does the reed actually reach the chip? — **U2 pin 3**

| No magnet | About **0 V** |
|---|---|
| Magnet held on the reed | Rises to about **3.3 V** |

**If it never moves**, the reed is not closing. Power off, put the meter in continuity across S1's two pads and bring the magnet up. It should beep.

- No beep → the reed is not switching. The commonest cause is a **cracked glass seal** from bending a lead too close to the body. Test the reed on its own, off the board.
- Beeps at the pads but pin 3 does not move → **R12** (100 Ω, reed to pin 3) has a bad joint.

**Note the reed's position:** S1's two pads are **35 mm apart**, at the bottom edge of the board. The glass body sits **between** them, near the middle of that span. That is where the magnet has to go — not over either pad.

---

### 5. Does the flip-flop actually flip? — **U2 pin 5**

Watch pin 5 while you touch the magnet on and take it away, several times.

| Expect | It **flips** between about 0 V and about 3.3 V, once per magnet touch |
|---|---|

**If pin 3 pulses (step 4 passed) but pin 5 never changes:**
- The **pin 2 to pin 6 link** is missing or open. That link is what makes this a toggle; without it pin 2 floats and the chip will not flip.
- Or U2 is in backwards, or dead. See the orientation list.

---

### 6. Does the gate follow, and does the MOSFET conduct?

Measure **Q1's gate leg** to BATT− while pin 5 is high.

| Expect | Same as pin 5, about **3.3 V**. No current flows through R10, so there is no drop across it. |
|---|---|

- **Pin 5 is high but the gate is at 0 V** → **R10** (1 kΩ) is open, or the gate is shorted to the source.
- **Gate is at 3.3 V but Drain-to-Source still does not conduct** → either the MOSFET is dead, or **Gate and Source are swapped** — see Q1 in the orientation list. The IRLZ44N turns fully on at 3.3 V; its threshold is 1–2 V, so there is no doubt at 3.3 V.

---

## THE ORIENTATION LIST — FIVE PARTS THAT CAN BE IN BACKWARDS

You said every connection matches the PCB. That is exactly the situation where the fault is **orientation, not connection** — every wire is right and a part is turned round.

### D1 — the Schottky diode ⭐ check this first

This feeds the whole latch rail. Backwards, the rail is dead and nothing works. It is the single best match for your symptom.

| D1's end | Goes to | Which pad |
|---|---|---|
| **Striped end (cathode)** | the latch rail, towards U2 and Q1 | the **upper** pad |
| **Plain end (anode)** | BATT+, towards the battery connector | the **lower** pad |

The two D1 pads sit in a vertical line about 11 mm apart. **The stripe must point away from the battery connector.**

Quick check without unsoldering: meter on diode mode, red probe on the plain end, black on the striped end — should read about 0.2–0.3 V. Reverse the probes — should read open. If it reads open both ways, the diode is dead or has a dry joint.

### C9 — the 10 µF on the latch rail ⭐ check this second

C9 is polarised. In backwards it conducts and drags the rail down, which looks identical to a dead D1.

| C9 leg | Goes to |
|---|---|
| **Plus** | the latch rail — the **lower** of its two pads |
| **Minus** | BATT− — the **upper** pad |

Remember the stripe rule: on an **aluminium can** the stripe marks **minus**. If C9 is warm to the touch, it is in backwards.

### U2 — the 74HC74

The notch is on the **left** edge — the end nearest R11, R14 and C9. If it is in 180° round, pin 14 lands where pin 7 should be and the chip is powered backwards. **Touch it. If it is warm, it is in backwards and it is dead** — fit a new one.

### Q1 — the IRLZ44N

Hold the MOSFET with the **printed face towards you and the legs pointing down**: **left = Gate, middle = Drain, right = Source.** The metal tab is also Drain.

The three Q1 pads run in a vertical line. Working from the top:

| Position | Should be | Net |
|---|---|---|
| **top** pad | **Gate** | goes to R10 and R8 |
| middle pad | Drain | the switched ground |
| **bottom** pad | **Source** | BATT− |

**Check with the meter, power off:** the **bottom** leg must beep to the BATT− pad. The **top** leg must beep to R8's and R10's nearest pads. If those two are the other way round, Q1 is in 180° and the gate is wired to BATT− — it can never turn on.

*(Rotating a TO-220 does not move the middle pin, so the Drain stays right either way. Only Gate and Source swap — which is exactly the failure you are seeing.)*

### The reed

Not polarised, but the glass cracks. Covered in step 4.

---

## TWO ISOLATION TESTS

### 1. Take the Pro Mini out of the loop

The Pro Mini's **A1** pin connects to U2 pin 1 — the reset — through **R9, 100 kΩ**. R11 pulls that pin up with 1 MΩ. **R9 is ten times stronger than R11**, so if A1 is sitting low for any reason, it wins, and the latch is held in reset permanently. That would give you exactly this symptom with every wire correct.

When the device is off, the Pro Mini still has BATT+ on its VCC while its ground floats — it is in a strange half-powered state, and if firmware is loaded it may also be driving A1.

**Test:** unsolder **one leg of R9** and lift it. Then try the magnet again.

- **It works now** → the Pro Mini was holding the latch down. Fix it in firmware: A1 must be an input (high-impedance) at all times except the moment it commands a shutdown.
- **Still dead** → R9 is innocent. Solder it back.

If U1 is socketed rather than soldered, just pull the Pro Mini out instead — same test, no desoldering.

### 2. Bridge Drain to Source with wire, not the meter

Use a short piece of wire or a paperclip. The device should then stay on indefinitely instead of dying after 3 seconds, and you can check that the rest of the board behaves — buzzer, radio, sensors.

---

## ONE THING TO WATCH WITH THE CONTINUITY TEST

The MOSFET has a **body diode** built in, from Source to Drain. With the device off, a continuity test across Drain and Source can beep **in one probe direction only**, through that diode. That is normal and does not mean the MOSFET is on.

To test whether the MOSFET is really switching, do not use continuity. **Measure the voltage from the switched ground to BATT−:**

| Device off | About **3.6 V** |
|---|---|
| Device on | About **0 V** (a few millivolts) |

---

## THE SHORT VERSION

1. Black probe on BATT− and leave it there.
2. **U2 pin 14** → 3.3 V? If not, it is **D1 backwards** or **C9 backwards**.
3. **U2 pin 1** → 3.3 V? If not, it is **R11**, **C11**, or the Pro Mini via **R9**.
4. **U2 pin 3** → pulses with the magnet? If not, the **reed** or **R12**.
5. **U2 pin 5** → toggles? If not, the **pin 2–pin 6 link** or U2 itself.
6. **Q1 gate** → follows pin 5? If yes and it still will not switch, **Q1 is in backwards**.

My money is on **D1**, then **Q1 rotated**, then **R9 / the Pro Mini**.

Write down what each step reads and send me the six numbers — that pins it down exactly.
