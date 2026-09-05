#include "config.h"

#include "DebugLog.h"
#include "DisplayUI.h"
#include "LoRaProtocol.h"
#include "OtaUpdater.h"
#include "SmartWaterTypes.h"
#include "SupabaseClient.h"
#include "BleProvisioning.h"
#include "TankCalc.h"

#include <Arduino.h>
#include <FastLED.h>
#include <Preferences.h>
#include <RadioLib.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>

#if ENABLE_TFT_DASHBOARD
#include <TFT_eSPI.h>
#endif

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

static constexpr const char *TAG_APP = "APP";
static constexpr const char *TAG_LORA = "LORA";
static constexpr const char *TAG_PUMP = "PUMP";
static constexpr const char *TAG_WIFI = "WIFI";
static constexpr const char *TAG_TASK = "TASK";

Preferences preferences;
SemaphoreHandle_t stateMutex = nullptr;
SemaphoreHandle_t tftMutex = nullptr;

SPIClass loraSPI(FSPI);  // FSPI=SPI2 — TFT_eSPI uses HSPI/SPI3 (via USE_HSPI_PORT in User_Setup.h), so LoRa gets FSPI as its own dedicated bus.
SX1278 radio = new Module(LORA_CS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, RADIOLIB_NC, loraSPI);

#if ENABLE_TFT_DASHBOARD
TFT_eSPI tft = TFT_eSPI();
#endif

CRGB rgbLeds[RGB_LED_COUNT];

TelemetrySnapshot telemetry;
AppState appState;

uint32_t lastRelaySwitchMs = 0;
// A pump request blocked by the min-switch guard is remembered here and applied
// once the guard clears, so a button press is never silently lost (guarded by
// stateMutex).
bool        pendingPumpValid   = false;
bool        pendingPumpDesired = false;
PumpSource  pendingPumpSource  = PumpSource::Button;

// Toggle-button debounce state. `armed` = ready to fire on the next press.
// (Declared here, above the first function, so the Arduino auto-generated
// prototype for buttonPressed() can see the type.)
struct ButtonDebounce {
  bool armed = true;
  uint32_t releaseStableMs = 0;
};
uint32_t lastCloudPushMs = 0;
uint32_t lastPumpPollMs = 0;
uint32_t lastWifiConnectAttemptMs = 0;
uint32_t lastLedUpdateMs = 0;
uint32_t lastHealthLogMs = 0;
bool wifiLedLevel = false;

// WiFi link diagnostics — written from the WiFi event task, read by the cloud
// task (32-bit volatiles, no lock needed). lastWifiEventMs tells us whether the
// stack is actively retrying (auto-reconnect fires a disconnect event every few
// seconds); wifiAuthFailures counts password-rejection disconnects so we can
// tell "stale credentials" apart from "router temporarily down".
volatile uint32_t lastWifiEventMs = 0;
volatile uint8_t wifiAuthFailures = 0;

TaskHandle_t loraTaskHandle = nullptr;
TaskHandle_t displayTaskHandle = nullptr;
TaskHandle_t cloudTaskHandle = nullptr;
TaskHandle_t buttonTaskHandle = nullptr;

volatile bool loraPacketReceived = false;

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void onLoRaPacketReceived() {
  loraPacketReceived = true;
}

String provisioningUrl() {
  // QR payload the mobile app scans to pair: a link carrying this hub's cloud id
  // (MAC-derived "HH-..."). The app reads the `hub` parameter.
  String url = PROVISIONING_BASE_URL;
  url += "?hub=";
  url += SupabaseClient::deviceId();
  return url;
}

void lockState() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
}

void unlockState() {
  xSemaphoreGive(stateMutex);
}

void copyState(TelemetrySnapshot &snapshot, AppState &state) {
  lockState();
  snapshot = telemetry;
  state = appState;
  unlockState();
}

void releaseSpiDevices() {
  digitalWrite(LORA_CS_PIN, HIGH);
#if ENABLE_TFT_DASHBOARD && defined(TFT_CS) && (TFT_CS >= 0)
  digitalWrite(TFT_CS, HIGH);
#endif
}

void setRgbStatus(const CRGB &color) {
  rgbLeds[0] = color;
  FastLED.show();
}

void addCurrentTaskToWatchdog() {
#if ENABLE_TASK_WATCHDOG
  esp_task_wdt_add(nullptr);
#endif
}

void resetWatchdog() {
#if ENABLE_TASK_WATCHDOG
  esp_task_wdt_reset();
#endif
}

void setupTaskWatchdog() {
#if ENABLE_TASK_WATCHDOG
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t wdtConfig = {
    .timeout_ms = TASK_WATCHDOG_TIMEOUT_S * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true,
  };
  esp_err_t result = esp_task_wdt_reconfigure(&wdtConfig);
  if (result == ESP_ERR_INVALID_STATE) {
    result = esp_task_wdt_init(&wdtConfig);
  }
  if (result == ESP_OK) {
    LOGI(TAG_APP, "Task watchdog configured timeout=%lus", static_cast<unsigned long>(TASK_WATCHDOG_TIMEOUT_S));
  } else {
    LOGW(TAG_APP, "Task watchdog config failed code=%d", result);
  }
#else
  esp_err_t result = esp_task_wdt_init(TASK_WATCHDOG_TIMEOUT_S, true);
  if (result == ESP_OK) {
    LOGI(TAG_APP, "Task watchdog configured timeout=%lus", static_cast<unsigned long>(TASK_WATCHDOG_TIMEOUT_S));
  } else {
    LOGW(TAG_APP, "Task watchdog config failed code=%d", result);
  }
#endif
#endif
}

void initializePumpSafeOff() {
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW);
  lastRelaySwitchMs = millis();
  LOGI(TAG_PUMP, "Boot safety default: relay OFF on GPIO %d", PUMP_RELAY_PIN);
}

// ~2.3s confirmation jingle for pump-ON: a gentle rising motif played twice, then
// a short ascending flourish. Long enough to notice, but with rests so it chimes
// rather than drones. tone() runs on the core's own internal task, so these calls
// just queue up and play back-to-back without blocking the caller. Works with a
// passive buzzer (plays the notes) and an active one (plays them as a rhythm; the
// {0,..} entries are silent rests). Edit the table to change the sound/length.
void playPumpOnChime() {
  struct Note { uint16_t freq; uint16_t ms; };   // freq 0 = rest
  static const Note melody[] = {
    {880, 150}, {1175, 150}, {1568, 250}, {0, 150},        // A5 D6 G6  .
    {880, 150}, {1175, 150}, {1568, 250}, {0, 150},        // A5 D6 G6  .
    {1047, 170}, {1319, 170}, {1568, 170}, {2093, 380},    // C6 E6 G6 C7
  };
  for (const Note &n : melody) {
    tone(BUZZER_PIN, n.freq, n.ms);
  }
}

bool setPumpState(bool enabled, PumpSource source) {
  bool changed = false;
  bool relayAllowed = false;
  bool attemptedChange = false;

  lockState();
  if (appState.pumpOn != enabled) {
    attemptedChange = true;
    uint32_t now = millis();
    relayAllowed = (now - lastRelaySwitchMs) >= RELAY_MIN_SWITCH_MS;
    if (relayAllowed) {
      digitalWrite(PUMP_RELAY_PIN, enabled ? HIGH : LOW);
      lastRelaySwitchMs = now;
      appState.pumpOn = enabled;
      appState.pumpStateChangedMs = now;
      appState.pumpDirtyForCloud = true;
      changed = true;
      pendingPumpValid = false;   // request satisfied
    } else {
      // Too soon after the last switch: remember the request instead of dropping
      // it. servicePendingPump() applies it as soon as the guard clears.
      pendingPumpValid   = true;
      pendingPumpDesired = enabled;
      pendingPumpSource  = source;
    }
  } else {
    pendingPumpValid = false;   // already in the desired state — cancel any pending
  }
  unlockState();

  if (changed) {
    preferences.putBool("pumpOn", enabled);
    LOGI(TAG_PUMP, "State=%s source=%u savedToNVS=1", enabled ? "ON" : "OFF", static_cast<uint8_t>(source));
    // Confirmation chime when the pump turns ON — but not on the silent boot
    // restore, so the device doesn't beep on every power-up.
    if (enabled && source != PumpSource::BootRestore) {
      playPumpOnChime();
    }
  } else if (attemptedChange && !relayAllowed) {
    LOGW(TAG_PUMP, "Blocked too-fast relay switch. Guard=%lums", static_cast<unsigned long>(RELAY_MIN_SWITCH_MS));
  } else {
    LOGD(TAG_PUMP, "No state change requested. State already %s", enabled ? "ON" : "OFF");
  }

  return changed;
}

// Apply a pump request that was blocked by the min-switch guard, once the guard
// has cleared. Call this periodically (e.g. from the button task) so a press made
// too soon after the last switch still takes effect instead of being lost.
void servicePendingPump() {
  bool valid, desired;
  PumpSource src;
  bool guardClear;
  lockState();
  valid      = pendingPumpValid;
  desired    = pendingPumpDesired;
  src        = pendingPumpSource;
  guardClear = (millis() - lastRelaySwitchMs) >= RELAY_MIN_SWITCH_MS;
  unlockState();
  if (valid && guardClear) {
    setPumpState(desired, src);   // guard is clear now, so this applies and clears pending
  }
}

void restorePumpStateFromNvs() {
  bool storedPumpOn = preferences.getBool("pumpOn", false);
  LOGI(TAG_PUMP, "NVS stored pump state=%s", storedPumpOn ? "ON" : "OFF");

  if (!storedPumpOn) {
    return;
  }

  uint32_t elapsed = millis() - lastRelaySwitchMs;
  if (elapsed < BOOT_RESTORE_DELAY_MS) {
    vTaskDelay(pdMS_TO_TICKS(BOOT_RESTORE_DELAY_MS - elapsed));
  }
  setPumpState(true, PumpSource::BootRestore);
}

// ── Sensor Node binding ───────────────────────────────────────────────────
// The hub locks onto the first node that presents the right pair id and keeps
// that node's permanent id in NVS. Packets from any other node on the same pair
// id are dropped and never ACKed, so a stray or duplicate node can't inject
// readings. The app clears the binding ("unlink sensor node"), after which the
// next packet re-links — which is also the way to prove the radio link works
// end to end right now instead of trusting stale data.
char linkedNodeId[12] = {0};
// Highest unlink request from the app this hub has carried out. Persisted so a
// reboot mid-handshake doesn't replay the request.
uint32_t appliedNodeUnlinkGen = 0;

void loadLinkedNode() {
  String stored = preferences.getString("nodeId", "");
  strlcpy(linkedNodeId, stored.c_str(), sizeof(linkedNodeId));
  appliedNodeUnlinkGen = preferences.getUInt("nodeUnlinkGen", 0);
  if (linkedNodeId[0]) {
    LOGI(TAG_LORA, "Sensor Node binding restored: %s", linkedNodeId);
  } else {
    LOGI(TAG_LORA, "No Sensor Node bound — will link to the next one heard");
  }
}

void applyNodeUnlink(uint32_t generation) {
  if (linkedNodeId[0]) {
    LOGI(TAG_LORA, "Sensor Node unlinked by the app (was %s)", linkedNodeId);
    linkedNodeId[0] = 0;
    preferences.remove("nodeId");
  } else {
    LOGI(TAG_LORA, "Sensor Node unlink requested; nothing was bound");
  }
  // Record the request as consumed even when nothing was bound, otherwise the
  // server keeps asking every sync.
  appliedNodeUnlinkGen = generation;
  preferences.putUInt("nodeUnlinkGen", appliedNodeUnlinkGen);
}

// True when this packet may be used. Binds on first sight.
bool acceptSensorNode(const char *nodeId) {
  // Node firmware < 1.1.0 sends no id — nothing to bind to, so accept it.
  if (nodeId == nullptr || nodeId[0] == 0) return true;

  if (linkedNodeId[0] == 0) {
    strlcpy(linkedNodeId, nodeId, sizeof(linkedNodeId));
    preferences.putString("nodeId", linkedNodeId);
    LOGI(TAG_LORA, "Sensor Node linked: %s", linkedNodeId);
    return true;
  }

  if (strcmp(linkedNodeId, nodeId) != 0) {
    LOGW(TAG_LORA, "Dropped packet from node %s (bound to %s)", nodeId, linkedNodeId);
    return false;
  }
  return true;
}

void sendLoRaAck(uint32_t sequence, bool pumpOn) {
  String ack = LoRaProtocol::buildAckPacket(sequence, pumpOn);
  releaseSpiDevices();
  int state = radio.transmit(ack);
  releaseSpiDevices();

  if (state == RADIOLIB_ERR_NONE) {
    LOGD(TAG_LORA, "ACK sent seq=%lu pump=%s", static_cast<unsigned long>(sequence), pumpOn ? "ON" : "OFF");
  } else {
    LOGW(TAG_LORA, "ACK transmit failed code=%d", state);
  }
}

// Short WiFi-button press after setup (spec §6.2): show the pairing QR again so
// more phones/accounts can link, and bring BLE provisioning up so the app can
// also change the WiFi network. Dismissed by another press or the timeout.
void showQrScreen() {
  lockState();
  bool alreadyShown = appState.screenMode == ScreenMode::WifiSetup;
  appState.screenMode = ScreenMode::WifiSetup;
  appState.qrManuallyOpened = true;
  appState.wifiSetupStartedMs = millis();
  unlockState();

  if (!alreadyShown) {
    LOGI(TAG_WIFI, "QR screen opened (button)");
  }
  if (!BleProvisioning::active()) {
    BleProvisioning::begin(SupabaseClient::deviceId(), FIRMWARE_VERSION);
  }
}

void dismissQrScreen() {
  lockState();
  appState.qrManuallyOpened = false;
  appState.screenMode = ScreenMode::Main;
  unlockState();

  // BLE was only up for the QR screen; free the RAM again if WiFi is fine.
  if (WiFi.status() == WL_CONNECTED && BleProvisioning::active()) {
    BleProvisioning::stop();
  }
  LOGI(TAG_WIFI, "QR screen dismissed");
}

// ── Tank configuration persistence (spec §3.2 step 6: saved on the device) ──

void saveTankConfigToNvs(const TankConfig &cfg) {
  preferences.putUInt("tankVer", cfg.version);
  preferences.putUShort("tankH", cfg.heightCm);
  preferences.putUShort("tankBlind", cfg.blindCm);
  preferences.putUInt("tankCap", cfg.capacityLiters);
  preferences.putUChar("tankCnt", cfg.tankCount);
  LOGI(TAG_APP, "Tank config v%lu saved to NVS", static_cast<unsigned long>(cfg.version));
}

TankConfig loadTankConfigFromNvs() {
  TankConfig cfg;
  cfg.version = preferences.getUInt("tankVer", 0);
  cfg.heightCm = preferences.getUShort("tankH", 0);
  cfg.blindCm = preferences.getUShort("tankBlind", 0);
  cfg.capacityLiters = preferences.getUInt("tankCap", 0);
  cfg.tankCount = preferences.getUChar("tankCnt", 1);
  cfg.valid = cfg.version > 0 && cfg.heightCm > 0 && cfg.capacityLiters > 0;
  return cfg;
}

// ── WiFi radio on/off (spec §6.2 long press, post-setup only) ──

void setWifiRadioEnabled(bool enabled) {
  lockState();
  bool wasOff = appState.wifiUserOff;
  appState.wifiUserOff = !enabled;
  if (!enabled) {
    appState.wifiConnected = false;
    appState.cloudOk = false;
    appState.cloudBusy = false;
    appState.wifiSSID[0] = '\0';
  }
  unlockState();

  if (!enabled) {
    if (BleProvisioning::active()) BleProvisioning::stop();
    WiFi.disconnect(true /*turn radio off*/, false /*keep credentials*/);
    WiFi.mode(WIFI_OFF);
    digitalWrite(WIFI_STATUS_LED_PIN, LOW);
    LOGI(TAG_WIFI, "WiFi turned OFF by long press — no internet activity until re-enabled");
  } else if (wasOff) {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    wifi_config_t staConf;
    bool hasCreds = (esp_wifi_get_config(WIFI_IF_STA, &staConf) == ESP_OK) &&
                    strlen(reinterpret_cast<const char *>(staConf.sta.ssid)) > 0;
    if (hasCreds) {
      WiFi.begin(reinterpret_cast<const char *>(staConf.sta.ssid),
                 reinterpret_cast<const char *>(staConf.sta.password));
      LOGI(TAG_WIFI, "WiFi turned back ON — reconnecting to saved network");
    } else {
      LOGI(TAG_WIFI, "WiFi turned back ON — no saved network");
    }
    lastWifiConnectAttemptMs = millis();
  }
}

// ── Factory reset (spec §6.3: both buttons held 10s) ──

void factoryReset() {
  LOGW(TAG_APP, "FACTORY RESET: erasing all saved data");
  // Invalidate every account link/pair request server-side on the next sync.
  SupabaseClient::bumpResetGeneration();
  // Wipe pump state, tank config, setup-done flag.
  preferences.clear();
  // Erase saved WiFi credentials.
  WiFi.disconnect(true /*radio off*/, true /*erase credentials*/);
  delay(200);
  ESP.restart();
}

void updateWifiLed() {
  uint32_t now = millis();

  lockState();
  AppState state = appState;
  unlockState();

  if (state.wifiConnected && !state.cloudBusy) {
    digitalWrite(WIFI_STATUS_LED_PIN, HIGH);
    return;
  }

  uint32_t interval = state.cloudBusy ? 120 : 700;
  if (now - lastLedUpdateMs >= interval) {
    lastLedUpdateMs = now;
    wifiLedLevel = !wifiLedLevel;
    digitalWrite(WIFI_STATUS_LED_PIN, wifiLedLevel ? HIGH : LOW);
  }
}

void logHealthIfDue() {
  if (millis() - lastHealthLogMs < DEBUG_HEALTH_LOG_INTERVAL_MS) {
    return;
  }
  lastHealthLogMs = millis();

  TelemetrySnapshot snapshot;
  AppState state;
  copyState(snapshot, state);

  LOGI("HEALTH", "wifi=%d cloud=%d pump=%d loraLost=%d packets=%lu rssi=%.0f heap=%lu psram=%lu",
       state.wifiConnected,
       state.cloudOk,
       state.pumpOn,
       snapshot.signalLost,
       static_cast<unsigned long>(snapshot.packetCount),
       snapshot.rssi,
       static_cast<unsigned long>(ESP.getFreeHeap()),
       static_cast<unsigned long>(ESP.getFreePsram()));
}

// Map filling + flow rate onto the dashboard's four fill states (spec §5.3;
// same thresholds as the cloud's filling_status).
PumpFillState fillStateFor(const TelemetrySnapshot &s) {
  if (!s.valid || !s.filling) return PumpFillState::None;
  if (s.flowLpm < FLOW_WEAK_LPM_MAX) return PumpFillState::Weak;
  if (s.flowLpm < FLOW_GOOD_LPM_MAX) return PumpFillState::Good;
  return PumpFillState::Strong;
}

bool startLoRaReceive() {
  releaseSpiDevices();
  int state = radio.startReceive();
  releaseSpiDevices();

  if (state != RADIOLIB_ERR_NONE) {
    LOGW(TAG_LORA, "RX start failed code=%d", state);
    return false;
  }

  return true;
}

void loraTask(void *parameter) {
  (void)parameter;
  addCurrentTaskToWatchdog();
  LOGI(TAG_TASK, "LoRa task started core=%d", xPortGetCoreID());

  loraPacketReceived = false;
  if (!startLoRaReceive()) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  while (true) {
    resetWatchdog();

    if (!loraPacketReceived) {
      lockState();
      if (millis() - telemetry.lastPacketMs > LORA_SIGNAL_LOST_MS) {
        telemetry.signalLost = true;
        strlcpy(telemetry.lastError, "Signal lost", sizeof(telemetry.lastError));
      }
      unlockState();
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    loraPacketReceived = false;

    String packet;
    releaseSpiDevices();
    int state = radio.readData(packet);
    releaseSpiDevices();

    if (state == RADIOLIB_ERR_NONE) {
      TelemetrySnapshot parsed;
      lockState();
      parsed = telemetry;
      unlockState();

      parsed.rssi = radio.getRSSI();
      parsed.snr = radio.getSNR();
      bool ok = LoRaProtocol::parseTelemetryPacket(packet, parsed);

      // Only the bound node may drive this hub. A foreign node is dropped
      // before it touches telemetry, and gets no ACK.
      if (ok && !acceptSensorNode(parsed.nodeId)) {
        loraPacketReceived = false;
        startLoRaReceive();
        vTaskDelay(pdMS_TO_TICKS(10));
        continue;
      }

      // Spec §5: the Sensor Node sends raw data; all math happens here. Under
      // the state lock because the cloud task can swap the tank config.
      if (ok && parsed.rawMode) {
        lockState();
        TankCalc::applyRawReading(parsed, parsed.distanceCm, parsed.flowSwitch, millis());
        unlockState();
      }

      lockState();
      parsed.packetCount = telemetry.packetCount + (ok ? 1 : 0);
      if (ok) {
        uint32_t now = millis();
        parsed.lastPacketMs = now;
        parsed.lastReadingMs = now;
      } else {
        parsed.lastPacketMs = telemetry.lastPacketMs;
      }
      telemetry = parsed;
      appState.telemetryDirtyForCloud = ok;
      bool pumpOn = appState.pumpOn;
      unlockState();

      if (ok) {
        LOGI(TAG_LORA, "RX ok seq=%lu rssi=%.1f snr=%.1f",
             static_cast<unsigned long>(parsed.sequence),
             parsed.rssi,
             parsed.snr);
#if ENABLE_TFT_DASHBOARD
        // Drive the dashboard gauge + pump-circle water from real telemetry.
        DisplayUI::setWaterLevel(parsed.levelPercent);
        DisplayUI::setPumpFillState(fillStateFor(parsed));
#endif
        sendLoRaAck(parsed.sequence, pumpOn);
        setRgbStatus(parsed.rssi > -100 ? CRGB::Green : CRGB::Yellow);
        vTaskDelay(pdMS_TO_TICKS(80));
        setRgbStatus(CRGB::Black);
      }
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      LOGW(TAG_LORA, "CRC mismatch");
    } else {
      LOGE(TAG_LORA, "RX failed code=%d", state);
      setRgbStatus(CRGB::Red);
      vTaskDelay(pdMS_TO_TICKS(250));
      setRgbStatus(CRGB::Black);
    }

    loraPacketReceived = false;
    startLoRaReceive();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void displayTask(void *parameter) {
  (void)parameter;
  addCurrentTaskToWatchdog();
  LOGI(TAG_TASK, "Display task started core=%d", xPortGetCoreID());

#if ENABLE_TFT_DASHBOARD
  // Seed with the opposite of the boot mode (WifiSetup) so the first loop detects
  // a "change" and paints the pairing screen once at startup.
  ScreenMode lastMode = ScreenMode::Main;
  bool firstMainDraw = true;
  TickType_t lastWake = xTaskGetTickCount();

  while (true) {
    resetWatchdog();

    TelemetrySnapshot snapshot;
    AppState state;
    copyState(snapshot, state);

    if (state.forceRedraw) {
      lockState();
      appState.forceRedraw = false;
      unlockState();
    }

    if (state.screenMode != lastMode || state.forceRedraw) {
      // ── MODE CHANGED (or repaint requested): full redraw ──
      xSemaphoreTake(tftMutex, portMAX_DELAY);
      releaseSpiDevices();
      if (state.screenMode == ScreenMode::WifiSetup) {
        DisplayUI::drawWifiSetup(tft, provisioningUrl());
        firstMainDraw = true;
      } else if (state.screenMode == ScreenMode::Ota) {
        DisplayUI::drawOtaScreen(tft, state.otaVersion);
        firstMainDraw = true;  // repaint the shell in full if the update fails
      } else {
        DisplayUI::drawMainShell(tft);
        DisplayUI::drawMain(tft, snapshot, state);
        firstMainDraw = false;
      }
      releaseSpiDevices();
      xSemaphoreGive(tftMutex);
      lastMode = state.screenMode;

    } else if (state.screenMode == ScreenMode::Main) {
      // ── SAME MODE: incremental update, only redraw what changed ──
      xSemaphoreTake(tftMutex, portMAX_DELAY);
      releaseSpiDevices();

      if (firstMainDraw) {
        DisplayUI::drawMainShell(tft);
        DisplayUI::drawMain(tft, snapshot, state);
        firstMainDraw = false;
      } else {
        DisplayUI::drawMainUpdateOnly(tft, snapshot, state);
      }

      releaseSpiDevices();
      xSemaphoreGive(tftMutex);
    }

    // Fixed-cadence pacing (delay accounts for how long the draw took, unlike a
    // plain vTaskDelay-after-work) so the fan advances in even steps. ~83fps while
    // the fan spins — the diff-based fan push only costs a few ms per frame, and a
    // high frame rate keeps the per-frame angle step small (less visible stepping
    // and tearing at high RPM); a light 10fps check rate otherwise. The display
    // task is alone-ish on core 1, so the faster rate won't starve the
    // WiFi/LoRa/cloud tasks on core 0.
    bool animating = (state.screenMode == ScreenMode::Main && DisplayUI::isPumpAnimating());
    TickType_t period = pdMS_TO_TICKS(animating ? 12 : 100);
    // If we've fallen far behind (e.g. after a heavy full redraw), resync so we
    // don't busy-spin trying to catch up.
    if ((xTaskGetTickCount() - lastWake) > (period * 3)) lastWake = xTaskGetTickCount();
    vTaskDelayUntil(&lastWake, period);
  }
#else
  while (true) {
    resetWatchdog();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
#endif
}

void cloudTask(void *parameter) {
  (void)parameter;
  addCurrentTaskToWatchdog();
  LOGI(TAG_TASK, "Cloud task started core=%d configured=%d", xPortGetCoreID(), SupabaseClient::configured());

  while (true) {
    resetWatchdog();

    lockState();
    bool wifiUserOff = appState.wifiUserOff;
    bool qrManuallyOpened = appState.qrManuallyOpened;
    uint32_t setupStartedMs = appState.wifiSetupStartedMs;
    unlockState();

    // WiFi disabled by the user (5s long press, spec §6.2): no reconnects, no
    // BLE, no cloud traffic — just keep servicing the LED/health logging.
    if (wifiUserOff) {
      lockState();
      appState.wifiConnected = false;
      appState.cloudOk = false;
      appState.cloudBusy = false;
      unlockState();
      digitalWrite(WIFI_STATUS_LED_PIN, LOW);
      logHealthIfDue();
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    // Auto-dismiss a *manually opened* QR screen after the timeout. The
    // pairing/QR screen shown while setup is incomplete must persist (this task
    // returns to Main on its own once the hub is linked + configured).
    if (qrManuallyOpened && millis() - setupStartedMs > WIFI_SETUP_SCREEN_TIMEOUT_MS) {
      LOGI(TAG_WIFI, "QR screen timeout reached");
      dismissQrScreen();
    }

    // Retry on a SLOW cadence. A WiFi association + DHCP can take 5-10s; calling
    // reconnect() too often (we used to do it every 5s) aborts the in-progress
    // attempt before it can finish, so it never connects. Also hold off entirely
    // while BLE provisioning is up — reconnect() would keep the radio "connecting"
    // and make the app's WiFi scan fail.
    if (WiFi.status() != WL_CONNECTED && !BleProvisioning::active() &&
        millis() - lastWifiConnectAttemptMs > WIFI_CONNECT_CHECK_MS) {
      lastWifiConnectAttemptMs = millis();
      LOGI(TAG_WIFI, "Reconnect attempt");
      WiFi.reconnect();
    }

    // We skip BLE at boot when saved creds exist (so the radios don't fight while
    // reconnecting). But if WiFi still hasn't come up after a grace period, those
    // creds are likely stale — bring BLE up so the app can re-provision without
    // the user ever touching WiFi settings.
    static uint32_t cloudTaskStartMs = millis();
    if (WiFi.status() != WL_CONNECTED && !BleProvisioning::active() &&
        millis() - cloudTaskStartMs > WIFI_BLE_FALLBACK_MS) {
      LOGW(TAG_WIFI, "No WiFi after grace period -> starting BLE provisioning fallback");
      BleProvisioning::begin(SupabaseClient::deviceId(), FIRMWARE_VERSION);
    }

    // Service in-app BLE provisioning (WiFi scan/connect requests from the app),
    // and tear BLE down once WiFi is up to free RAM for the dashboard — unless
    // the QR screen holds it open for a network change / extra account link.
    BleProvisioning::loop();

    bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && BleProvisioning::active()) {
      // Re-read the flag right before stopping: the button task may have just
      // opened the QR screen (which needs BLE) after this loop's snapshot.
      lockState();
      bool qrOpenNow = appState.qrManuallyOpened;
      unlockState();
      if (!qrOpenNow) {
        BleProvisioning::stop();
      }
    }
    lockState();
    if (connected && !appState.wifiConnected) {
      String ssid = WiFi.SSID();
      strncpy(appState.wifiSSID, ssid.c_str(), sizeof(appState.wifiSSID) - 1);
      appState.wifiSSID[sizeof(appState.wifiSSID) - 1] = '\0';
    } else if (!connected) {
      appState.wifiSSID[0] = '\0';
    }
    appState.wifiConnected = connected;
    unlockState();

    static bool     cloudRegistered = false;
    static uint32_t nextCloudSyncMs = 0;
    static bool     lastKnownPaired = false;

    if (WiFi.status() == WL_CONNECTED && SupabaseClient::configured()) {
      // Ensure the device has a cloud row (trust-on-first-use); retry until it sticks.
      if (!cloudRegistered) {
        cloudRegistered = SupabaseClient::registerDevice();
      }

      // Sync on the server-suggested cadence (adaptive: faster while pairing or
      // when a pump command is pending, slower when idle).
      if (cloudRegistered && (int32_t)(millis() - nextCloudSyncMs) >= 0) {
        TelemetrySnapshot snapshot;
        bool pumpOn, localOverride;
        lockState();
        snapshot = telemetry;
        pumpOn = appState.pumpOn;
        localOverride = appState.pumpDirtyForCloud;  // pump was changed locally
        appState.cloudBusy = true;
        unlockState();

        // Advertise pairing mode until the device is linked to an account, and
        // whenever the QR screen was opened on purpose (extra phones/accounts).
        bool pairingMode = !lastKnownPaired || qrManuallyOpened;

        SupabaseClient::SyncResult sync;
        bool ok = SupabaseClient::deviceSync(snapshot, pumpOn, pairingMode, localOverride,
                                             linkedNodeId, appliedNodeUnlinkGen, sync);

        if (ok) {
          lastKnownPaired = sync.isPaired;

          // "Unlink sensor node" from the app: forget the binding so the next
          // packet re-links, proving the radio path end to end.
          if (sync.unlinkSensorNode) {
            applyNodeUnlink(sync.nodeUnlinkGen);
          }

          // Configuration lock (spec §7.3): stay on the setup/QR screen until
          // the hub is BOTH linked to an account AND the tank data is saved.
          // Don't fight a manually opened QR screen (button / timeout owns it).
          bool setupDone = sync.isPaired && sync.setupComplete;
          bool setupDoneChanged;
          lockState();
          setupDoneChanged = appState.setupDone != setupDone;
          appState.setupDone = setupDone;
          if (!appState.qrManuallyOpened) {
            appState.screenMode = setupDone ? ScreenMode::Main : ScreenMode::WifiSetup;
          }
          unlockState();
          if (setupDoneChanged) {
            preferences.putBool("setupDone", setupDone);
            LOGI(TAG_APP, "Setup %s (paired=%d tankConfig=%d)",
                 setupDone ? "COMPLETE" : "incomplete", sync.isPaired, sync.setupComplete);
          }

          // Tank data entered/updated in the app -> adopt + persist locally
          // (spec §3.2 step 6: saved to the account AND the device).
          if (sync.hasTankConfig && sync.tankConfig.version != TankCalc::config().version) {
            lockState();  // the LoRa task runs TankCalc under the same lock
            TankCalc::setConfig(sync.tankConfig);
            unlockState();
            saveTankConfigToNvs(sync.tankConfig);
          }

          // A user scanned the QR and asked to pair -> confirm from the device.
          if (sync.hasPendingPair) {
            SupabaseClient::confirmPair(sync.pendingPairRequestId,
                                        SupabaseClient::resetGeneration());
          }

          // Apply the app's desired pump state (only once linked to an account).
          if (sync.isPaired && sync.pumpDesired != pumpOn) {
            setPumpState(sync.pumpDesired, PumpSource::Remote);
          }

          // This sync reported the local change; clear the override flag.
          if (localOverride) {
            lockState();
            appState.pumpDirtyForCloud = false;
            unlockState();
          }

          nextCloudSyncMs = millis() + sync.pollIntervalMs;

          // Remote firmware update (spec §9). Runs inside this task: telemetry
          // pauses during the download, then the device reboots into the new
          // firmware and resumes on its saved WiFi + setup state.
          if (sync.hasOta && OtaUpdater::shouldUpdate(sync.otaVersion)) {
            // Hand the screen to the display task: it owns the panel, so drawing
            // the OTA screen from here only gets painted over by the next
            // incremental main-screen update (leftover gauge/ETA on top of it).
            lockState();
            ScreenMode modeBeforeOta = appState.screenMode;
            strncpy(appState.otaVersion, sync.otaVersion, sizeof(appState.otaVersion) - 1);
            appState.otaVersion[sizeof(appState.otaVersion) - 1] = 0;
            appState.screenMode = ScreenMode::Ota;
            unlockState();
#if ENABLE_TFT_DASHBOARD
            // Give the display task a frame to paint before the download hogs
            // the CPU, so the screen is up even on a fast update.
            delay(150);
#endif
            if (!OtaUpdater::applyUpdate(sync.otaUrl, sync.otaVersion)) {
              LOGE(TAG_APP, "OTA update failed; continuing on %s", FIRMWARE_VERSION);
              lockState();
              appState.screenMode = modeBeforeOta;
              appState.forceRedraw = true;  // repaint over the OTA screen
              unlockState();
            }
          }
        } else {
          nextCloudSyncMs = millis() + CLOUD_SYNC_INTERVAL_MS;
        }

        lockState();
        appState.cloudOk = ok;
        appState.cloudBusy = false;
        unlockState();
      }
    } else {
      cloudRegistered = false;
      lockState();
      appState.cloudOk = false;
      appState.cloudBusy = false;
      unlockState();
    }

    updateWifiLed();
    logHealthIfDue();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void handlePumpButtonPress() {
  bool current;
  lockState();
  current = appState.pumpOn;
  unlockState();
  LOGI(TAG_PUMP, "Manual button pressed");
  setPumpState(!current, PumpSource::Button);
}

// Short press (spec §6.2): main screen only — toggle the QR screen so more
// phones/accounts can link or the WiFi network can be changed from the app.
// Ignored during setup: the configuration lock (§7.3) owns the screen.
void handleWifiShortPress() {
  bool setupDone;
  ScreenMode mode;
  lockState();
  setupDone = appState.setupDone;
  mode = appState.screenMode;
  unlockState();

  LOGI(TAG_WIFI, "WiFi button short press");
  if (!setupDone) {
    LOGI(TAG_WIFI, "Ignored: setup still in progress");
    return;
  }
  if (mode == ScreenMode::Ota) {
    LOGI(TAG_WIFI, "Ignored: firmware update in progress");
    return;
  }
  if (mode == ScreenMode::WifiSetup) {
    dismissQrScreen();
  } else {
    showQrScreen();
  }
}

// 5s long press (spec §6.2): after setup, toggles the WiFi radio fully off/on.
// During setup it has no effect — WiFi must stay available for provisioning.
void handleWifiLongPress() {
  bool setupDone, off;
  lockState();
  setupDone = appState.setupDone;
  off = appState.wifiUserOff;
  unlockState();

  LOGI(TAG_WIFI, "WiFi button long press");
  if (!setupDone) {
    LOGI(TAG_WIFI, "No effect during setup: WiFi stays on");
    return;
  }
  setWifiRadioEnabled(off);  // toggle
}

// Returns true exactly once per physical press: it fires on the first contact
// (so quick taps register), then disarms and won't fire again until the button
// has read released continuously for BUTTON_DEBOUNCE_MS. That release-must-settle
// rule means bounce — even a long, messy release bounce — can never sneak in a
// second toggle, which was making presses look like they "did nothing".
static bool buttonPressed(ButtonDebounce &b, int pin, int pressedLevel, uint32_t now) {
  bool raw = (digitalRead(pin) == pressedLevel);
  if (b.armed) {
    if (raw) {                       // press edge
      b.armed = false;
      b.releaseStableMs = 0;
      return true;
    }
  } else if (!raw) {                 // released — must stay released to re-arm
    if (b.releaseStableMs == 0) b.releaseStableMs = now;
    else if (now - b.releaseStableMs >= BUTTON_DEBOUNCE_MS) b.armed = true;
  } else {
    b.releaseStableMs = 0;           // still pressed / bounced low — reset the timer
  }
  return false;
}

void buttonTask(void *parameter) {
  (void)parameter;
  addCurrentTaskToWatchdog();
  LOGI(TAG_TASK, "Button task started core=%d", xPortGetCoreID());

  const int PRESSED_LEVEL = BUTTON_ACTIVE_LOW ? LOW : HIGH;
  ButtonDebounce pumpBtn;

  // WiFi button gesture tracking: short press fires on release, long press
  // fires at the 5s mark while still held. The pump button always toggles on
  // its press edge (spec §6.1: it must work at ALL times).
  bool     wifiHeld = false;
  bool     wifiLongFired = false;
  bool     wifiComboSeen = false;   // both buttons were down during this press
  uint32_t wifiPressStartMs = 0;

  // Factory reset combo (spec §6.3): both buttons held for 10s.
  uint32_t comboStartMs = 0;

  while (true) {
    resetWatchdog();
    uint32_t now = millis();

    bool pumpRaw = digitalRead(PUMP_BUTTON_PIN) == PRESSED_LEVEL;
    bool wifiRaw = digitalRead(WIFI_BUTTON_PIN) == PRESSED_LEVEL;

    if (buttonPressed(pumpBtn, PUMP_BUTTON_PIN, PRESSED_LEVEL, now)) {
      LOGD(TAG_PUMP, "Button PRESS");
      handlePumpButtonPress();
    }

    // ── WiFi button: short vs 5s long press ──
    if (wifiRaw && !wifiHeld) {
      wifiHeld = true;
      wifiLongFired = false;
      wifiComboSeen = pumpRaw;
      wifiPressStartMs = now;
    } else if (wifiRaw && wifiHeld) {
      if (pumpRaw) wifiComboSeen = true;
      if (!wifiLongFired && !wifiComboSeen && now - wifiPressStartMs >= WIFI_LONG_PRESS_MS) {
        wifiLongFired = true;
        handleWifiLongPress();
      }
    } else if (!wifiRaw && wifiHeld) {
      wifiHeld = false;
      uint32_t heldMs = now - wifiPressStartMs;
      // >=30ms filters release bounce; combo presses never count as a short press.
      if (!wifiLongFired && !wifiComboSeen && heldMs >= 30 && heldMs < WIFI_LONG_PRESS_MS) {
        handleWifiShortPress();
      }
    }

    // ── Factory reset: both buttons held for 10s ──
    if (pumpRaw && wifiRaw) {
      if (comboStartMs == 0) {
        comboStartMs = now;
      } else if (now - comboStartMs >= FACTORY_RESET_HOLD_MS) {
        factoryReset();  // does not return
      }
    } else {
      comboStartMs = 0;
    }

    // Apply any pump press that was blocked by the min-switch guard, now that
    // enough time has passed — so no button press is ever lost.
    servicePendingPump();

    vTaskDelay(pdMS_TO_TICKS(5));   // poll fast for crisp response
  }
}

void setupPins() {
  pinMode(LORA_CS_PIN, OUTPUT);
  pinMode(WIFI_STATUS_LED_PIN, OUTPUT);
  pinMode(PUMP_BUTTON_PIN, BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
  pinMode(WIFI_BUTTON_PIN, BUTTON_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);

  // Keep the buzzer quiet until we intentionally play a tone (tone() re-attaches
  // this pin to LEDC on first use).
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

#if ENABLE_TFT_DASHBOARD && defined(TFT_CS) && (TFT_CS >= 0)
  pinMode(TFT_CS, OUTPUT);
#endif

  digitalWrite(WIFI_STATUS_LED_PIN, LOW);
  releaseSpiDevices();
  LOGI(TAG_APP, "Pins ready. LoRaCS=%d PumpRelay=%d PumpBtn=%d WiFiBtn=%d WiFiLED=%d",
       LORA_CS_PIN,
       PUMP_RELAY_PIN,
       PUMP_BUTTON_PIN,
       WIFI_BUTTON_PIN,
       WIFI_STATUS_LED_PIN);
}

void setupTft() {
#if ENABLE_TFT_DASHBOARD
  bool setupDone;
  lockState();
  setupDone = appState.setupDone;
  unlockState();

  xSemaphoreTake(tftMutex, portMAX_DELAY);
  releaseSpiDevices();
  DisplayUI::setupPanel(tft);
  DisplayUI::runSelfTest(tft);
  // Establish the cloud identity first so the QR encodes the real device id.
  SupabaseClient::beginIdentity();
  // With no completed setup, boot straight into the pairing/QR screen (matches
  // the boot screenMode and avoids allocating the dashboard sprites before the
  // hub is paired). A configured hub boots to the main screen instead (spec
  // §3.1) — the display task paints it on its first loop.
  if (!setupDone) {
    DisplayUI::drawWifiSetup(tft, provisioningUrl());
  }
  releaseSpiDevices();
  xSemaphoreGive(tftMutex);
#endif
}

void setupLoRa() {
  // Ensure all SPI CS lines are deasserted before touching the LoRa SPI bus.
  // This prevents contention if TFT previously held the bus.
  releaseSpiDevices();
  delay(20); // Let the bus fully settle after TFT operations.

  LOGI(TAG_LORA, "SPI pins SCK=%d MISO=%d MOSI=%d NSS=%d", LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_CS_PIN);

  loraSPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_CS_PIN);
  releaseSpiDevices();
  delay(10); // Short settle after SPI bus init.

  // Feed the watchdog before the blocking radio.begin() call.
  resetWatchdog();
  yield();

  int state = radio.begin(
    LORA_FREQUENCY_MHZ,
    LORA_BANDWIDTH_KHZ,
    LORA_SPREADING_FACTOR,
    LORA_CODING_RATE,
    LORA_SYNC_WORD,
    LORA_TX_POWER_DBM,
    LORA_PREAMBLE_LEN
  );

  if (state != RADIOLIB_ERR_NONE) {
    LOGE(TAG_LORA, "Init failed code=%d — check wiring on RST/DIO0/CS/SPI", state);
    setRgbStatus(CRGB::Red);
    // Feed watchdog in the failure loop to avoid another INT_WDT on top of the real error.
    while (true) {
      resetWatchdog();
      delay(1000);
    }
  }

  radio.setCRC(true);
  radio.setPacketReceivedAction(onLoRaPacketReceived);
  LOGI(TAG_LORA, "Online freq=%.1fMHz bw=%.1fkHz sf=%u cr=%u sync=0x%02X",
       LORA_FREQUENCY_MHZ,
       LORA_BANDWIDTH_KHZ,
       LORA_SPREADING_FACTOR,
       LORA_CODING_RATE,
       LORA_SYNC_WORD);
}

void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  // Establish the cloud device identity (MAC-derived id + NVS secret) so the id
  // is available for the pairing QR code even before WiFi connects.
  SupabaseClient::beginIdentity();

  // Kick off the connection with the EXPLICIT stored SSID+password (same call
  // that works during BLE provisioning) rather than a bare WiFi.begin() — that
  // is more reliable than relying on the core's auto-reconnect. If there are no
  // saved credentials, bring up BLE provisioning so the app can send them.
  wifi_config_t staConf;
  bool hasCreds = (esp_wifi_get_config(WIFI_IF_STA, &staConf) == ESP_OK) &&
                  strlen(reinterpret_cast<const char *>(staConf.sta.ssid)) > 0;
  if (hasCreds) {
    const char *ssid = reinterpret_cast<const char *>(staConf.sta.ssid);
    const char *pass = reinterpret_cast<const char *>(staConf.sta.password);
    LOGI(TAG_WIFI, "Saved WiFi '%s' -> connecting (BLE provisioning skipped)", ssid);
    WiFi.begin(ssid, pass);
  } else {
    LOGI(TAG_WIFI, "No saved WiFi -> BLE provisioning");
    // Torn down by the cloud task once WiFi connects.
    BleProvisioning::begin(SupabaseClient::deviceId(), FIRMWARE_VERSION);
  }

  if (!SupabaseClient::configured()) {
    LOGW(TAG_WIFI, "Supabase is not configured yet. Cloud upload/polling disabled until config.h is updated.");
  }
}

void createTaskChecked(TaskFunction_t function, const char *name, uint32_t stack, UBaseType_t priority, TaskHandle_t *handle, BaseType_t core) {
  BaseType_t ok = xTaskCreatePinnedToCore(function, name, stack, nullptr, priority, handle, core);
  if (ok == pdPASS) {
    LOGI(TAG_TASK, "Created %s stack=%lu priority=%u core=%d", name, static_cast<unsigned long>(stack), priority, core);
  } else {
    LOGE(TAG_TASK, "Failed to create %s result=%d", name, ok);
  }
}

void createTasks() {
  createTaskChecked(loraTask, "lora_rx", LORA_TASK_STACK, 3, &loraTaskHandle, LORA_TASK_CORE);
  createTaskChecked(displayTask, "display", DISPLAY_TASK_STACK, 2, &displayTaskHandle, DISPLAY_TASK_CORE);
  createTaskChecked(cloudTask, "cloud", CLOUD_TASK_STACK, 1, &cloudTaskHandle, CLOUD_TASK_CORE);
  createTaskChecked(buttonTask, "buttons", BUTTON_TASK_STACK, 3, &buttonTaskHandle, BUTTON_TASK_CORE);
}

void setup() {
  DebugLog::begin();
  LOGI(TAG_APP, "Smart Water System Device 2 booting");
  LOGI(TAG_APP, "Firmware=%s Device=%s Pair=%s", FIRMWARE_VERSION, DEVICE_ID, DEVICE_PAIR_ID);
  DebugLog::resetReason(TAG_APP);

  stateMutex = xSemaphoreCreateMutex();
  tftMutex = xSemaphoreCreateMutex();
  if (stateMutex == nullptr || tftMutex == nullptr) {
    LOGE(TAG_APP, "Failed to allocate mutexes");
    while (true) {
      delay(1000);
    }
  }

  setupTaskWatchdog();
  // Register the Arduino setup/loop task with the watchdog so long
  // blocking init calls (TFT, LoRa) don't silently trip the INT_WDT.
  addCurrentTaskToWatchdog();

  setupPins();
  initializePumpSafeOff();

  preferences.begin("smart-water", false);

  // Restore setup progress (spec §3.1): a hub that finished setup boots to the
  // main screen; anything else re-enters setup mode automatically.
  bool setupDone = preferences.getBool("setupDone", false);
  lockState();
  appState.setupDone = setupDone;
  appState.screenMode = setupDone ? ScreenMode::Main : ScreenMode::WifiSetup;
  unlockState();
  LOGI(TAG_APP, "Boot state: setup %s", setupDone ? "complete -> main screen" : "incomplete -> setup mode");

  // Restore which Sensor Node this hub is bound to (factory reset clears it
  // along with everything else via preferences.clear()).
  loadLinkedNode();

  // Restore the tank configuration so calculations work before/without cloud.
  TankConfig tankCfg = loadTankConfigFromNvs();
  if (tankCfg.valid) {
    TankCalc::setConfig(tankCfg);
  }

  FastLED.addLeds<WS2812, RGB_LED_PIN, GRB>(rgbLeds, RGB_LED_COUNT);
  FastLED.setBrightness(RGB_LED_BRIGHTNESS);
  setRgbStatus(CRGB::Black);

  resetWatchdog(); // Feed WDT before TFT init (display self-test can be slow).
  setupTft();

  // LoRa BEFORE WiFi: once WiFi.begin() runs, the WiFi stack does flash/NVS
  // writes on its own task that can collide with the radio's SPI init and trip
  // the interrupt watchdog (caused a boot loop in 1.2.1 — build-timing lottery).
  resetWatchdog(); // Feed WDT before LoRa init (radio.begin() can take ~1-2s).
  setupLoRa();

  resetWatchdog(); // Feed WDT before WiFi init.
  setupWifi();

  restorePumpStateFromNvs();
  createTasks();

  DebugLog::heap(TAG_APP);
  LOGI(TAG_APP, "Setup complete");
}

void loop() {
  resetWatchdog();
  vTaskDelay(pdMS_TO_TICKS(1000));
}
