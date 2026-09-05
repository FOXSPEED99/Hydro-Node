# Hydro Hub — Indoor Receiver & Controller

Firmware for the **Hydro Hub**, the indoor ESP32-S3 unit of the water system. It
receives tank data over LoRa from the sensor device, shows it on a 480x320 TFT
dashboard (animated pump + water-level gauge), syncs to Supabase, and controls
the pump relay from a button or a remote app command — with a confirmation buzzer
chime when the pump turns on.

## Folder Structure

```
Hydro Hub Device/
├── Hydro-Hub/            Arduino sketch (open this folder in the IDE)
│   ├── Hydro-Hub.ino     Coordinator: boot, tasks, pins, pump safety, WiFi portal, watchdog
│   ├── config.h          Pins, LoRa settings, pair ID, Supabase keys, task sizes, debug level
│   ├── SmartWaterTypes.h Shared telemetry / app-state structs
│   ├── DebugLog.h        Serial debug macros + heap/health logging
│   ├── DisplayUI.h/.cpp  TFT dashboard, gauge/pump animation, WiFi-setup QR screen
│   ├── LoRaProtocol.h/.cpp   LoRa JSON parse, pair-ID validation, ACK builder
│   ├── SupabaseClient.h/.cpp Supabase REST upload + remote pump-command polling
│   ├── ArabicLabels.h    Pre-rendered Arabic label glyph data
│   ├── partitions.csv    Custom 16MB partition table (3MB app slots)
│   └── data/             PNGs flashed to LittleFS (see data/README.md)
├── database/
│   └── supabase_schema.sql   Starter tables, indexes, trigger, RLS
├── tools/
│   └── generate_arabic.py    Regenerates ArabicLabels.h
└── docs/
    └── icon-system.md        Notes on the PNG/icon pipeline
```

## Arduino IDE Setup

1. Install Arduino IDE 2.x.
2. Install ESP32 board package by Espressif.
3. Select board close to your module, usually `ESP32S3 Dev Module`.
4. Recommended board settings:
   - USB CDC On Boot: `Enabled`
   - Flash Size: `16MB`
   - PSRAM: **`Disabled`** — this unit's module has no working PSRAM; firmware
     built with `OPI PSRAM` boot-loops with INT_WDT resets (learned the hard way)
   - CPU Frequency: `240MHz`
   - Upload Speed: `921600` first, reduce to `460800` if upload is unstable
5. Open `Hydro-Hub/Hydro-Hub.ino`.
6. Set **Partition Scheme: `Custom`** so the bundled `partitions.csv` (3MB app) is used.

## Required Libraries

Install from Arduino Library Manager where possible:

- `RadioLib`
- `TFT_eSPI`
- `FastLED`
- `ArduinoJson`
- `NimBLE-Arduino` (in-app BLE WiFi provisioning)
- QR library that provides `qrcode.h`, `QRCode`, `qrcode_initText`, `qrcode_getModule`

## TFT_eSPI Configuration

This project expects TFT pins to be configured inside the TFT_eSPI library setup file.

Use these TFT pins:

```cpp
#define TFT_CS    8
#define TFT_RST   9
#define TFT_DC    10
#define TFT_MOSI  11
#define TFT_SCLK  12
```

Use the correct driver for your 3.5 inch 480x320 module:

```cpp
#define ILI9488_DRIVER
// or, if your panel needs it:
// #define ILI9486_DRIVER
```

Set dimensions/rotation support:

```cpp
#define TFT_WIDTH  320
#define TFT_HEIGHT 480
```

The firmware sets landscape rotation using `TFT_ROTATION` in `config.h`.

## Production Configuration

Edit `config.h` before real field testing:

- `DEVICE_PAIR_ID`: must match Device 1.
- `DEVICE_ID`: unique ID for this indoor unit.
- `LORA_SYNC_WORD`: must match Device 1.
- `SUPABASE_URL`: your Supabase project URL.
- `SUPABASE_ANON_KEY`: anon key only, never service-role key.
- `SUPABASE_TLS_INSECURE_DEV_MODE`: keep `1` for bench testing, set `0` and add `SUPABASE_ROOT_CA` before production.
- `PROVISIONING_BASE_URL`: your mobile app deep link/setup URL.

## Expected LoRa Packet from the Sensor Node

Production format (spec §4.2 — the Sensor Node sends RAW data only; the hub
computes level %, volume, flow state and time-to-full from the tank
configuration entered in the app):

```json
{
  "pairId": "SWS-PAIR-0001",
  "node": "9F2C41A7",
  "seq": 1001,
  "distanceCm": 55.5,
  "flow": true,
  "tempC": 21.4,
  "bat": 3.58,
  "sens": 7
}
```

`node` is the Sensor Node's permanent EEPROM id (node firmware >= 1.1.0). The
hub binds to the first node it hears on a matching `pairId`, keeps that id in
NVS and **drops packets from any other node** — so a stray or duplicate node
can't inject readings. The app's device card shows the binding and can unlink
it; the next packet re-links, which is how you confirm the radio path is alive
right now instead of trusting a stale reading. Older nodes send no `node` and
are still accepted. A factory reset clears the binding too.

`sens` is the sensor **presence** mask (bit0 ultrasonic, bit1 DS18B20, bit2
flow) — whether each sensor is physically connected, tested before any
measurement is attempted. On the hub's screen a missing ultrasonic shows an
orange `--` where the percentage goes, and a missing probe a muted `-- C` under
it. A sensor that is connected but not reading keeps its bit set and simply
sends no value, so "unplugged" stays distinguishable from "faulty".

The legacy precomputed format is still accepted for bench testing:

```json
{
  "pairId": "SWS-PAIR-0001",
  "seq": 1001,
  "levelPct": 50,
  "volumeLiters": 500,
  "filling": true,
  "flowLpm": 30.5,
  "etaSeconds": 600,
  "pumpOn": false
}
```

The hub replies with ACK JSON:

```json
{
  "pairId": "SWS-PAIR-0001",
  "ack": 1001,
  "deviceId": "SWS-INDOOR-0001",
  "pumpOn": false
}
```

## Supabase Tables

Run `database/supabase_schema.sql` in the Supabase SQL editor to create the starter tables.

Important:

- RLS is enabled in the schema.
- Add proper app/user policies before shipping.
- Do not put a Supabase service-role key in firmware.
- For production TLS, set `SUPABASE_TLS_INSECURE_DEV_MODE` to `0` and provide the root CA in `SUPABASE_ROOT_CA`.

## Wireless Updates (OTA) — no USB needed

Once a hub runs firmware >= 1.1.0 and is on WiFi, you never need the USB cable
for updates again:

1. Make your code changes.
2. **Bump `FIRMWARE_VERSION` in `config.h`** (e.g. `1.1.1-hydrohub` -> `1.1.2-hydrohub`).
   The script refuses to re-release an existing version.
3. Run:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "Hydro Hub Device\tools\release-ota.ps1"
   ```
   It compiles, uploads the binary to the public Supabase `firmware` bucket and
   publishes a row in `hydro_hub_firmware_releases`.
4. Every online hub sees the release on its next sync (~2s), shows
   "Updating firmware..." on the TFT, flashes the inactive OTA slot
   (`partitions.csv` has two 3MB slots) and reboots into the new version.
   WiFi credentials, pairing, tank config and pump state all survive.

Rollback: set `is_active=false` on the bad release row — the previous active
release becomes the target again and hubs "update" back to it.

**Safety:** if an update crashes at boot, OTA can't recover it — only USB can.
The script never publishes a build that fails to compile, but test significant
changes on a bench unit before releasing to assembled devices.

## Upload Procedure

1. Connect ESP32-S3 by USB.
2. Confirm the TFT and LoRa wiring matches `config.h`.
3. Open Serial Monitor at `115200`.
4. Compile the sketch.
5. Upload.
6. Watch for:
   - `Smart Water System Device 2 booting`
   - `TFT Initialized`
   - `SX1278 Online`
   - `Setup complete`

## Bring-Up Test Checklist

Test in this order:

1. Boot with only ESP32 + TFT connected. Confirm color self-test and dashboard.
2. Add LoRa module. Confirm `SX1278 Online`.
3. Send one valid LoRa JSON packet from Device 1. Confirm dashboard updates and ACK log.
4. Press pump button. Confirm relay GPIO 17 changes, pump badge updates, NVS state saved.
5. Press pump button rapidly. The 1-second relay guard defers faster switches; the last press is remembered and applied once the guard clears (no press is lost).
6. Reboot after pump ON. Confirm boot starts relay OFF first, then restores stored state after guard delay.
7. Complete setup in the app (pair + tank data). Then short-press the WiFi button: QR screen appears (with BLE up for network changes / extra accounts) and returns after another press or 3 minutes.
8. Hold the WiFi button 5s on the main screen: WiFi turns fully off (LED off, no cloud traffic). Hold 5s again: it reconnects to the saved network. During setup the long press does nothing.
9. Provision WiFi from the app. Confirm WiFi LED solid ON when connected.
10. Confirm telemetry upload logs and remote pump control from the app.
11. Send a raw LoRa packet (`distanceCm` + `flow`). Confirm the hub computes level/volume/ETA using the tank config from the app.
12. Hold BOTH buttons 10s: factory reset — all data wiped, reboots into setup mode, old account links invalidated.

## Debugging Guide

Serial logs use this format:

```text
[ millis ] [level] TAG      Lline message
```

Important tags:

- `APP`: boot, setup, reset reason, heap.
- `TFT`: display initialization and screen changes.
- `LORA`: radio init, received packets, CRC errors, pair mismatch.
- `PUMP`: relay changes, button edges, debounce, safety guard.
- `WIFI`: reconnect attempts and provisioning portal.
- `CLOUD`: Supabase upload/polling status and HTTP errors.
- `HEALTH`: periodic WiFi/cloud/pump/LoRa/heap status.

Debug level is controlled in `config.h`:

```cpp
static constexpr uint8_t DEBUG_LOG_LEVEL = 3;
```

Use:

- `0` for production errors only.
- `2` for normal production info.
- `3` during development.
- `4` only when chasing noisy bugs.

## Safety Notes

- GPIO 17 is initialized LOW immediately on boot.
- Relay switching is blocked faster than once per second.
- Manual pump button has 50ms debounce.
- Pump state is saved to NVS and restored after boot safety delay.
- Supabase firmware must use anon key with Row Level Security policies, not service-role key.
