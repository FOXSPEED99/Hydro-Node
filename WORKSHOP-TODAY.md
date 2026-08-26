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
