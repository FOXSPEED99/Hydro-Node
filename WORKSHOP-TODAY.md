# WORKSHOP LIST — today, on the board you built

2026-08-24. Physical rework on the existing hand-built PCB. About an hour.

---

## ⚠️ FIRST — I GAVE YOU ONE WRONG INSTRUCTION

**Do NOT change R9.** I told you to make it 1 MΩ. That would break the device's ability to shut itself down.

R9 is how the Pro Mini pulls the flip-flop's reset low. R11 holds that pin up with 1 MΩ. The two form a divider, so R9 has to be the **stronger** one or the MCU can never win:

| R9 | Pin 1 when A1 drives low | |
|---|---|---|
| **100 kΩ (as built)** | **0.32 V** | ✅ resets |
| 220 kΩ | 0.63 V | ✅ resets |
| 470 kΩ | 1.12 V | ❌ too high, will not reset |
| 1 MΩ | 1.75 V | ❌ will not reset |

A valid low needs to be under 1.05 V. **R9 at 100 kΩ is correct. Leave it exactly as it is.**

The leak through R9 pushes pin 1 *up*, which is the harmless direction — your 3.2 V reading is that leak and it is still a solid high. Nothing to fix here. The A0 side was the dangerous one, and that is what you are removing below.

---

## BRING WITH YOU

| Part | For | Notes |
|---|---|---|
| **2 × 2.2 MΩ** resistor | R14 | if you cannot get 2.2 MΩ, **two 1 MΩ in series** = 2 MΩ, near enough |
| **1 × 10 µF ceramic 16 V** (1206) or tantalum | replaces C9 | |
| **2 × 100 nF ceramic** | extra decoupling at U3 | |
| Isopropyl alcohol + a stiff brush | cleaning flux | genuinely matters for step 2 |
| Multimeter that reads **µA** | sleep current | if yours only does mA, the sleep test has to wait |

---

## THE WORK, IN ORDER

### 1. Remove R13 completely — 2 minutes

One leg is already lifted. Take the whole resistor off and leave the pads empty.

A0 is now disconnected from the latch. Nothing uses it yet, and in Rev B it comes back on the Schmitt trigger's output where the leak cannot reach it.

☐ R13 removed, both pads clear

---

### 2. R14: 470 kΩ → 2.2 MΩ — 10 minutes

This is the reed-chatter fix. R14 sets how long the board **ignores everything after a toggle**:

| R14 | Ignore window | Rising edge into the clock | Magnet-held current |
|---|---|---|---|
| 470 kΩ (as built) | 57 ms | 10 µs | 7.4 µA |
| **2.2 MΩ** | **265 ms** | **10 µs — unchanged** | **1.6 µA** |

Nearly five times the window, the clock edge is untouched, and the magnet-held current drops. Changing R14 is much safer than changing C12, because C12 would slow the clock edge and the flip-flop may not like that.

**Then clean the flux** around U2 pins 1 and 3 and around R14 with IPA and a brush, and let it dry. At 2.2 MΩ, sticky flux residue between pads can conduct enough to hold the pin up.

**Now verify, before anything else:**

☐ R14 replaced with 2.2 MΩ
☐ Flux cleaned around U2 and R14, dry
☐ Power on, black probe on BATT−, **U2 pin 3 with no magnet reads under 0.3 V**

**If pin 3 does not sit under 0.3 V**, your chip's leakage is too high for 2.2 MΩ. Drop to **1 MΩ** (120 ms window, still twice as good as now) and move on. Tell me if that happens.

**Then test it the way that used to fail:** bring the magnet in **slowly**, ten times, and take it away each time.

☐ 10 slow approaches, 10 clean single toggles

---

### 3. Replace C9 — 10 minutes

C9 is the 10 µF aluminium can on the latch rail. Swap it for **10 µF ceramic 16 V** or a tantalum.

**Do this before you measure sleep current.** C9 sits on a rail that is live whether the device is on or off, and its datasheet leakage is up to 5 µA against a 25 µA target — so an aluminium can there will inflate your sleep reading and you will not know whether the number is the circuit or the capacitor.

If tantalum: **the bar is PLUS.** Plus goes to the lower pad (the latch rail), minus to the upper pad (BATT−). Ceramic has no polarity.

☐ C9 replaced

---

### 4. Add one extra 100 nF at the radio — 5 minutes

Solder a 100 nF ceramic **directly across U3's 3.3 V pin and its nearest GND pin**, on the underside, legs as short as you can cut them.

C6 is 12 mm away, which is too far for it to do its job during a transmit. Rather than move C6 — which means cutting traces — just add a second one where it should have been. C6 stays where it is and does no harm.

☐ 100 nF added at U3, legs short

---

### 5. Do NOT touch these

- **R9** — see the correction at the top
- **C7, C8** — the aluminium ones at the radio. They should change in Rev B but they are not hurting anything today
- **C6** — leave it, step 4 covers the problem
- Anything to do with vias, the ground pour, silkscreen or mounting holes — those are Altium changes for Rev B, not solderable on this board

---

## THEN — THE MEASUREMENTS

Power from the pack, black probe on BATT−.

| | Measure | Target |
|---|---|---|
| ☐ | **Sleep current** — device on, MCU asleep | **≤ 25 µA** |
| ☐ | Current with the magnet resting on the reed | ~1.6 µA now that R14 is 2.2 MΩ |
| ☐ | Current with the device **off** | as low as it will go — write it down |
| ☐ | U2 pin 14 during a transmit | stays above **2.0 V** |
| ☐ | Buzzer, radio link to the Hub, both temp probes, ultrasonic, flow switch | works |
| ☐ | Ultrasonic blind zone — flat target, 2 cm outward in 5 mm steps | write down where it starts reading |
| ☐ | Transducer centre-to-centre spacing, callipers | write it down |

**Sleep current is the important one.** The entire two-year battery target rests on it and it has never been measured.

---

## SEND ME

1. Did pin 3 sit under 0.3 V with R14 at 2.2 MΩ?
2. Did the ten slow magnet approaches give ten clean toggles?
3. The sleep current.

That is enough to close out the bring-up and freeze the Rev B change list.

---

## WHAT IS *NOT* ON THIS LIST, AND WHY

The 2.2 MΩ is a good prototype fix, but it is **not the production answer**. It works because your particular 74HC74 leaks far less than its datasheet allows — the worst-case number would lift a 2.2 MΩ node by 2 V. Relying on a chip being better than its specification is fine on a bench and not fine on a roof in Syria for two years.

**Rev B still gets the Schmitt trigger** (HW-069), which makes the ignore-window as long as you like without depending on anyone's leakage, and puts A0 back on its driven output so HW-067 is fixed structurally instead of by resistor ratio.


---

# GROUND PLANE ON THE HAND-BUILT BOARD — AND THE RADIO

Added 2026-08-24, after seeing photos of the built board.

## The question: would a 7 × 9 cm copper sheet, wired to all the grounds, act as a ground plane?

**Partly, and not in the way that matters most.**

A ground plane works because the return current flows in copper **directly beneath the trace**, a fraction of a millimetre away, so the loop the current goes round is tiny. A separate sheet joined by wires is connected at **points**. The return current still travels along your solder tracks to reach a wire before it can get into the sheet — the loop area is exactly what it was. You gain a good low-inductance common reference between the points you bond, and some shielding. You do not gain the thing a plane is actually for.

## But the plane is not what limits your range — a correction to what I told you earlier

The **272 Ω** figure in HW-004 assumes **433 MHz current flowing on the board ground**. That is true when a whip is soldered straight to the PCB. It is **not** true on your build: the antenna leaves on a u.FL pigtail to an SMA bulkhead, so the RF return is **the coax shield**, and only a small common-mode leakage rides the board.

What your board ground actually carries is the transmit **supply pulse**, and that is a far lower frequency:

| Ground path | Inductance | Drop at 120 mA, 1 MHz |
|---|---|---|
| 20 mm of wire | ~20 nH | **15 mV** |
| 60 mm solder track | ~60 nH | **45 mV** |
| long thin daisy chain | ~150 nH | 113 mV |

Out of 3600 mV. **Your perfboard grounding is not costing you range.** The imperfect ground mostly makes the radiation *pattern* less predictable — the board becomes a small part of the antenna system — rather than losing you decibels outright.

## What actually helps range, in order

### 1. The antenna's counterpoise — the big one, and the right job for that copper sheet

At 433 MHz, λ = 69 cm and a quarter wave is **17.3 cm**. A quarter-wave whip is only half an antenna; the other half is the **counterpoise** — the metal it works against. Screw an SMA bulkhead into a plastic wall with nothing behind it and the counterpoise is whatever the coax braid happens to be, which is why range on builds like this is often unrepeatable between two identical boxes.

**Use the sheet here instead of under the board:**

- Mount it flat inside the enclosure wall, at the antenna end
- Drill it and **bolt the SMA bulkhead through it**, metal to metal
- Bond it to the coax shield at that point
- Keep it as large as the box allows and roughly centred on the antenna

A 7 × 9 cm plate is **0.13 λ** across — a partial counterpoise, not a full one. Bigger is better; a proper one wants a radius of about 17 cm. But partial beats absent by a wide margin, and it is the single highest-value thing you can do for range with the parts in your hand.

### 2. The 100 nF at the module — already on today's list

Directly across the Ra-02's 3.3 V and nearest GND pin, legs as short as they cut.

### 3. One short, thick ground wire for the radio

From the Ra-02's GND pins to the MOSFET drain, direct — **not** daisy-chained through the perfboard tracks. That takes the 60 mm path down to about 20 mm and drops the TX-pulse ground bounce from ~45 mV to ~15 mV.

## If you still want the sheet under the board

⚠️ **Your solder side is bare copper across the whole board.** Look at the photo — every track is exposed tinned solder. A copper sheet laid against it shorts the entire circuit instantly.

If you do it anyway:

- **Insulate**: Kapton tape over the solder side, or standoffs holding the sheet 3–5 mm clear
- **Bond it to the switched ground (GND), not BATT−** — GND is what the radio and the Pro Mini return through
- **Many short connections, not one** — especially right under the Ra-02
- Accept that it buys you a common reference and some shielding, not a real plane

My advice: put the sheet at the antenna instead. Same piece of copper, far more benefit.

## ⚠️ Two things from the photos

**Clean the flux.** The solder side is covered in it. This matters *today*: at 2.2 MΩ, flux residue between pads conducts enough to hold U2 pin 3 up, and the R14 change will look like it failed when it did not. Scrub with IPA and let it dry properly before you judge the result.

**The u.FL connector on the Ra-02 is empty.** **Do not key the transmitter without an antenna connected.** The PA has nowhere to send its energy and it comes back into the chip. Fit the pigtail and antenna before any radio test.


---

# REVISION — YOUR LIST IS SHORTER THAN I MADE IT

Added 2026-08-24 after three questions from the bench. Two items I listed as "worth doing" are **not worth doing today**, and I should have separated "needed" from "nice" more clearly the first time.

## What the A0 pin does now — the honest answer

The pin is **A0**. R13 was the 100 Ω between A0 and the 74HC74's pin 3, and A0 went nowhere else on the whole board.

**With R13 gone, A0 is simply an unconnected pin.** Nothing is broken and nothing stops working.

| | |
|---|---|
| What was lost | the MCU can no longer see when a magnet is applied |
| What uses that today | **nothing** — no firmware feature depends on it |
| What it was for | a future "hold the magnet 5 seconds to unpair" gesture (**HW-022**) |
| When it comes back | Rev B, on the Schmitt trigger's output, where the leak cannot reach it |

**⚠️ One thing you must do in firmware, and it matters for the measurement you are about to take.** A0 is now a **floating input**. A floating CMOS input drifts around the switching threshold and the input stage oscillates, which burns current continuously — exactly what **HW-035** is about, and it will show up in your sleep-current reading.

So in `setup()`, treat A0 like any other unused pin: **`pinMode(A0, INPUT_PULLUP)`**, or set it as an output driven low. Either is fine; do not leave it as a bare floating input.

## R14 → 2.2 MΩ is optional. Here is what it is actually for

**It has nothing to do with the fault you fixed.** Removing R13 fixed *"the device never turns on"*. That is done, and it stays done.

R14 is for the **other** problem you reported — the slow magnet approach that gives on-off-on. R14 sets how long the board ignores everything after a toggle:

| R14 | Ignore window |
|---|---|
| 470 kΩ, as built | 57 ms |
| 2.2 MΩ | 265 ms |

Hand chatter runs over a few hundred milliseconds, so **265 ms will reduce it and may not eliminate it.** It is an improvement, not a cure — the cure is the Schmitt trigger in Rev B.

**Do it only if you have the resistor in your hand.** If you do not, skip it. The board works, and the chatter is a known issue with a designed fix already written down. Not worth a trip.

## C9 — skip it today. Measure first

I put this on the list to protect the sleep-current reading, and on reflection that was over-cautious.

The 5 µA figure is the **datasheet maximum at the part's rated 50 V**, measured two minutes after applying it. Your C9 sits at **3.5 V on a 50 V part** — real leakage there is normally well under 1 µA, nowhere near the 5 µA worst case.

And the other reason to change it — aluminium electrolytics drying out at rooftop temperature (**HW-058**) — is a *two-year* problem. Your prototype is not going to sit on a roof for two years.

**So: measure the sleep current with C9 as it is.**

| Sleep current | What it means |
|---|---|
| comfortably under 25 µA | C9 is fine. Leave it. Change it in Rev B as a BOM line, not a rework. |
| near or over 25 µA | *then* swap C9 for ceramic and measure again — now the test tells you something |

That way you only do the work if it turns out to be needed, and if it is, the before-and-after actually means something.

## The revised list

**Must do**
- ☐ Finish removing R13 — both pads clear
- ☐ `pinMode(A0, INPUT_PULLUP)` in firmware before measuring sleep current

**Cheap and worth it**
- ☐ Clean the flux around U2 pins 1 and 3
- ☐ One 100 nF directly across U3's 3.3 V and nearest GND pin, short legs

**Only if you have the part**
- ☐ R14 → 2.2 MΩ, then check pin 3 sits under 0.3 V, then ten slow magnet approaches

**Do not do today**
- C9 — measure first, swap only if the sleep current says so
- C6, C7, C8, R9, and everything layout-related
