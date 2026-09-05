# Preparing a one-month field test

Read this before deploying. Some of it is a checklist; some of it is about what
a month-long test can and cannot tell you.

---

## 1. What a month actually proves

Worth settling up front, because the obvious expectation is wrong.

**It will not validate the two-year battery claim.** With sleep enabled the Node
averages roughly 100 µA. Over 720 hours that is about **72 mAh** — under 3% of a
single LS14500. Li‑SOCl₂ has a famously flat discharge curve, so at 3% depth of
discharge the voltage will barely move. You will finish the month with a battery
reading that is almost identical to the one you started with, and it will tell
you nothing about month 20.

**Measure the current directly instead.** A µA-capable meter in series with the
pack, for a few cycles on the bench, answers the power question properly and in
an afternoon:

| Phase | Expect |
|---|---|
| Sleeping (most of the 120 s) | 10–25 µA |
| Awake, measuring | ~4 mA for ~600 ms |
| Transmitting | ~100 mA for ~185 ms at SF9 |

If the sleeping figure is much above 25 µA, something is still powered that
should not be — that is the single most valuable measurement on this device.

**What the month *does* prove**, and none of it can be shortcut:

- Does it run for a month without hanging, resetting or drifting?
- What is the link like across a month of weather, at the real installed range?
- Do the readings stay sane, or does the transducer foul, the enclosure sweat,
  the cable chafe?
- Does anything get wet, hot, or eaten?

Those are the real unknowns, and they need calendar time.

---

## 2. Battery: read this before you power anything

**HW-003 is still an open BLOCKER.** The two LS14500 cells are wired directly in
parallel with no blocking diodes. Primary lithium cells must never be charged,
and two hard-paralleled cells push current into each other as they diverge.

The failure mode is **age-correlated** — it arrives at 18–24 months, when one
cell reaches its knee and the healthy one drives the full difference into it
through zero resistance. That is precisely why a month of running proves nothing
about it, and precisely why it must not be read as "we tested it, it was fine".

**For this test, run the Node on a single LS14500.**

One cell removes the hazard completely — there is no parallel path to be unsafe —
and costs you nothing: the month needs about 72 mAh of a 2600 mAh cell. It is
also a *better* test, because a single cell tells you what one cell actually
delivers.

Fix the isolation properly (ideal-diode controller per cell, or a low-Vf
Schottky each, plus a fuse in the pack lead) before any test long enough to
matter, and before production.

---

## 3. Node: deploying

### 3a. Calibrate the battery reading first

The internal bandgap is only specified to ±10%, so an uncalibrated reading is
±0.35 V — useless. Once per board, on the bench:

1. Build with `HN_PRODUCTION 0` so serial works. Read the `Battery: nnnn mV`
   line in the banner.
2. Measure the actual VCC pin with a meter.
3. `HN_BANDGAP_CAL = 1125300 × V_real ÷ V_reported`
4. Reflash, confirm the reported value now matches the meter.

Skip this and every battery number you collect for a month is noise.

### 3b. Flip one switch

In `hn_config.h`:

```c
#define HN_PRODUCTION  1
```

That is deliberately a single switch, because a field test needs three unrelated
things changed together and forgetting any one invalidates it:

| | Bench | Production |
|---|---|---|
| Sleep | off | **on** — ~4.5 µA instead of ~4 mA |
| Cycle | 5 s | **120 s** |
| Serial | on | **off** |

The third is the one people forget. The serial report is ~600 bytes, and at 9600
baud that is **625 ms of blocking transmission** every cycle — roughly as much
awake time as the measurement and the radio burst combined. Leaving it on
doubles the energy per cycle. You can see it in the build: production drops from
15150 bytes to 7202, because all the formatting code disappears.

### 3c. Confirm before it goes up a ladder

Flash the production build, then verify from the **Hub** (the Node has no serial
now):

- Packets still arrive, about every 2 minutes
- The battery figure appears in the header and matches your meter
- `LINK OK` persists across several cycles

Then leave it running on the bench for an hour and check the Hub's field log
shows ~30 packets and 0 missed before it goes anywhere.

### 3d. What protects it while you are not there

- **Watchdog.** If an awake cycle hangs for more than 8 s the chip resets. A
  normal cycle is under a second, so it only fires on a genuine fault — and a
  reboot beats a Node sitting flat on a roof. Watchdog resets are counted and
  reported at start-up.
- **A watchdog reset cannot brick it.** After a watchdog reset the AVR restarts
  with the watchdog still armed at its *shortest* timeout; if the sketch does
  not disable it within ~16 ms the chip resets forever. `hn_sleep.cpp` clears it
  in `.init3`, before the C runtime and before `main()`.
- **Reflashing still works while it sleeps.** DTR resets the chip into the
  bootloader regardless of sleep state.

---

## 4. Hub: deploying

### 4a. WiFi and OTA

In `config.h`:

```c
#define WIFI_ENABLED   1
#define WIFI_SSID      "your-wifi"
#define WIFI_PASSWORD  "your-password"
#define OTA_PASSWORD   "change-this"
```

**Change the OTA password.** Without one, anyone on the network can flash
arbitrary firmware onto a device wired to your tank.

Once it is on the network the Hub appears in the Arduino IDE under
`Tools → Port` as `hydro-hub at 192.168.x.x`. Upload as normal — no cable.

WiFi is entirely optional and non-blocking. If the router reboots, the Hub keeps
receiving and displaying; only OTA stops. At `WIFI_ENABLED 0` the network stack
is not even compiled in.

### 4b. The field log

Press **Button A** twice to reach the **FIELD** screen. It answers the question
the dashboard cannot: *did this work, and what did it cost*.

| Row | Why it matters |
|---|---|
| running | days, hours, **and boot count** — repeated boots mean something is resetting |
| packets received | reliability %, and how many were lost outright |
| **longest silence** | the single most useful number. An average hides a three-hour hole; the hole is what you design around |
| signal | RSSI range and worst SNR — margin, not just "it worked" |
| battery | first → last, and the drop |
| level / temp / flow | cycles where each sensor was not OK, and how many were no-echo |

Counters live in NVS and survive reboots and power cuts, saved about hourly.

Note the Hub also has a watchdog (30 s) and its own reboot counter — if the
Hub's boot count climbs, that is a finding in itself.

---

## 5. What is *not* ready, and is not a firmware problem

Being straight about it, since these decide whether a month outdoors is
informative or just destructive:

| | Status |
|---|---|
| **HW-003** paralleled cells | Open BLOCKER — run on one cell for the test |
| **HW-045** tank-wall penetration | Undefined. A hole in a water tank is a leak path both ways |
| **HW-027** enclosure over temperature | PETG-CF is the wrong material for a sunlit roof; ASA-CF and a sun shield |
| **HW-048** transducer fouling | Expected to develop over months — this test may be the first sign |
| Antenna mounting | Worth more dB than any PA setting. Get it vertical and clear of the tank body |

---

## 6. Weekly, and at the end

**Weekly, from the FIELD screen:** boot count still 1, reliability still high,
longest silence not growing, sensor fault counts flat.

**At the end, photograph the FIELD screen** and note:

1. Reliability % and longest silence
2. RSSI/SNR range → whether SF7 is viable (roughly doubles battery margin)
3. Battery start → end (expect little movement; see §1)
4. Sensor fault counts, especially level
5. Boot counts on both devices
6. Physical condition: condensation, corrosion, transducer film, cable chafe

Then the interesting questions become answerable: drop the spreading factor,
close out the enclosure and battery-isolation issues, and decide whether the
2-minute interval should become adaptive.
