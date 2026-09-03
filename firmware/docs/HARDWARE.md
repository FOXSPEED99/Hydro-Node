# Hydro Node — hardware facts the Section 1 firmware relies on

Everything below was extracted from `Hydro Node Parts & Schematic/Schematic/Hydro_Node_Schematic.SchDoc`
by parsing the Altium file, not read off a rendered image. The schematic has
only three net labels (`BATT+`, `BATT-`, `GND`); every other connection is drawn
wire geometry, so the netlist was rebuilt from wire vertices, junction objects
and pin coordinates.

Re-derive it after any schematic edit and check it against this file.

---

## 1. Pin map

| Signal | Arduino | AVR | Path from the connector |
|---|---|---|---|
| Ultrasonic TRIG | D6 | PD6 | `J3.4 → R2 100 Ω → D6` |
| Ultrasonic ECHO | **D8** | **PB0 / ICP1** | `J3.3 → R1 100 Ω → D8` |
| DS18B20 power | D3 | PD3 | `J2.3 → D3`, and `R7 4.7 kΩ` pull-up referenced to D3 |
| DS18B20 data | D4 | PD4 | `J2.1 → R4 100 Ω → D4` |
| Flow switch, digital | D5 | PD5 | `J1.2 → R5 100 Ω → D5` |
| Flow switch, analogue | A2 | PC2 / ADC2 | `J1.2 → R3 330 Ω → A2` |
| Buzzer | D7 | PD7 | `LS1.P → R9 100 Ω → D7` |
| **Latch clear** | **A1** | **PC1** | `A1 → R11 100 kΩ → U2.1 (74HC74 1CLR, active low)` |
| LoRa DIO0 | D2 | | `U3.5` |
| LoRa RESET | D9 | | `U3.4` |
| LoRa NSS | D10 | | `U3.15` |
| LoRa MOSI / MISO / SCK | D11 / D12 / D13 | | `U3.14 / U3.13 / U3.12` |
| Programming | D0 / D1 / DTR | | `J? "Programing", 7-pin` |

Genuinely unconnected on this board: **A0, A3, A4, A5, A6, A7, RAW, RST**, and
`U3` DIO1–DIO5. `U2.8` and `U2.9` (the unused flip-flop's outputs) are correctly
left open.

### Connectors

| Ref | Silkscreen | Part | Pin 1 | Pin 2 | Pin 3 | Pin 4 |
|---|---|---|---|---|---|---|
| J1 | Flow Switch | B2B-XH-A | switched GND | flow node | — | — |
| J2 | Temp | B3B-XH-A | 1-Wire data | switched GND | sensor VDD (D3) | — |
| J3 | Ultrasonic | B4B-XH-A | switched GND | BATT+ | ECHO | TRIG |

---

## 2. Power architecture, and why it matters to firmware

The MCU's **VCC is `BATT+`, unswitched**; its **GND is the switched ground**
downstream of `Q1` (IRLZ44N, low-side switch). A 74HC74 D-type latch wired as a
toggle drives that MOSFET, clocked by the reed switch `S1` through a
100 Ω / 100 nF / 2.2 MΩ debounce network, and fed from `BATT+` through `D1`
(1N5819) with `C9` 10 µF + `C10` 100 nF holding the latch's rail up during a
transmit burst. `R12` 1 MΩ and `C11` 100 nF form a ~100 ms power-on reset into
the active-low `1CLR`, so a Node powers up **OFF** when a cell is first fitted.

Two consequences the firmware must respect:

1. **`A1` can turn the device off.** It reaches `1CLR` through 100 kΩ. Driving
   it low clears the latch, opens `Q1`, and the only way back is a magnet at the
   enclosure. Section 1 keeps `A1` a high-impedance `INPUT` at all times and
   never calls `pinMode(A1, OUTPUT)`. The commanded-shutdown path belongs to a
   later section (low-battery cut-off) and is the only thing entitled to use it.

2. **The ultrasonic module's VCC is `BATT+` and cannot be switched** (`J3.2`).
   Unlike the DS18B20 there is no rail to drop between readings, so its ~1.5 µA
   standby is a permanent cost. The DS18B20 *is* switchable — from `D3` — and
   the driver uses that.

---

## 3. Design decisions in the hardware that the firmware is built around

These are not incidental; each one changes how the corresponding driver works.

**ECHO is on D8 = PB0 = ICP1.** This is the ATmega328P's input capture pin. The
Stage 0 review raised the previous revision's placement as **HW-018** — *"Echo
is on D7 instead of D8, so hardware input capture is unavailable"* — and
recommended swapping echo to D8. This board is that fix. The driver therefore
uses Timer1 input capture rather than `pulseIn()`: the edges are timestamped in
hardware with no interrupt-latency jitter, and the CPU can idle-sleep through
the flight time instead of spinning.

**The DS18B20 is powered from a GPIO, with its pull-up referenced to that same
GPIO.** Driving `D3` low depowers the sensor *and* removes the 4.7 kΩ pull-up's
drain in one action, so the whole 1-Wire subsystem costs nothing between
readings. The driver must never leave `D3` high, and must release the data line
before dropping the power pin — otherwise `D4` back-feeds the sensor through its
input protection diode and leaves it half-alive at an undefined voltage.

**The flow switch has an external 1 MΩ pull-up, not an internal one.** With the
contact closed, that is ~3.6 µA of standby current; the ATmega's internal
~40 kΩ pull-up would be ~110 µA, which is a large fraction of the entire budget
for the two-year target. `D5` is configured `INPUT`, never `INPUT_PULLUP`.

**The flow node is read twice, digitally and as a voltage.** `R3` 330 Ω to `A2`
is what makes a *partial* fault visible: water across the contacts or a corroded
pin presents a resistance rather than a short, and lands the node at mid-rail
where a digital read still returns a confident, wrong answer.

**100 Ω series resistors on every sensor line** (`R1`, `R2`, `R4`, `R5`, `R9`).
Among other things they limit a mis-wired harness to ~33 mA of contention rather
than letting two push-pull outputs fight at full current.

---

## 4. Things the firmware has to work around

### 4.1 The flow node's RC makes a normal transition look like a fault

`R6` 1 MΩ into `C2` 100 nF gives τ = 100 ms. The contact collapses the node in
microseconds when it closes, but when it **opens** the node has to climb back
through 1 MΩ:

| Threshold | Time |
|---|---|
| `V_IH` = 0.6·VCC | 0.92 τ = **92 ms** |
| 70 % ADC (open threshold) | 1.20 τ = **120 ms** |

For that ~120 ms the node genuinely sits at mid-rail — the exact signature of a
resistive fault. A single look would occasionally report a normal end-of-fill as
water in the connector.

The firmware separates them by persistence: a mid-rail result triggers one
re-read after 3 τ (`HN_FLOW_SETTLE_MS` = 300 ms), and only a second mid-rail
result is called a fault. A transient passes; a fault does not.

### 4.2 A 1 MΩ source is far outside the ADC's specified range

The ATmega328P's sample-and-hold is specified for a source impedance of 10 kΩ or
less. With the contact open the ADC looks into 1 MΩ. Two consequences:

- The 14 pF S/H capacitor needs ~140 µs to charge to within ten time constants
  (1 MΩ × 14 pF = 14 µs), and the first conversion after a channel change
  borrows that charge from the node. The driver therefore performs a throwaway conversion,
  waits `HN_FLOW_ADC_SETTLE_US` (500 µs), and only then takes the reading that
  counts.
- Pin leakage across 1 MΩ is an offset: at the datasheet's worst case of 1 µA
  that is a full volt. This is why the "open" threshold is 70 % of full scale
  rather than 90 %.

### 4.3 A two-wire dry contact cannot prove it is connected

An open flow switch and an unplugged flow switch are the same circuit. No
measurement separates them, so the firmware reports presence as
`UNCONFIRMED` rather than guessing. A *closed* contact, or a fault voltage, is
positive proof the harness is there.

**Hardware fix, if this matters:** fit an end-of-line resistor (e.g. 100 kΩ)
across the contact inside the switch housing. The node then sits at a third,
distinct voltage when the harness is present and the contact is open, and
`hn_classify_flow()` gains a band it can name. This is the only change that
would make flow-switch presence detection definitive.

---

## 4.4 Bench connection: powering and programming the board

The `J?` header carries the serial lines **and** the battery rail, so how you
connect it matters.

| J? pin | Signal | USB-serial adapter |
|---|---|---|
| 1 | DTR | DTR |
| 2 | TXO (the MCU transmits) | **RXD** |
| 3 | RXI (the MCU receives) | **TXD** |
| 4 | BATT+ | VCC — see the warning |
| 5, 6, 7 | GND (switched) | GND (any one) |

**The adapter must be set to 3.3 V.** A 5 V adapter puts 5 V directly onto the
battery rail and the MCU.

> **Never connect the adapter's VCC while the battery is fitted.** `J?.4` is the
> `BATT+` net, so adapter VCC back-feeds the LS14500 cells. Those are primary
> Li-SOCl₂ — non-rechargeable. Back-feeding them is a safety hazard, not just
> bad practice.

That leaves two valid arrangements:

**Bench powered.** Remove the battery and connect all five lines including VCC.
The adapter's VCC reaches the MCU's VCC and its ground reaches the switched
ground net, so the board runs from USB and the magnet latch is bypassed
entirely — `Q1` is irrelevant and no magnet is needed. This is the convenient
mode for development.

**Battery powered.** Fit the battery and **leave the adapter's VCC wire
disconnected** — only DTR, RXD, TXD and GND. Then bring the magnet to the reed
switch to turn the device on. This is how the product actually runs, and it is
the arrangement to use for any current measurement or endurance test.

Either way, the DTR line gives the Pro Mini its usual auto-reset, so programming
needs no button press. Note that resetting the MCU does **not** disturb the
latch: that independence is deliberate, so a crashed or reflashed Node stays
powered.

---

## 5. Assumptions to verify on the bench

The firmware is written so each of these is one line to change if it turns out
to be wrong. None of them should be taken on trust.

| # | Assumption | Why we believe it | How to check | If wrong |
|---|---|---|---|---|
| 1 | `J3.3` = ECHO, `J3.4` = TRIG | The ultrasonic harness is a **cross-over cable** — the RCWL-1670's own pads run GND / RX(TRIG) / TX(ECHO) / +5 V, so positions 2 and 4 swap. Confirmed during the Stage 0 review. Independently, ECHO on D8 is only meaningful if D8 is the input. | Continuity from each module pad to its `J3` pin before first power-up. | Swap `HN_PIN_US_TRIG` / `HN_PIN_US_ECHO` in `hn_board.h`. Note the driver needs ECHO on D8 for input capture, so a genuine swap means a harness rework, not a firmware change. |
| 2 | The flow switch is normally-open and **closes** on flow | Standard for an HT-60 class paddle switch. | Blow through the switch and watch `fl.d` in the serial output. | `HN_FLOW_FILLING_IS_LOW` in `hn_config.h`. |
| 3 | The RCWL-1670 drives ECHO **low** while idle | Normal for HC-SR04-compatible modules; it is what makes the pull-up presence probe work. | Scope the echo line with the module connected and idle. | Only the presence *probe* is affected. A real echo already overrides it, so the reading itself stays correct. |
| 4 | The DS18B20 measures the air the pulse travels through | It is the temperature that determines the speed of sound. | Note where the probe is physically mounted. | If the probe is in the **water**, the compensation is using a poor proxy for headspace air temperature. The review's HW-023 makes this the dominant error term in the whole measurement — see §6. |
| 5 | The module's blind zone is under 50 mm | Datasheet claims 2 cm. | Flat target, 2 cm outward in 5 mm steps, 20 readings each (review HW-051). | Mechanical: raise the sensor standoff. The firmware already reports this case as `NO ECHO` rather than as a fault. |

---

## 6. Why the Node transmits raw values

The Node performs **no interpretation**. It reports echo time in microseconds,
the DS18B20's raw register, and the flow switch's raw levels. Every conversion
lives on the Hub.

That is not laziness — three of the corrections cannot sensibly live here:

- **Split-transducer parallax.** The RCWL-1670's transmit and receive
  transducers are physically offset by roughly 30–50 mm, so the sound path is a
  triangle: `d = √((L/2)² − (s/2)²)`. At the 50 mm full-tank distance that is a
  systematic over-read of ~4 mm — largest exactly where the product cares most.
  `s` is a per-module build constant measured with callipers.
- **Humidity.** A tank headspace is essentially always saturated, which raises
  the speed of sound by 0.35–0.6 % over dry air.
- **Tank geometry and the volume curve**, which differ per installation.

Freezing any of those into a sealed rooftop device means a site visit to change
them. On the Hub they are a Wi-Fi update. The millimetre figure the serial
report prints is a bench convenience only, computed in `hn_report.cpp`, stored
nowhere, and never transmitted.

---

## 7. Reproducing the netlist

The extraction is geometric, and one detail matters: an Altium pin record's
`Location` is the **body-side** end of the pin, and the electrical end is
`Location + PinLength` along the pin's orientation. Testing both ends against
the wire segments produces false nets — it was what originally made `D3` appear
to be shorted to ground.

Connectivity rules used:

- wire segments sharing an endpoint are one net;
- an endpoint of one wire lying **on** another wire is a connection (Altium's
  automatic junction);
- wires that merely **cross** are only connected where an explicit junction
  object sits at the crossing;
- a pin joins a net when its electrical end lies on a wire segment, or abuts
  another pin's electrical end directly.

---

## 8. Schematic hygiene notes

Minor, and neither affects the firmware:

- A second, unplaced `S1` reed-switch symbol sits off-sheet at roughly
  (1070, −220) with both pins unconnected. It is a duplicate.
- The programming header `J?` has no numeric designator, and carries three
  ground pins (5, 6, 7) alongside DTR / TXO / RXI / BATT+.
