# Hydro Hub ↔ App cloud integration

How the Hydro Hub device links to the Bleu Land mobile app through Supabase.
Built to mirror the existing **Smart Switch** integration, plus water telemetry.

## Architecture

```
Hydro Hub (ESP32-S3)                 Supabase (Postgres)                 Mobile app (Expo)
  │  device_sync_hydro_hub  ───────▶  SECURITY DEFINER RPCs  ◀───────  tRPC server (service role)
  │  register / confirm_pair          hydro_hub_* tables                  │  hydroHub.* router
  │  (anon key, secret-authed)        + Realtime on devices  ──────────▶  Realtime live updates
```

- **Device → cloud:** the ESP32 calls PostgREST RPCs with the public **anon key**.
  Every call carries the device secret; the RPCs validate it (`SECURITY DEFINER`).
- **App → cloud:** the app calls the **tRPC server** (`hydroHub` router), which runs
  the user-facing RPCs with the **service-role** key and the user id taken from the
  authenticated session — never from client input.
- **Live data:** the app subscribes to Supabase Realtime on `hydro_hub_devices`
  (anon `SELECT` policy) so pump/telemetry changes appear instantly.

## Database objects (`public`)

| Table | Purpose |
|-------|---------|
| `hydro_hub_devices` | one row per hub: pump current/desired state + latest telemetry (level, volume, flow, filling_status, is_filling, eta) + tank config (height, blind area, per-tank volumes, capacity, config version, setup_completed_at) + meta (last_seen, pairing window, fw, rssi, reset_generation) |
| `hydro_hub_links` | device ↔ `App_Users.id` links (a paired account) |
| `hydro_hub_pair_requests` | short-lived pairing handshake rows |
| `hydro_hub_firmware_releases` | OTA releases: version, HTTPS url, force flag, is_active |

Device-facing RPCs (granted to **anon**, secret-authed):
`register_hydro_hub_device`, `device_sync_hydro_hub`, `confirm_hydro_hub_pair_request`.

App-facing RPCs (**service-role only** — called by the tRPC server):
`create_hydro_hub_pair_request`, `get_hydro_hub_pair_request_status`,
`set_hydro_hub_pump_state`, `set_hydro_hub_tank_config`, `list_user_hydro_hubs`,
`list_pairing_hydro_hubs`, `unlink_hydro_hub_for_user`.

## Device identity

- `device_id` = `HH-` + 12 hex of the eFuse MAC (stable, unique). Shown in the QR.
- `device_secret` = 32 random hex chars generated on first boot, stored in NVS
  (`hydrohub/secret`). The DB stores only its SHA-256 (`hash_hydro_hub_device_secret`).
- First boot self-registers (trust-on-first-use). For a hardened product, pre-provision
  device rows and revoke anon `EXECUTE` on `register_hydro_hub_device`.

## Setup + pairing flow (QR → BLE → WiFi → link)

The whole thing happens in-app — no hotspot switching. The device runs a BLE
provisioning service (`BleProvisioning.*`, matching the app's `lib/ble-provisioning.ts`,
service `4bf5a100-…`) while unprovisioned.

1. Device shows a QR encoding `…?hub=HH-XXXX`; app scans it (`app/pair-hydro-hub.tsx`).
2. App connects to the device over **BLE** (finds it by the `BleuLand-HH-…` name).
3. App asks the device to scan WiFi → shows the list → user picks SSID + password.
4. App sends the credentials over BLE → device joins that WiFi and registers with the cloud.
5. Device is now online + advertising pairing (`pairing_mode=true` while unpaired).
   App calls `hydroHub.createPairRequest` → 30 s pending request.
6. The device's next `device_sync` sees `pending_pair_request_id` and calls
   `confirm_hydro_hub_pair_request` → inserts the link. App polls until `confirmed`.
7. The app then routes to `app/tank-setup.tsx` (spec step 4): tank volume(s),
   tank height and blind area → `hydroHub.setTankConfig` →
   `set_hydro_hub_tank_config` stores them and stamps `setup_completed_at`.
8. The hub's next sync returns `setup_complete=true` + the `tank_config`
   payload; the hub saves it to NVS and switches from the QR screen to the
   dashboard. Until then it stays locked on the setup screen (spec §7.3).

After setup, a **short press** on the hub's WiFi button re-opens the QR screen
(and BLE) so more phones/accounts can link or the network can be changed. A
**5s long press** toggles WiFi fully off/on. Holding **both buttons 10s**
factory-resets: bumps `reset_generation` (expiring all links server-side),
wipes NVS + WiFi credentials, and reboots into setup mode.

## OTA updates

`device_sync_hydro_hub` returns `ota: {version, url, sha256, force}` when the
newest active row in `hydro_hub_firmware_releases` differs from the firmware
version the device reports. The hub streams the binary over HTTPS into the
inactive OTA slot (`partitions.csv` has two 3MB app slots) and reboots. To
publish: export the compiled `.bin`, host it on HTTPS (e.g. a public Supabase
Storage bucket), and insert a row with the new `version` + `firmware_url`.

BLE is torn down (`BleProvisioning::stop`) once WiFi connects, freeing RAM for the
dashboard sprites (BLE and the heavy sprites never run at the same time).
Requires a native app dev build (react-native-ble-plx) — same as the Smart Switch.

## Control & telemetry loop

- The device calls `device_sync_hydro_hub` every ~0.5–2 s (server returns an adaptive
  `poll_interval_ms`). It reports the current pump state + telemetry and receives
  `pump_desired_state`, which it applies to the relay via `setPumpState(..., Remote)`.
- The app sets the target with `hydroHub.setPumpState`; a local button press is reported
  with `local_override=true` so the cloud adopts the on-device state.
- An unpaired device is never remote-controlled (`desired` follows `current`).

## App files

| File | Role |
|------|------|
| `server/_core/hydroHubData.ts`, `hydroHubRouter.ts` | secure server API (registered in `server/routers.ts`) |
| `lib/hydro-hub-service.ts` | types, QR parsing, Realtime subscription, formatting |
| `app/pair-hydro-hub.tsx` | QR-scan pairing screen |
| `app/hydro-hub.tsx` | device list, live telemetry, pump on/off |
| `app/(tabs)/index.tsx` | Hydro Hub entry card on the home screen |

## Config

Firmware `config.h`: `SUPABASE_URL`, `SUPABASE_ANON_KEY` (public anon key — safe to embed).
App `.env`: `EXPO_PUBLIC_SUPABASE_URL`, `EXPO_PUBLIC_SUPABASE_ANON_KEY`, and the tRPC
server base URL. The service-role key lives only on the server.
