# THE 0.10 mA — WHAT IT MEANS AND WHAT TO DO

Written 2026-08-26, in plain language.
Your test: **Pro Mini + LoRa module only. No sensors plugged in.** Result: **0.10 mA**.

> ## ▶ RESULTS ARE IN — 2026-08-27. FINAL FIGURE **7.8 µA.**
> Everything below this box was written before the test. It is kept because the
> reasoning still explains *why*, but **the numbers in PART 1 are now out of
> date** — read `RESULTS` immediately below first.

---

# RESULTS — MEASURED 2026-08-27

| Stage | What it added | Measured | Change |
|---|---|---|---|
| 0 | nothing — radio in standby | **1.87 mA** | — |
| 1 | radio asleep | **0.68 mA** | **−1190 µA** |
| 2 | ADC + comparator off | **0.27 mA** | **−410 µA** |
| 3 | pin 13 LED properly off | **0.020 mA** | **−250 µA** |
| 4 | all pins given a defined state | **0.020 mA** | 0 |
| 5 | brown-out detector off | **4.8 µA** | **−15 µA** |
| 6 | watchdog running — the real product | **7.8 µA** | +3.0 µA |

**Final: 7.8 µA. The target was 25 µA. You beat it by more than three times.**

## What this does to the battery

| | Sleep energy over 2 years | Total needed | Margin on the 4400 mAh pack |
|---|---|---|---|
| The old measurement, 100 µA | 1752 mAh | 3125 mAh | 1.41× |
| The design target, 25 µA | 438 mAh | 1811 mAh | 2.43× |
| **What you achieved, 7.8 µA** | **137 mAh** | **1510 mAh** | **2.91×** |

And the thing that actually mattered — the gamble on the radio link:

| | at 100 µA | at 7.8 µA |
|---|---|---|
| **SF7** (easy link) | 1.41× ✅ | **2.91× ✅** |
| **SF9** (harder link) | **0.92× ❌ FAILS** | **1.39× ✅ PASSES** |

**The bet is off the table.** Whatever HW-047's link measurement comes back as,
the battery is no longer the thing that decides it. That was the whole point of
doing this, and it is done.

---

## THE ONE SURPRISE — AND IT IS IMPORTANT

**Stage 2 → stage 3 dropped 250 µA. I predicted 40 µA.** I was wrong by six
times, and the reason matters more than the number.

You also reported the key clue yourself: *"the led is on but it's so weak"* in
stages 1 and 2, and gone from stage 3.

Here is what is happening. Pin 13 is two things at once — the **LED**, and
**SCK, the clock wire going to the LoRa module**.

In stages 1 and 2 pin 13 is `INPUT_PULLUP`. So it is not driven; it is only
weakly held up through about 35 kΩ, and the LED hangs off it pulling down. Those
two fight, and the pin settles at **the LED's forward voltage — roughly 1.8 V**.

1.8 V is the problem. It is not a high and it is not a low. **It is exactly
halfway**, and the LoRa module's SCK input is staring at it.

A digital input given a halfway voltage does not sit quietly. Both halves of its
input transistor pair switch on at the same time and current pours straight from
3.3 V to ground through the chip, continuously, for as long as the halfway
voltage is there. That is where the 250 µA went — **most of it inside the LoRa
module, not in the LED**.

So the LED costs about 40 µA of its own, and then causes another ~210 µA in a
completely different chip by holding the clock line at a voltage that means
nothing.

### What to do about it

**Desolder the D13 LED, or its series resistor.** Do not rely on firmware alone.

Driving pin 13 low in the sleep routine fixes it — you measured that, it is
stage 3. But it only works as long as *every* piece of code always remembers.
The bootloader flashes that LED on every reset. A library that calls `SPI.end()`
leaves the pin as an input. One forgotten line anywhere and you are back to
250 µA and there is no warning, because the only symptom is a glow you cannot
see in daylight.

Take the part off and the problem cannot come back. This is HW-046 and it is no
longer a cosmetic issue.

**Optional 10-second confirmation:** flash stage 2 again and put the meter on
pin 13 while it sleeps. If it reads about 1.8 V instead of 0 V or 3.3 V, the
explanation above is confirmed directly.

---

## THE OTHER THING I GOT WRONG

**Stage 3 → stage 4 changed nothing.** Both read 0.020 mA.

Stage 4 is the one that gives every floating pin a defined state — A0, A3–A7,
D0, D1, D3, and the two ultrasonic pins. I said in HW-035 that floating pins
would cost "tens of microamps" and that the measurement would be meaningless
until they were fixed.

**On this board, they measured below what the meter can see — under about
10 µA, and possibly zero.** That was over-stated.

Keep the pin setup anyway. It costs nothing, it is not guaranteed to stay this
harmless across temperature or across different chips, and A0 in particular sits
next to the latch circuit. But it was not the problem, and I should not have
put it ahead of the LED.

---

## WHAT THE OTHER STAGES CONFIRMED

**Stage 0 → 1, −1190 µA. The radio is the whole game.** An awake SX1278 is
worth more than every other item on this board put together. Your radio-sleep
code works and the module answered its ID check.

**Stage 1 → 2, −410 µA. The ADC was on after all.** I said earlier that your
0.10 mA proved the ADC was already off. That was true of *your* firmware, not
of a bare sketch — this stage is measuring the default state. `ADCSRA = 0` is
worth about 250 µA on its own and it is one line.

**Stage 4 → 5, −15 µA. The brown-out detector is real and your fuse has it
enabled.** Matches the datasheet's ~20 µA. This is the line that has to sit in
exactly the right place or it silently does nothing.

**Stage 5 → 6, +3.0 µA. The watchdog costs 3 µA.** Cheaper than the 5 µA
budgeted, and you must pay it — it is what wakes the node every 2 minutes.

---

## WHAT IS LEFT

1. **Port the stage-6 sequence into the real node firmware.** Your production
   firmware is still the one that measured 100 µA. The hardware is now proven
   capable of 7.8 µA; the sketch shows exactly which lines get you there.
2. **Take the D13 LED off the board.** See above.
3. **Measure the ultrasonic module on its own** at 3.6 V, idle, on a bench
   supply. It is permanently powered on the sleeping rail and it is now the
   single largest unknown in the power budget — bigger than everything the
   Pro Mini and the radio do together. That is HW-071.

---

## PART 1 — "0.10 mA STILL GIVES 2 YEARS. ISN'T THAT ENOUGH?"

**You are right, and my last answer did not explain the real reason properly.**

Yes — 0.10 mA lasts two years. Think of the battery as a bucket with
**4400 units** in it:

| | Units used in 2 years | Left over |
|---|---|---|
| The plan (0.025 mA) | 1800 | 2600 spare |
| What you measured (0.10 mA) | 3100 | 1300 spare |

Both fit. Nothing is broken and nothing is urgent. So far you are correct.

### But that "2 years" assumes the radio link turns out easy

Here is the part I should have led with. **The radio's energy cost is not one
number.** It depends on how hard the Hub is to reach — 50 metres through thick
concrete. If the signal is weak, the radio has to send each message more slowly
so it survives the journey. That setting is called the spreading factor, SF.
A slower setting means the transmitter stays on longer for the same message,
and airtime is where almost all the active energy goes.

**You have not measured the link yet.** That is HW-047, still open. So you do
not know yet which of these columns you are in:

| | Radio energy in 2 years | + sleep at 0.025 mA | + sleep at 0.10 mA |
|---|---|---|---|
| **SF7** (easy link) | 1400 units | 1800 total — **fits, 2.4× spare** | 3100 total — **fits, 1.4× spare** |
| **SF9** (harder link) | 3000 units | 3500 total — **fits, 1.3× spare** | **4800 total — DOES NOT FIT** |

Read the bottom-right cell. At SF9 the bucket holds 4400 and the design needs
4800. **It runs out about two months early.**

So the honest answer to your question is:

> **0.10 mA is fine if the link is easy, and fails if the link is hard.
> You will not know which until you measure the link.
> 0.025 mA works either way.**

That is the whole reason to chase it. Not "2 years isn't enough" — it is
"2 years only holds if the other unknown goes your way, and you can remove that
gamble for free, using five lines of code and no parts."

The other things the spare has to cover are smaller but real: the cells lose
capacity in summer heat, and the two blocking diodes in HW-003 eat some voltage
so the node stops working slightly before the cells are truly empty.

**Bottom line: not urgent, not a blocker, but it costs nothing to fix, so fix
it before you measure the link rather than after.**

---

## PART 2 — THE GOOD NEWS IN YOUR NUMBER

Your measurement already proves two important things are working.

**The LoRa module is properly asleep.** ✅
An awake SX1278 draws about **1.6 mA**. You measured 0.10 mA — sixteen times
less. So your code is putting the radio to sleep correctly. That was the single
biggest risk and it is already handled.

**The ADC is already off.** ✅
The ADC is the part of the Arduino that reads voltages. Left running it costs
about **0.25 mA** all by itself. You measured less than half of that in total,
so it cannot be on.

Both of those would have shown up as a much bigger number. They didn't.

---

## PART 3 — SO WHERE IS THE 0.10 mA GOING?

Here is the idea you need, and it is the only idea in this document:

> **When you tell an Arduino to sleep, it does not switch everything off.
> Several parts inside it keep running unless your code specifically
> switches each one off, one at a time, by name.**

That is all this is. It is a **code** problem, not a soldering problem.
There is nothing wrong with your board.

Here are the parts that are probably still awake, and what each one costs:

| What is still awake | Costs you | Is it needed? |
|---|---|---|
| The brown-out detector | **0.020 mA** | No — switch it off |
| The little LED on pin 13 | **0.040 mA** | No — switch it off |
| Empty pins with nothing connected | **~0.020 mA** | No — give them a state |
| The analog comparator | ~0.010 mA | No — switch it off |
| The watchdog timer | 0.005 mA | **Yes — keep it.** This is what wakes the node every 2 minutes |

Add those up: **0.095 mA.** You measured 0.10 mA.

That is not a coincidence. It means there is no mystery fault hiding anywhere —
the number is fully explained by four things that are simply switched on and
shouldn't be.

---

## PART 4 — THE LED ON PIN 13

This one deserves its own section, because it is the biggest single item and
because it looks like it isn't happening.

You said: *"there's an LED I didn't remove, it's attached to pin 13, but it's
turned off anyway while in deep sleep."*

**It looks off. It may not be off.**

A pin on the Arduino can be set to four different states. What the LED does
depends entirely on which one your code left it in:

| Pin 13 is set to | LED current | What you see |
|---|---|---|
| output, LOW | 0 | off — genuinely off |
| input, plain | 0 | off — genuinely off |
| **input with pull-up** | **0.040 mA** | **looks off. It is not off.** |
| output, HIGH | 1.5 mA | obviously lit up |

The third row is the trap.

"Input with pull-up" means the chip connects a small internal resistor from
3.3 V to that pin. If an LED is sitting on that pin, that resistor pushes a
tiny current out of the pin, through the LED, to ground. About **0.040 mA**.

An LED at 0.040 mA **does** light up. But it is roughly forty times dimmer than
a normal indicator LED, which is far too faint to notice in a lit room.

### Test it yourself in ten seconds

Put the board to sleep. Go into a **dark room**, or just cup both hands tightly
over the Pro Mini and look into the gap.

- **You see a faint red glow** → that is 0.040 mA of your 0.10 mA, found.
- **Completely black** → the LED is genuinely off, cross it off the list.

Either way you learn something real, and it costs nothing.

---

## PART 5 — THE TEST SKETCH — FLASH IT AND FIND OUT

Stop guessing. There is a sketch ready to flash:

**`firmware/sleep_test/sleep_test.ino`**

It does nothing except go to sleep. At the top there is one line:

```c
#define STAGE 0
```

Change that number, flash, measure, write the number down, repeat. **Seven
flashes, about half an hour**, and at the end you know exactly where every
microamp goes instead of us arguing about it.

| Stage | What it switches off | Expect |
|---|---|---|
| 0 | nothing — radio still in standby | ~1.70 mA |
| 1 | radio asleep | ~0.10 mA |
| 2 | ADC + comparator off | ~0.10 mA |
| **3** | **pin 13 LED properly off** | **~0.06 mA ← the LED test** |
| 4 | all pins given a defined state | ~0.04 mA |
| 5 | brown-out detector off | ~0.02 mA |
| 6 | watchdog running — the real product | ~0.025 mA |

**Stage 2 → stage 3 is the LED measurement.** In stages 1 and 2 the sketch
deliberately leaves pin 13 as `INPUT_PULLUP` — the exact state we suspect your
firmware leaves it in. Stage 3 fixes it. The difference between those two
readings *is* what the LED costs, measured rather than argued about.

### Three things that will ruin the readings if you forget them

1. **Unplug the USB-TTL programmer before you measure.** It pushes current
   backwards into the board through the RX and DTR wires. Flash, pull all six
   wires off, then measure. This is the single most common way to get a
   nonsense number.
2. **Use the µA range**, or a 10 Ω resistor in the battery negative lead with
   the meter reading millivolts across it. On the mA range you cannot see the
   difference between stages 3, 4 and 5 at all.
3. **The board must be switched on** — magnet toggled, MOSFET conducting.

### How it tells you what it is doing

There is no serial output, because a serial adapter would ruin the measurement.
It uses the LED instead, then stops forever:

- **First group, slow blinks** — which stage is running. Stage 0 blinks once,
  stage 1 twice, and so on. This is how you confirm you flashed what you think
  you flashed.
- **Second group, fast blinks** — the radio check:
  - **2 fast blinks** = the radio answered correctly and is now asleep. Good.
  - **6 fast blinks** = the radio did not answer. Stop; the reading means
    nothing until the SPI wiring is fixed.

The sketch reads the Ra-02's ID register and checks it comes back as `0x12`,
then commands sleep and reads the mode back to confirm it took. So it does not
assume the radio went to sleep, it **proves** it.

---

## PART 5b — WHAT GOES IN THE REAL FIRMWARE

Once the test tells you which stages matter, this is what belongs in the actual
node code, before it sleeps.

```c
// 1. Pin 13's LED, properly off
SPI.end();
pinMode(13, OUTPUT);
digitalWrite(13, LOW);

// 2. ADC off (probably already off — do it anyway, it costs nothing)
ADCSRA = 0;

// 3. Analog comparator off
ACSR |= (1 << ACD);

// 4. Every pin with nothing connected to it gets a defined state
pinMode(A0, INPUT_PULLUP);   // A0 is bare now that R13 came off
pinMode(A3, INPUT_PULLUP);
pinMode(A4, INPUT_PULLUP);
pinMode(A5, INPUT_PULLUP);
pinMode(A6, INPUT_PULLUP);
pinMode(A7, INPUT_PULLUP);

// 5. Brown-out detector off — the biggest single win
set_sleep_mode(SLEEP_MODE_PWR_DOWN);
cli();
sleep_enable();
sleep_bod_disable();
sleep_cpu();
sei();
sleep_disable();
```

**One warning about item 5.** `sleep_bod_disable()` must sit exactly where it is
— after `sleep_enable()` and immediately before `sleep_cpu()`, with `cli()`
before them. The chip only remembers "brown-out off" for four clock cycles.
If you put anything in between, it silently forgets and you get nothing.

**One warning about item 4.** `INPUT_PULLUP` is safe here **only because those
pins connect to nothing.** Never do it to a pin that touches the latch circuit
(A1, or A0 if you ever reconnect it). That is exactly how the board failed to
switch on last week.

### Expected result

If all five work, you should land somewhere around **0.020–0.030 mA**, which is
the original target and takes you back to a comfortable 2600 units of spare.

---

## PART 6 — TWO THINGS FOR LATER, NOT TODAY

**1. Your 0.10 mA is not the final number.**
You tested with the sensors unplugged. In the finished product they are plugged
in, and they are on the rail that stays powered while the node sleeps. Write
this down so it isn't forgotten:

| Plug in | Expect it to add |
|---|---|
| J2, the two temperature probes | ~0.0015 mA — nothing, this is fine |
| **J3, the ultrasonic module** | **possibly 2 mA or more — this would be a real problem** |

The ultrasonic module has its own microcontroller inside it, and right now
nothing ever switches it off. **Measure it on its own** with a bench supply at
3.6 V, sitting idle, before you worry about it. It is tracked as HW-071 and the
fix is one transistor on the D3 pin, which is free — but measure first.

**2. Re-read the number on a better range.**
"0.10 mA" on a milliamp range is a **single count** on the display. The meter
cannot resolve better than about 0.01 mA there, and the meter's own internal
resistance drops enough voltage to change what the circuit does.

Switch the meter to a **microamp (µA)** range and read it again. Better still,
put a **10 Ω resistor** in the battery negative wire and measure the millivolts
across it — 1 mV across 10 Ω is exactly 0.1 mA, and it barely disturbs anything.

You cannot tell 0.10 mA from 0.14 mA on the range you used, and once you start
removing 0.02 mA at a time you need to see those steps.

---

## THE SHORT VERSION

1. 0.10 mA still lasts two years. It just leaves no spare. Worth fixing, not urgent.
2. You were right that 0.10 mA gives two years. It gives two years **only if the
   radio link turns out easy.** At SF9 it runs out two months early, and you have
   not measured the link yet. 0.025 mA works either way — that is the real reason.
3. Your LoRa sleep code works and your ADC is already off. Both confirmed by the number itself.
4. The remaining 0.10 mA is four things inside the Arduino that the code never switched off.
5. Biggest single item is probably the pin-13 LED, glowing too faintly to see. Check it in a dark room.
6. **Flash `firmware/sleep_test/sleep_test.ino`** and step through the seven
   stages. Unplug the programmer before each reading, and use the µA range.
7. Five lines of code should get you to 0.02–0.03 mA.
8. Then measure the ultrasonic module on its own before plugging it in.
