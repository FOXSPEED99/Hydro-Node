#pragma once

#include "SmartWaterTypes.h"

#include <Arduino.h>

// All water math runs on the Hydro Hub (spec §5). The Sensor Node only reports
// the raw ultrasonic distance and the flow reed switch; this module converts
// that into level %, volume, a flow-strength classification and a time-to-full
// estimate using the user's tank configuration.
namespace TankCalc {

// Install / replace the active tank configuration (from NVS at boot, or from a
// device_sync response when the user saves new values in the app).
void setConfig(const TankConfig &config);
const TankConfig &config();
bool hasConfig();

// Reset the level-rate history (e.g. after a config change so stale rates
// computed against the old geometry don't leak into the new estimates).
void resetHistory();

// Convert one raw reading into computed telemetry. Fills levelPercent,
// volumeLiters, filling, flowLpm and etaSeconds on the snapshot. Returns false
// (and only sets lastError) when no tank configuration is available yet.
bool applyRawReading(TelemetrySnapshot &snapshot, float distanceCm, bool flowOn, uint32_t nowMs);

}  // namespace TankCalc
