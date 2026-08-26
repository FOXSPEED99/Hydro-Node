# THE 0.10 mA — WHAT IT MEANS AND WHAT TO DO

Written 2026-08-26, in plain language.
Your test: **Pro Mini + LoRa module only. No sensors plugged in.** Result: **0.10 mA**.

---

## PART 1 — IS 0.10 mA BAD?

No. It is worse than planned, but the device still works for two years.

Think of the battery as a bucket with **4400 units** in it.

| | Units used in 2 years | Units left over |
|---|---|---|
| The plan (0.025 mA) | 1800 | 2600 spare |
| What you measured (0.10 mA) | 3100 | 1300 spare |

Both fit in the bucket. So **nothing is broken and nothing is urgent.**

But the spare is what protects you from the things you cannot control yet:
the battery losing capacity in the summer heat, the radio needing to shout
louder than expected to reach the Hub, the blocking diodes eating some voltage.

**2600 spare is comfortable. 1300 spare is thin.** That is the whole problem.
Not "it will die", just "there is no room left for surprises".

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

## PART 5 — WHAT TO ACTUALLY DO

### In the code, before it sleeps

Five lines. Add them one at a time and re-measure after each one, so you can
see what each is worth.

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
2. Your LoRa sleep code works and your ADC is already off. Both confirmed by the number itself.
3. The remaining 0.10 mA is four things inside the Arduino that the code never switched off.
4. Biggest single item is probably the pin-13 LED, glowing too faintly to see. Check it in a dark room.
5. Five lines of code should get you to 0.02–0.03 mA.
6. Then re-measure on a µA range, and measure the ultrasonic module separately before plugging it in.
