#include "BleProvisioning.h"

#include "DebugLog.h"

#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

// UUIDs — must stay byte-for-byte identical to lib/ble-provisioning.ts in the app.
static const char *SVC_UUID       = "4bf5a100-0000-0000-0000-000000000001";
static const char *CH_DEVICE_INFO = "4bf5a101-0000-0000-0000-000000000001";
static const char *CH_WIFI_SSID   = "4bf5a102-0000-0000-0000-000000000001";
static const char *CH_WIFI_PASS   = "4bf5a103-0000-0000-0000-000000000001";
static const char *CH_WIFI_CMD    = "4bf5a104-0000-0000-0000-000000000001";
static const char *CH_WIFI_STATUS = "4bf5a105-0000-0000-0000-000000000001";
static const char *CH_WIFI_NETS   = "4bf5a106-0000-0000-0000-000000000001";

namespace {

bool gActive = false;
char gDeviceId[24] = {0};
char gFw[24] = {0};

NimBLECharacteristic *gStatusChar = nullptr;
NimBLECharacteristic *gNetsChar   = nullptr;

String gSsid;
String gPass;
volatile bool gPendingScan = false;
volatile bool gPendingConnect = false;

String toStr(const NimBLEAttValue &v) {
  return String((const char *)v.data(), v.length());
}

void notifyStatus(const String &json) {
  if (!gStatusChar) return;
  gStatusChar->setValue(json.c_str());
  gStatusChar->notify();
  LOGD("BLE", "status -> %s", json.c_str());
}

// ── Characteristic write callbacks ──
class SsidCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &) override { gSsid = toStr(c->getValue()); }
};
class PassCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &) override { gPass = toStr(c->getValue()); }
};
class CmdCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &) override {
    NimBLEAttValue v = c->getValue();
    if (v.length() < 1) return;
    uint8_t cmd = v.data()[0];
    if (cmd == 0x02) gPendingScan = true;       // scan WiFi
    else if (cmd == 0x01) gPendingConnect = true; // connect with stored ssid/pass
  }
};

SsidCb gSsidCb;
PassCb gPassCb;
CmdCb  gCmdCb;

void doScan() {
  LOGI("BLE", "WiFi scan requested");
  WiFi.mode(WIFI_STA);
  // Cancel any in-flight association first — scanNetworks() returns -2
  // (WIFI_SCAN_FAILED) if the STA is mid-connect (e.g. retrying stale creds).
  WiFi.disconnect(false, false);
  delay(100);
  int n = WiFi.scanNetworks(false, true);  // blocking, include hidden

  StaticJsonDocument<1024> doc;
  JsonArray arr = doc.createNestedArray("networks");
  int limit = n < 20 ? n : 20;
  for (int i = 0; i < limit; i++) {
    JsonObject o = arr.createNestedObject();
    o["ssid"]   = WiFi.SSID(i);
    o["rssi"]   = WiFi.RSSI(i);
    o["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
  }
  String out;
  serializeJson(doc, out);
  WiFi.scanDelete();

  if (gNetsChar) {
    gNetsChar->setValue(out.c_str());
    gNetsChar->notify();
  }
  notifyStatus("{\"status\":\"scan_complete\"}");
  LOGI("BLE", "WiFi scan done: %d networks", n);
}

void doConnect() {
  LOGI("BLE", "WiFi connect to '%s'", gSsid.c_str());
  notifyStatus("{\"status\":\"connecting\"}");

  WiFi.persistent(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(gSsid.c_str(), gPass.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(200);
    esp_task_wdt_reset();  // this runs in the cloud task (20s watchdog)
    if (WiFi.status() == WL_NO_SSID_AVAIL || WiFi.status() == WL_CONNECT_FAILED) break;
  }

  if (WiFi.status() == WL_CONNECTED) {
    StaticJsonDocument<256> doc;
    doc["status"] = "connected";
    doc["ip"] = WiFi.localIP().toString();
    doc["device_id"] = gDeviceId;
    String out; serializeJson(doc, out);
    notifyStatus(out);
    LOGI("BLE", "WiFi connected ip=%s", WiFi.localIP().toString().c_str());
  } else {
    const char *reason = "timeout";
    if (WiFi.status() == WL_NO_SSID_AVAIL) reason = "ssid_not_found";
    else if (WiFi.status() == WL_CONNECT_FAILED) reason = "wrong_password";
    StaticJsonDocument<128> doc;
    doc["status"] = "failed";
    doc["reason"] = reason;
    String out; serializeJson(doc, out);
    notifyStatus(out);
    LOGW("BLE", "WiFi connect failed: %s", reason);
  }
}

}  // namespace

namespace BleProvisioning {

void begin(const char *deviceId, const char *fwVersion) {
  if (gActive) return;
  strncpy(gDeviceId, deviceId ? deviceId : "", sizeof(gDeviceId) - 1);
  strncpy(gFw, fwVersion ? fwVersion : "", sizeof(gFw) - 1);

  String bleName = String("BleuLand-") + gDeviceId;  // app scans for the "BleuLand-" prefix
  NimBLEDevice::init(bleName.c_str());
  NimBLEDevice::setMTU(256);

  NimBLEServer *server = NimBLEDevice::createServer();
  NimBLEService *svc = server->createService(SVC_UUID);

  // Device info (READ)
  NimBLECharacteristic *info = svc->createCharacteristic(CH_DEVICE_INFO, NIMBLE_PROPERTY::READ);
  {
    StaticJsonDocument<192> doc;
    doc["device_id"] = gDeviceId;
    doc["fw"] = gFw;
    doc["status"] = (WiFi.SSID().length() > 0 || WiFi.status() == WL_CONNECTED) ? "has_wifi" : "needs_wifi";
    String out; serializeJson(doc, out);
    info->setValue(out.c_str());
  }

  svc->createCharacteristic(CH_WIFI_SSID, NIMBLE_PROPERTY::WRITE)->setCallbacks(&gSsidCb);
  svc->createCharacteristic(CH_WIFI_PASS, NIMBLE_PROPERTY::WRITE)->setCallbacks(&gPassCb);
  svc->createCharacteristic(CH_WIFI_CMD,  NIMBLE_PROPERTY::WRITE)->setCallbacks(&gCmdCb);
  gStatusChar = svc->createCharacteristic(CH_WIFI_STATUS, NIMBLE_PROPERTY::NOTIFY);
  gNetsChar   = svc->createCharacteristic(CH_WIFI_NETS, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  svc->start();

  // A 128-bit service UUID (16 B) + a 24-char name won't both fit in the 31-byte
  // advertisement, and the app finds the device by NAME — so keep the name in the
  // primary packet and push the UUID into the scan response.
  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->setName(bleName.c_str());
  adv->enableScanResponse(true);
  NimBLEAdvertisementData scanResp;
  scanResp.addServiceUUID(SVC_UUID);
  adv->setScanResponseData(scanResp);
  adv->start();

  gActive = true;
  LOGI("BLE", "Provisioning advertising as %s", bleName.c_str());
}

void loop() {
  if (!gActive) return;
  if (gPendingScan)    { gPendingScan = false;    doScan(); }
  if (gPendingConnect) { gPendingConnect = false; doConnect(); }
}

void stop() {
  if (!gActive) return;
  NimBLEDevice::deinit(true);
  gStatusChar = nullptr;
  gNetsChar = nullptr;
  gActive = false;
  LOGI("BLE", "Provisioning stopped (freed BLE)");
}

bool active() { return gActive; }

}  // namespace BleProvisioning
