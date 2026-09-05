#pragma once

#include <Arduino.h>

// In-app WiFi provisioning over BLE, matching the mobile app's
// lib/ble-provisioning.ts protocol (service 4bf5a100-..., chars 101-106).
// The app connects over Bluetooth, reads device info, asks the device to scan
// WiFi, sends the chosen SSID/password, and the device joins that network — no
// manual hotspot switching. BLE runs only while unprovisioned and is torn down
// once WiFi connects, so it never fights the dashboard for RAM.
namespace BleProvisioning {

// Start advertising. deviceId is the cloud hub id ("HH-..."), reported to the app.
void begin(const char *deviceId, const char *fwVersion);

// Process any pending scan/connect requests. Call periodically (e.g. from the
// cloud task). Runs the blocking WiFi scan/connect off the BLE stack.
void loop();

// Tear down BLE to free RAM (call once WiFi is connected).
void stop();

bool active();

}  // namespace BleProvisioning
