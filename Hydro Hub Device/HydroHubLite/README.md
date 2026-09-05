# Hydro Hub Lite

A focused receiver for the current stage: listen to the Hydro Node over LoRa, do
the tank maths, and put it on the 480×320 panel.

The full Hub in `../Hydro-Hub` keeps the cloud sync, the phone app, BLE
provisioning, OTA and the pump relay. **This is not a replacement for it** — it
is the same hardware running a much smaller job, so the Node↔Hub link and the
display can be brought up without the rest of the system in the way.

---

## The split with the Node

The Node sends **raw values only** — echo microseconds, the DS18B20's own
register, the flow switch's ADC count — and every conversion happens here.

That is not an arbitrary division. The blind zone, tank height, capacity,
speed-of-sound correction and transducer geometry all change per installation
and get revised as you learn things. On this device that is a reflash from the
kitchen table. On the Node it is a ladder, a roof and a sealed enclosure.

```
   Node  ──19 bytes over LoRa──▶  Hub
   echo_us, temp_raw, flow_adc,       speed of sound, parallax,
   per-sensor status                  tank geometry, litres, display
```

## Configuration

Everything an installer sets is at the top of `config.h`:

```c
#define TANK_COUNT              2
#define TANK_LITERS_LIST        { 1000, 500 }   // summed -> 1500 L
#define TANK_WATER_HEIGHT_CM    80              // water depth at 100%
#define TANK_BLIND_CM           10              // sensor -> full water line
```

The tanks are assumed **plumbed together**, so the water sits at the same level
in all of them: one Node measures one tank and that level applies to the set,
with the litres summed. Tanks of different sizes are fine — list them
individually. If they ever fill *independently*, this model breaks and each
tank needs its own Node.

```
        ultrasonic
            │  ▲
            │  │ TANK_BLIND_CM        reads this when 100% full
            ▼  ▼
        ~~~~~~~~~~~~  100%  ▲
                            │ TANK_WATER_HEIGHT_CM
        ____________    0%  ▼        reads blind+height when empty
```

`TANK_TRANSDUCER_SEP_MM` enables the split-transducer parallax correction —
measure the centre-to-centre spacing of the two barrels with callipers once.
It is worth ~4 mm at a 50 mm distance and nothing past 300 mm, so it matters
exactly when the tank is nearly full.

## Build

Arduino IDE 2.x, **ESP32S3 Dev Module**, PSRAM **Disabled** (as the full Hub's
README warns — this module boot-loops with OPI PSRAM enabled).

Libraries: **TFT_eSPI** and **RadioLib**. Nothing else. TFT pins go in the
TFT_eSPI `User_Setup`, same as the full Hub.

```
make test            # host tests for the tank maths and the wire format
make check-protocol  # verify the Node and Hub copies of hn_packet.* match
```

## The screen

```
┌────────────────────────────────────────────────────────┐
│ HYDRO HUB                        12s ago    [LINK OK]  │
├──────────┬─────────────────────────────────────────────┤
│          │  1125 L                                     │
│  ▓▓▓▓▓▓  │  of 1500 L in 2 tanks                       │
│  ▓▓▓▓▓▓  │  ┌─────────┬─────────┬─────────┐            │
│  ▓▓75%▓  │  │ LEVEL   │ DEPTH   │ TEMP    │            │
│  ▓▓▓▓▓▓  │  │ 75 %    │ 60 cm   │ 20.5 C  │            │
│  ▓▓▓▓▓▓  │  └─────────┴─────────┴─────────┘            │
│          │  [ Water is flowing in ]                    │
├──────────┴─────────────────────────────────────────────┤
│ OK LEVEL ok   OK TEMP ok   X FLOW MISSING              │
└────────────────────────────────────────────────────────┘
```

**Button A** toggles between this and a diagnostics screen showing the raw
values, RSSI/SNR, and packet/missed/rejected counters.

### The rule the design is built on

**The screen must never look confident about data it does not have.** A tank
monitor still showing a comfortable 80% because the sensor died an hour ago is
worse than one showing nothing — it is how somebody runs a pump dry.

So the water in the gauge carries *trust* as well as level:

| Appearance | Meaning |
|---|---|
| Solid blue | fresh reading, healthy sensor |
| Hatched | stale, or taken while the tank was filling and the surface was moving |
| Hollow, `--` | no usable reading — never a number, never a 0% |

And a status colour **never carries meaning alone**. Every sensor pill pairs
its colour with a glyph and a word (`X FLOW MISSING`), so the screen reads
correctly for a colour-blind viewer and at a glance across a dark room. The
banner says the same thing in plain language: *"Level sensor is not connected"*,
*"No echo — tank may be full, or surface unreadable"*, *"Filling now — level
settling, not exact"*.

The link state is its own thing, because *"the sensor is broken"* and *"the Node
has gone off the air"* are different problems that look identical if you only
track the last good reading. `LINK OK` / `OVERDUE` / `LINK LOST`, with a human
age (`12s ago`, `4m ago`) rather than a timestamp.

## Debugging

Every accepted packet is printed to serial as JSON at 115200 baud:

```json
{"node":1,"seq":42,"echoUs":2912,"tempRaw":328,"flowAdc":252,
 "st":{"level":0,"temp":0,"flow":0},"presence":{"level":1,"temp":1,"flow":2},
 "flowState":1,"gated":false,
 "calc":{"distanceCm":50.0,"levelPct":50.0,"liters":750,"valid":true},
 "rssi":-78,"snr":9.2}
```

Binary on the air is an energy decision, not a debuggability one — a bench
session looks exactly as it would have if the Node sent text. See
`hn_packet.h` for why: JSON at SF9 would put the Node about a year short of its
two-year battery target.

## Matching the Node

These must be identical at both ends or the radios simply never hear each
other, with no error on either side:

| Setting | Value | Node |
|---|---|---|
| Frequency | 433.0 MHz | `HN_LORA_FREQ_HZ` |
| Bandwidth | 125 kHz | `HN_LORA_BW_KHZ` |
| Spreading factor | 9 | `HN_LORA_SPREADING_FACTOR` |
| Coding rate | 4/5 | `HN_LORA_CODING_RATE` |
| Sync word | 0x42 | `HN_LORA_SYNC_WORD` |
| Preamble | 8 | `HN_LORA_PREAMBLE_LEN` |
| Pair id | `SWS-PAIR-0001` | `HN_PAIR_ID` |

`make check-protocol` covers the packet layout; the eight settings above are
still on you.

**SF9 is the bring-up setting, not the final one.** It has ~5 dB more margin
than SF7, which is what you want while first getting the two ends to hear each
other — but it costs 3× the airtime and takes the Node's battery margin down to
about 1.2×. Once the diagnostics screen is showing real RSSI and SNR, drop to
the lowest spreading factor that still has comfortable headroom and the battery
gets that back.
