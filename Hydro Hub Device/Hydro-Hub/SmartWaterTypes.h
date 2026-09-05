#pragma once

#include <Arduino.h>

enum class ScreenMode : uint8_t {
  Main,
  WifiSetup,
  // A firmware download is running. The display task paints the OTA screen once
  // and then leaves the panel alone until the reboot — nothing else may draw.
  Ota
};

enum class PumpSource : uint8_t {
  BootRestore,
  Button,
  Remote,
  LoRa
};

// How strongly the tank is currently filling — drives the water level shown
// inside the pump circle on the dashboard. Sent by the remote sender over LoRa
// (once that device exists); until then use DisplayUI::setPumpFillState() or the
// demo-cycle option in DisplayUI.cpp to preview all four states.
enum class PumpFillState : uint8_t {
  None = 0,   // not filling — circle empty
  Weak,       // weak inflow  — low water
  Good,       // good inflow  — mid water
  Strong      // strong inflow — circle full
};

// Tank geometry/capacity entered by the user in the app (spec §3.2 step 4) and
// delivered to the hub through device_sync. Persisted to NVS so calculations
// work offline after a reboot. `version` bumps every time the user re-saves.
struct TankConfig {
  bool valid = false;
  uint32_t version = 0;
  uint16_t heightCm = 0;        // max water height when 100% full
  uint16_t blindCm = 0;         // sensor -> max-water-level distance (reads this when full)
  uint32_t capacityLiters = 0;  // total capacity across all tanks
  uint8_t tankCount = 1;
};

struct TelemetrySnapshot {
  bool valid = false;
  uint32_t sequence = 0;
  uint8_t levelPercent = 0;
  float volumeLiters = 0;
  bool filling = false;
  float flowLpm = 0;
  uint32_t etaSeconds = 0;
  bool sourcePumpOn = false;
  // Raw sensor readings (spec §4.2: the Sensor Node sends raw data only; the
  // hub does all calculations). rawMode is true when the last packet carried a
  // distance instead of precomputed values.
  bool rawMode = false;
  float distanceCm = -1;
  bool flowSwitch = false;
  // Extras reported by the Sensor Node: water temperature (DS18B20; -127 =
  // not available) and the node's battery voltage (0 = not available).
  float waterTempC = -127;
  float nodeBatteryV = 0;
  // Sensor presence mask from the node (bit0=ultrasonic bit1=temp bit2=flow;
  // 0xFF = sender didn't report it).
  uint8_t sensorMask = 0xFF;
  // Permanent id of the Sensor Node that sent this packet (node firmware
  // >= 1.1.0). Empty from older nodes, which the hub still accepts.
  char nodeId[12] = {0};
  float rssi = 0;
  float snr = 0;
  uint32_t packetCount = 0;
  uint32_t lastPacketMs = 0;
  uint32_t lastReadingMs = 0; // Local timestamp of last valid reading
  bool signalLost = true;
  char lastError[48] = "Waiting for LoRa";
};

struct AppState {
  bool pumpOn = false;
  uint32_t pumpStateChangedMs = 0; // Timestamp of last pump state change
  bool isAutoMode = false; // Default to Manual
  char wifiSSID[33] = "";
  bool wifiConnected = false;
  bool cloudOk = false;
  bool cloudBusy = false;
  // WiFi turned off by a 5s long-press on the WiFi button (spec §6.2). Never
  // persisted: WiFi is always re-enabled on boot.
  bool wifiUserOff = false;
  // Setup finished at least once (paired + tank data saved). Restored from NVS
  // so a reboot goes straight to the main screen; the cloud sync keeps it true.
  bool setupDone = false;
  // Set while the user opened the QR screen with a short press after setup
  // (extra phones/accounts, spec §6.2); auto-dismissed by timeout or button.
  bool qrManuallyOpened = false;
  // One-shot: another task painted over the screen (e.g. failed OTA) — the
  // display task repaints the current mode in full and clears this.
  bool forceRedraw = false;
  // Boot into the pairing/QR screen and stay there until the cloud reports the
  // hub is linked to an account — then the cloud task switches to Main. Keeping
  // the dashboard sprites unallocated until paired also leaves maximum heap for
  // the WiFi/BLE/TLS stack during registration.
  ScreenMode screenMode = ScreenMode::WifiSetup;
  // Target version shown on the OTA screen while screenMode == Ota.
  char otaVersion[32] = "";
  uint32_t wifiSetupStartedMs = 0;
  bool telemetryDirtyForCloud = false;
  bool pumpDirtyForCloud = false;
};
