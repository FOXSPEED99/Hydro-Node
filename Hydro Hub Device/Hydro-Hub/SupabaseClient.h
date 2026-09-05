#pragma once

#include "SmartWaterTypes.h"

#include <Arduino.h>

// Cloud link for the Hydro Hub. Talks to Supabase PostgREST RPC functions
// (device_sync_hydro_hub / register_hydro_hub_device / confirm_hydro_hub_pair_request)
// using the public anon key. The device authenticates every call with its own
// secret (MAC-derived id + a random secret kept in NVS).
namespace SupabaseClient {

bool configured();

// Derive the cloud device id from the chip and load/create the NVS secret.
// Safe to call once early in setup(); does not need WiFi.
void beginIdentity();
const char *deviceId();

// Factory-reset generation. Stored in NVS next to the device secret; bumping it
// (factory reset) invalidates all account links/pair requests server-side.
uint32_t resetGeneration();
void bumpResetGeneration();

// Ensure this device has a row in the cloud (trust-on-first-use). Idempotent.
bool registerDevice();

// Parsed result of a device_sync call.
struct SyncResult {
  bool     ok = false;
  bool     pumpDesired = false;   // desired pump state (from the app), apply this
  bool     isPaired = false;      // linked to at least one user account
  bool     pairingActive = false; // pairing window currently open
  bool     hasPendingPair = false;
  char     pendingPairRequestId[40] = {0};
  uint32_t pollIntervalMs = 2000; // adaptive cadence hint from the server
  // Setup / tank configuration (spec §3.2 step 4, saved from the app).
  bool     setupComplete = false; // paired + tank data saved at least once
  bool     hasTankConfig = false;
  TankConfig tankConfig;
  // The app asked the hub to forget its bound Sensor Node (diagnostic re-link).
  // nodeUnlinkGen is the server's request counter: persist it once applied and
  // report it back, so one request is consumed exactly once.
  bool     unlinkSensorNode = false;
  uint32_t nodeUnlinkGen = 0;
  // Pending OTA update (spec §9), when the server has a release that differs
  // from the running firmware version.
  bool     hasOta = false;
  bool     otaForce = false;
  char     otaVersion[32] = {0};
  char     otaUrl[224] = {0};
};

// Report telemetry + current pump state; receive desired pump state + pairing info.
// pairingMode=true asks the server to (keep) the pairing window open.
// localOverride=true tells the server the pump was just changed locally (button),
// so it adopts the current state as the desired state.
// sensorNodeId: the Sensor Node this hub is bound to, or "" when unbound.
// appliedUnlinkGen: the highest unlink request this hub has already carried out.
bool deviceSync(const TelemetrySnapshot &snapshot, bool pumpOn, bool pairingMode,
                bool localOverride, const char *sensorNodeId, uint32_t appliedUnlinkGen,
                SyncResult &out);

// Confirm a pending pair request seen in a SyncResult (links device -> user).
bool confirmPair(const char *requestId, int resetGeneration);

}  // namespace SupabaseClient
