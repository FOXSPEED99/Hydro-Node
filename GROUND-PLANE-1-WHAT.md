# GROUND PLANE — WHAT WE ARE DOING AND WHY
Hydro Node, issue **HW-004** (with **HW-007** folded in — they are the same problem).

Scope: **the ground plane only.** No new components, no other changes.
The click-by-click execution is in `GROUND-PLANE-2-HOWTO.md`.

---

## 1. WHAT A GROUND PLANE IS

Instead of connecting every GND pin with thin traces, you fill an entire layer of the board with solid copper and connect everything to it.

That is the whole idea. The interesting part is *why* it matters, because it is not about resistance.

---

## 2. THE ONE THING TO UNDERSTAND: RETURN CURRENT

Current always flows in a **loop**. When the Ra-02 draws 120 mA during a transmission, that current leaves the battery, passes through the module, and **has to come back**. It does not vanish at the GND pin.

On the current board it has to travel all the way back along one specific 1 mm trace.

Here is the part that is not obvious:

- **At DC**, return current follows the path of least **resistance**.
- **Above about 100 kHz**, it follows the path of least **inductance** — which is *directly underneath the outgoing trace*, as close as the board material allows.

A ground plane lets it do exactly that. Without one, the return is forced along whatever route was drawn.

---

## 3. THE NUMBER THAT MATTERS

Resistance is a red herring here. The 1 mm / 35 µm trace is 0.48 mΩ per mm, so a 100 mm ground run is 48 mΩ — **5.8 mV at 120 mA**. Nothing.

**Inductance is the problem.** A 1 mm trace with no copper beneath it is roughly **1 nH per mm**:

| Ground trace length | Inductance | Impedance at 433 MHz |
|---|---|---|
| 50 mm | 50 nH | 136 Ω |
| **100 mm** | **100 nH** | **272 Ω** |
| 150 mm | 150 nH | 408 Ω |

| With a plane | ~1–2 nH | **2.7–5.4 Ω** |

> **The Ra-02's ground connection is currently around 272 Ω at its own operating frequency, instead of under 5 Ω.**

That single sentence is the entire issue.

---

## 4. WHAT THAT CAUSES

**1. The radio module's antenna matching is detuned.**
The Ra-02 has an on-board power amplifier and matching network that assume solid ground beneath them. Give it an inductive ground and the match shifts — you lose transmit power and receive sensitivity. On a 50 m link through thick concrete, that is margin you cannot spare.

**2. The board itself becomes an antenna.**
At 433 MHz the wavelength is 69 cm, and a conductor radiates usefully above about λ/20 = 3.5 cm. The ground traces are 5–15 cm, which is λ/14 to λ/5. Energy leaving through the PCB is energy that did not leave through the antenna — and it couples straight back into the Node's own receiver.

**3. The radiating loop is 12× larger than necessary.**
Radiated field scales with loop **area**. Signal out 100 mm and back 100 mm, separated by 20 mm, encloses 2000 mm². With a plane the return hugs the trace across 1.6 mm of board thickness — **160 mm²**.

**4. Every subsystem shares one ground conductor.**
The ultrasonic module's return current flows through the same copper as the MCU's and the radio's. Current from one becomes a voltage the others read as ground. That is ground bounce, and it lands directly on the ultrasonic echo timing — which is the measurement this product sells.

---

## 5. WHAT IS ACTUALLY WRONG WITH THIS BOARD

Read directly out of `Hydro-Node.PcbDoc`:

| Property | Current value | Should be |
|---|---|---|
| Copper pours / polygons | **0** | One large pour, plus one small one |
| Vias | **0** | Ground pins tied to the pour, plus stitching |
| Tracks on Bottom layer | 227 | — |
| Tracks on Top layer | **6** | — |
| Track width, every net | 1.0 mm uniform | — |
| Ra-02 ground pins connected | **1 of 4** (J2.8 only) | All 4 |

That last row is **HW-007** and it is part of the same job: J1 pin 1, J1 pin 2 and J2 pin 1 are all GND on the module and all left floating. The whole 120 mA transmit return goes through a single pin.

---

## 6. THE APPROACH — AND WHY IT IS SMALL

The obvious plan is "move all the signals to the top layer and pour the bottom." That means relocating **227 tracks**.

**Do the opposite.** A plane works by being on the layer *adjacent* to the signals — it does not care which physical layer that is. The signals are already on the bottom, so:

> **Pour the ground on the TOP layer.** That means moving **6 tracks**, not 227.

Same electrical result, a fraction of the work. And because the components are through-hole, every ground pin already passes through the board — so its top-side pad connects to a top-side pour automatically, with **no vias needed for the component pins at all**.

---

## 7. ONE THING SPECIFIC TO THIS DESIGN: TWO GROUNDS

This board has **two separate ground nets**, divided by the power MOSFET Q1:

| Net | Altium name | What it is | Treatment |
|---|---|---|---|
| **GND_SW** | `NetC3_1` | Return for the radio, MCU, ultrasonic, temperature, flow, LED — every signal and every load | **This gets the main pour** |
| **GND_RAW** | `Net1_1` | Battery negative, Q1 source, and the latch circuit (microamps) | Small local pour around U1 / S1 / Q1 only |
| VBAT | `Net1_2` | Battery positive | (for reference — not poured) |

> **Never merge the two pours.** They are separated by Q1 on purpose. Joining them shorts across the master power switch and the device can never turn off.

---

## 8. WHEN THIS IS DONE

| # | Check | Pass |
|---|---|---|
| 1 | A solid `NetC3_1` pour exists on the Top layer | Yes |
| 2 | A separate `Net1_1` pour exists around U1 / S1 / Q1, not touching the first | Yes |
| 3 | All four Ra-02 ground pins connected | J1.1, J1.2, J2.1, J2.8 |
| 4 | No long slots cutting through the main pour | Visual check |
| 5 | Solid copper under the whole Ra-02 footprint | Visual check |
| 6 | No isolated copper islands after repouring | Altium check |
| 7 | Design Rule Check passes | 0 errors |
| 8 | Hub-logged RSSI, same Node, same place, old board vs new | Should measurably improve |

Test 8 is the real proof. Everything before it is process.
