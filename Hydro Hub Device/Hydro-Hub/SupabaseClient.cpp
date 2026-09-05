#include "SupabaseClient.h"

#include "DebugLog.h"
#include "config.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_system.h>
#include <string.h>

namespace {

char gDeviceId[24] = {0};   // "HH-XXXXXXXXXXXX"
char gSecret[33]   = {0};   // 32 hex chars + NUL
uint32_t gResetGeneration = 0;

void addHeaders(HTTPClient &http) {
  http.addHeader("apikey", SUPABASE_ANON_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_ANON_KEY);
  http.addHeader("Content-Type", "application/json");
}

// POST a JSON body to a PostgREST RPC and capture the response body.
bool callRpc(const char *fn, const String &payload, String *response) {
  if (WiFi.status() != WL_CONNECTED) {
    LOGW("CLOUD", "RPC %s skipped: WiFi down", fn);
    return false;
  }
  if (!SupabaseClient::configured()) {
    LOGW("CLOUD", "RPC %s skipped: Supabase not configured", fn);
    return false;
  }

  WiFiClientSecure client;
#if SUPABASE_TLS_INSECURE_DEV_MODE
  client.setInsecure();
#else
  if (strlen(SUPABASE_ROOT_CA) == 0) {
    LOGE("CLOUD", "Secure TLS requested but SUPABASE_ROOT_CA empty");
    return false;
  }
  client.setCACert(SUPABASE_ROOT_CA);
#endif

  String url = String(SUPABASE_URL) + "/rest/v1/rpc/" + fn;
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(client, url)) {
    LOGE("CLOUD", "HTTP begin failed for %s", fn);
    return false;
  }
  addHeaders(http);

  int code = http.POST(payload);
  String body = http.getString();
  if (response != nullptr) *response = body;
  http.end();

  bool ok = code >= 200 && code < 300;
  if (ok) {
    LOGD("CLOUD", "RPC %s ok code=%d", fn, code);
  } else {
    LOGW("CLOUD", "RPC %s failed code=%d body=%s", fn, code, body.c_str());
  }
  return ok;
}

// Map the boolean "filling" + flow rate to the four-state filling label.
// Thresholds live in config.h (FLOW_*_LPM_MAX) and match TankCalc.
const char *fillingStatus(const TelemetrySnapshot &s) {
  if (!s.filling) return "none";
  if (s.flowLpm < FLOW_WEAK_LPM_MAX) return "weak";
  if (s.flowLpm < FLOW_GOOD_LPM_MAX) return "good";
  return "strong";
}

}  // namespace

namespace SupabaseClient {

bool configured() {
#if !ENABLE_SUPABASE
  return false;
#else
  return String(SUPABASE_URL).startsWith("https://") &&
         String(SUPABASE_URL).indexOf("YOUR_PROJECT_REF") < 0 &&
         String(SUPABASE_ANON_KEY) != "YOUR_SUPABASE_ANON_KEY";
#endif
}

void beginIdentity() {
  if (gDeviceId[0] != 0) return;  // already initialised

  // Device id: "HH-" + 12 hex of the factory eFuse MAC (stable, unique, no WiFi needed).
  uint64_t mac = ESP.getEfuseMac();
  snprintf(gDeviceId, sizeof(gDeviceId), "HH-%04X%08X",
           (uint16_t)(mac >> 32), (uint32_t)mac);

  // Secret: 32 random hex chars, generated once and kept in NVS.
  Preferences prefs;
  prefs.begin("hydrohub", false);
  String stored = prefs.getString("secret", "");
  if (stored.length() == 32) {
    strncpy(gSecret, stored.c_str(), sizeof(gSecret) - 1);
  } else {
    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 32; i++) gSecret[i] = hex[esp_random() & 0x0F];
    gSecret[32] = 0;
    prefs.putString("secret", gSecret);
    LOGI("CLOUD", "Generated new device secret");
  }
  gResetGeneration = prefs.getUInt("resetGen", 0);
  prefs.end();
  LOGI("CLOUD", "Cloud identity id=%s gen=%lu", gDeviceId,
       static_cast<unsigned long>(gResetGeneration));
}

const char *deviceId() { return gDeviceId; }

uint32_t resetGeneration() { return gResetGeneration; }

void bumpResetGeneration() {
  // Survives the factory reset wipe (lives in the identity namespace, which we
  // keep) so the cloud can expire all old account links for this device.
  gResetGeneration++;
  Preferences prefs;
  prefs.begin("hydrohub", false);
  prefs.putUInt("resetGen", gResetGeneration);
  prefs.end();
  LOGI("CLOUD", "Reset generation bumped to %lu",
       static_cast<unsigned long>(gResetGeneration));
}

bool registerDevice() {
  if (gDeviceId[0] == 0) beginIdentity();
  StaticJsonDocument<256> doc;
  doc["p_device_id"] = gDeviceId;
  doc["p_device_secret"] = gSecret;
  doc["p_firmware_version"] = FIRMWARE_VERSION;
  String payload;
  serializeJson(doc, payload);
  bool ok = callRpc("register_hydro_hub_device", payload, nullptr);
  if (ok) LOGI("CLOUD", "Device registered/confirmed in cloud");
  return ok;
}

bool deviceSync(const TelemetrySnapshot &snapshot, bool pumpOn, bool pairingMode,
                bool localOverride, const char *sensorNodeId, uint32_t appliedUnlinkGen,
                SyncResult &out) {
  if (gDeviceId[0] == 0) beginIdentity();

  StaticJsonDocument<512> doc;
  doc["p_device_id"] = gDeviceId;
  doc["p_device_secret"] = gSecret;
  doc["p_pump_current_state"] = pumpOn;
  if (snapshot.valid) {
    doc["p_water_level_percent"] = snapshot.levelPercent;
    doc["p_volume_liters"] = (int)snapshot.volumeLiters;
    doc["p_flow_lpm"] = snapshot.flowLpm;
    doc["p_filling_status"] = fillingStatus(snapshot);
    doc["p_is_filling"] = snapshot.filling;
    doc["p_eta_seconds"] = (int)snapshot.etaSeconds;
    // Sensor Node extras (water temp / node battery / presence mask), when reported.
    if (snapshot.waterTempC > -100) doc["p_water_temp_c"] = snapshot.waterTempC;
    if (snapshot.nodeBatteryV > 0) doc["p_sensor_battery_v"] = snapshot.nodeBatteryV;
    if (snapshot.sensorMask != 0xFF) doc["p_sensor_status"] = snapshot.sensorMask;
  }
  doc["p_pairing_mode"] = pairingMode;
  doc["p_wifi_rssi"] = (int)WiFi.RSSI();
  String ssid = WiFi.SSID();
  if (ssid.length() > 0) doc["p_wifi_ssid"] = ssid;  // shown in the app profile
  doc["p_firmware_version"] = FIRMWARE_VERSION;
  doc["p_reset_generation"] = gResetGeneration;
  doc["p_local_override"] = localOverride;
  // The Sensor Node this hub is currently bound to ("" when unbound). The
  // server mirrors it for the app and clears its pending-unlink flag once it
  // sees the hub report an empty id.
  doc["p_sensor_node_id"] = sensorNodeId != nullptr ? sensorNodeId : "";
  doc["p_sensor_node_unlink_gen"] = appliedUnlinkGen;

  String payload, response;
  serializeJson(doc, payload);
  if (!callRpc("device_sync_hydro_hub", payload, &response)) {
    out.ok = false;
    return false;
  }

  StaticJsonDocument<1536> res;
  DeserializationError err = deserializeJson(res, response);
  if (err) {
    LOGW("CLOUD", "device_sync response parse failed: %s", err.c_str());
    out.ok = false;
    return false;
  }

  out.ok = true;
  out.pumpDesired    = res["pump_desired_state"] | false;
  out.isPaired       = res["is_paired"] | false;
  out.pairingActive  = res["pairing_mode_active"] | false;
  out.pollIntervalMs = res["poll_interval_ms"] | (uint32_t)CLOUD_SYNC_INTERVAL_MS;
  const char *pending = res["pending_pair_request_id"] | "";
  out.hasPendingPair = pending != nullptr && strlen(pending) > 0;
  if (out.hasPendingPair) {
    strncpy(out.pendingPairRequestId, pending, sizeof(out.pendingPairRequestId) - 1);
  } else {
    out.pendingPairRequestId[0] = 0;
  }

  out.setupComplete = res["setup_complete"] | false;
  // The app asked to forget the bound Sensor Node so it re-links on the next
  // packet. True only while the server's request counter is ahead of the one
  // this hub reported as applied.
  out.unlinkSensorNode = res["unlink_sensor_node"] | false;
  out.nodeUnlinkGen = res["sensor_node_unlink_gen"] | (uint32_t)0;

  // Tank configuration saved by the user in the app (spec §3.2 step 4).
  JsonVariantConst tank = res["tank_config"];
  if (!tank.isNull()) {
    out.hasTankConfig = true;
    out.tankConfig.valid = true;
    out.tankConfig.version = tank["version"] | (uint32_t)0;
    out.tankConfig.heightCm = tank["height_cm"] | (uint16_t)0;
    out.tankConfig.blindCm = tank["blind_cm"] | (uint16_t)0;
    out.tankConfig.capacityLiters = tank["capacity_l"] | (uint32_t)0;
    out.tankConfig.tankCount = tank["tank_count"] | (uint8_t)1;
  }

  // Pending OTA release (spec §9).
  JsonVariantConst ota = res["ota"];
  if (!ota.isNull()) {
    const char *v = ota["version"] | "";
    const char *u = ota["url"] | "";
    if (strlen(v) > 0 && strlen(u) > 0) {
      out.hasOta = true;
      out.otaForce = ota["force"] | false;
      strncpy(out.otaVersion, v, sizeof(out.otaVersion) - 1);
      strncpy(out.otaUrl, u, sizeof(out.otaUrl) - 1);
    }
  }
  return true;
}

bool confirmPair(const char *requestId, int resetGeneration) {
  if (gDeviceId[0] == 0) beginIdentity();
  if (requestId == nullptr || strlen(requestId) == 0) return false;

  StaticJsonDocument<256> doc;
  doc["p_device_id"] = gDeviceId;
  doc["p_device_secret"] = gSecret;
  doc["p_request_id"] = requestId;
  doc["p_reset_generation"] = resetGeneration;
  String payload;
  serializeJson(doc, payload);
  bool ok = callRpc("confirm_hydro_hub_pair_request", payload, nullptr);
  LOGI("CLOUD", "Confirm pair request %s -> %s", requestId, ok ? "OK" : "FAIL");
  return ok;
}

}  // namespace SupabaseClient
