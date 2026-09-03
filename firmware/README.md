# Hydro Node — Section 1 firmware

Sensor acquisition and connection detection for the Hydro Node: a
battery-powered, ultra-low-power LoRa water-level sensor that reads a tank and
reports **raw** values to a Hydro Hub.

This section covers the sensors only. LoRa, the sleep cycle, pairing and sound
feedback are later sections, and the code is arranged so they drop in without
reopening anything here.

**Target:** Arduino Pro Mini 3.3 V / 8 MHz (ATmega328P), regulator bypassed,
VCC fed directly from a 3.6 V lithium pack, ground switched by a magnet-operated
74HC74 latch. Wiring and its evidence: [`docs/HARDWARE.md`](docs/HARDWARE.md).

---

## Build and flash — Arduino IDE

The project is laid out as a normal Arduino sketch. There is nothing to
install beyond the IDE itself.

1. **Open the sketch.** `File > Open…` → `firmware/HydroNode/HydroNode.ino`.
   All the `.cpp` and `.h` files appear as tabs, including `hn_config.h`, which
   is where every tunable lives.

2. **Select the board.**

   | Menu | Setting |
   |---|---|
   | Tools → Board | Arduino AVR Boards → **Arduino Pro or Pro Mini** |
   | Tools → Processor | **ATmega328P (3.3V, 8 MHz)** |
   | Tools → Port | your USB-serial adapter |

   The Processor setting matters more than it looks. Choosing the 5 V / 16 MHz
   variant still compiles and still uploads, and then fails in a way that is
   genuinely hard to diagnose — every delay in the 1-Wire driver runs twice as
   long, so the temperature sensor stops answering, and the serial monitor
   shows only garbage because the UART divisor is halved. So the firmware
   refuses to build on the wrong clock and tells you which menu to fix.

3. **Verify** (✓). You should see roughly:

   ```
   Sketch uses 12194 bytes (39%) of program storage space.
   Global variables use 310 bytes (15%) of dynamic memory.
   ```

4. **Upload** (→). The bootloader runs at 57600 baud and DTR handles the reset,
   so no button press is needed.

5. **Serial Monitor** (`Ctrl+Shift+M`) at **9600 baud**.

Wiring the USB-serial adapter to the `J?` header, and the warning about never
connecting its VCC while the battery is fitted, are in
[`docs/HARDWARE.md`](docs/HARDWARE.md).

### Other build routes

Both use the same files — there is only one copy of the source.

**Make**, for CI or a shell workflow (this is what the firmware is regression
tested with):

```
sudo apt install gcc-avr avr-libc arduino-core-avr
make                    # build, print size
make test               # 43 host tests for the decision logic
```

**PlatformIO**, if you prefer it: `pio run`. `platformio.ini` points `src_dir`
at the sketch folder.

Current footprint: **12194 bytes flash (39 % of the Pro Mini's 30720), 310 B
SRAM (15 %)** — room for the radio, the sleep manager and the pairing state
machine.

No external libraries. The 1-Wire master and the DS18B20 driver are part of this
firmware on purpose: the fault detection Section 1 exists to provide lives in
the details a general-purpose library hides — whether the bus rose at all,
whether anything answered, whether the scratchpad's CRC held.

---

## What it does, once per cycle

1. Read the flow switch (digital **and** analogue), and record whether water is
   running in.
2. Start the DS18B20 conversion.
3. Fire the ultrasonic burst — five triggers, median-filtered with outlier
   rejection — while that conversion runs.
4. Collect the temperature.
5. Print a human block and a machine-readable line.

Steps 2–4 overlap on purpose. The conversion takes ~94 ms and the burst ~300 ms,
on completely separate hardware; running them sequentially would add 94 ms of
awake time to every cycle for nothing.

### Sample output

```
--- cycle 12  t=64213 ms ------------------------------------------
  Ultrasonic   OK  [connected]  echo 3402 us  spread 15 us  samples 5/5/5
      ~ 585.4 mm at 20.50 C  [diagnostic only, uncorrected]
  Temperature  OK  [connected]  raw 328 (20.50 C)  9-bit  CRC ok
  Flow switch  OK  [cannot tell]  idle  D5=1  A2=1009/1023  agree 5/5
#HN seq=12 up=64213 us.st=OK us.pr=YES us.echo=3402 us.spr=15 us.n=5,5,5 \
tp.st=OK tp.pr=YES tp.raw=328 tp.res=9 tp.crc=1 fl.st=OK fl.pr=MAYBE \
fl.state=IDLE fl.d=1 fl.adc=1009 gate=0
```

The `#HN` line is the shape the Section 2 LoRa payload will be packed from, so
the bench view and the wire format cannot drift apart unnoticed.

---

## How a sensor reports itself

Two questions are answered separately, because they are not the same question:

* **status** — is the *data* usable?
* **presence** — is the *sensor* physically there?

An ultrasonic sensor that is definitely connected can still return no echo (a
full tank inside the blind zone), and a flow switch reading "open" is
electrically identical to one that has been unplugged. Collapsing both into a
single `ok`/`error` flag is what makes a field device lie about itself.

| Sensor | How presence is established | Distinctive failures it can name |
|---|---|---|
| Ultrasonic | The internal pull-up is switched on for 200 µs and the echo line watched. Something driving it low means the module is there. A real echo overrides the probe — behaviour beats a static test. | line already high before triggering (stuck module, or TRIG/ECHO swapped); a burst that starts and never ends; **no echo from a healthy line**, reported as `NO ECHO` rather than a fault, because that is the full-tank case |
| DS18B20 | The 1-Wire presence pulse, plus a ROM read checked for family code `0x28` and CRC | bus never rises → data line shorted to ground; rises but nobody answers → not connected; scratchpad CRC failure; config byte not echoed back; the `0x0550` power-on value, which means the conversion never completed |
| Flow switch | A closed contact, or a fault voltage, proves the harness is there. A clean open proves nothing — and is reported as *cannot tell* | node at neither rail → water in the contacts, a corroded pin, a chafed cable; the digital and analogue views of the same net disagreeing → one of the two paths is damaged |

`OUT OF RANGE` is advisory: the value is real, it is still reported, it is simply
outside what this installation expects.

---

## Module map

Everything lives flat in `HydroNode/`, so every file is a tab in the IDE.

| File | Owns |
|---|---|
| `hn_board.*` | Pin map, safe idle states, ADC power, and the two timing hooks Section 3 replaces |
| `hn_config.h` | Every tunable, each with the reasoning for its value |
| `hn_ultrasonic.*` | Trigger, echo pulse timing, filtering handoff |
| `hn_temperature.*` | DS18B20 with GPIO power gating; split `start`/`finish` so the conversion can overlap |
| `hn_onewire.*` | 1-Wire master with a three-valued reset: presence / no-presence / shorted |
| `hn_flow.*` | Dual-path sampling and the settle-and-retry that keeps an RC transient from being called a fault |
| `hn_filter.*` | **All decision logic, hardware-free and unit-tested** |
| `hn_crc8.*` | Dallas CRC-8, unit-tested against the catalogue check value |
| `hn_reading.h` | The one struct the LoRa payload will be built from |
| `hn_report.*` | Serial output, compiled out entirely when `HN_SERIAL_ENABLED` is 0 |
| `hn_acquire.*` | Cycle sequencing — the seam Sections 2 and 3 plug into |

`hn_filter.cpp` exists as a separate translation unit for a specific reason. Sample
filtering and fault classification are the parts of this firmware most likely to
be wrong and least likely to be caught on a bench: a median that mishandles an
outlier and a chattering switch misreported as a wet connector both look
entirely plausible on a serial monitor. Keeping them free of hardware access
means `make test` can run the shipping source against known inputs — including
the ghost-echo rejection and the bouncing-contact case.

---

## Extending it

The seams are deliberate and small:

* **Sleep (Section 3).** `hn_delay_ms()` and `hn_idle_once()` in `hn_board.cpp`
  are the only places this firmware ever waits. Replace their bodies with a
  watchdog-timed power-down; nothing in the sensor layer changes. That is why
  no module calls `delay()` directly.
* **LoRa (Section 2).** Build with `HN_PARK_LORA_PINS 0` and take over D2, D9,
  D10 and the SPI pins; Section 1 currently parks them in a defined state and
  holds the SX1278 in reset. Pack the payload from `hn_reading_t` — the `#HN`
  line already shows the field set.
* **Production build.** `HN_SERIAL_ENABLED 0` removes the UART and roughly 6 kB
  of formatting code, and with it the awake time spent shifting characters out.

Three rules worth keeping:

1. **Never drive `A1`.** It reaches the power latch's active-low `CLR` through
   100 kΩ. Driving it low switches the device off, and only a magnet at the
   enclosure brings it back.
2. **Timer1 belongs to the ultrasonic driver.** Do not call `analogWrite()` on
   D9 or D10.
3. **No converted values in `hn_reading_t`.** The moment one appears there it
   will end up in the LoRa payload, and the correction constants will be frozen
   into a device on a roof. The Hub owns every conversion — see
   [`docs/HARDWARE.md` §6](docs/HARDWARE.md).

---

## Before first power-up

`docs/HARDWARE.md` §5 lists the five assumptions this firmware makes and how to
check each one. The two worth doing with a meter in hand:

* **Ultrasonic harness continuity.** It is a cross-over cable — the module's
  pads run GND / TRIG / ECHO / +5 V while `J3` runs GND / VCC / ECHO / TRIG.
  That is by design, but it is not marked on the board.
* **Flow switch polarity.** The firmware assumes normally-open, closing on flow.
