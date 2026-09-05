#pragma once

#include "config.h"
#include "SmartWaterTypes.h"

#include <Arduino.h>

#if ENABLE_TFT_DASHBOARD
#include <TFT_eSPI.h>
#endif

namespace DisplayUI {
#if ENABLE_TFT_DASHBOARD
void setupPanel(TFT_eSPI &tft);
void runSelfTest(TFT_eSPI &tft);
void drawMainShell(TFT_eSPI &tft);
void drawMain(TFT_eSPI &tft, const TelemetrySnapshot &snapshot, const AppState &state);
void drawMainUpdateOnly(TFT_eSPI &tft, const TelemetrySnapshot &snapshot, const AppState &state);
bool isPumpAnimating(); // true while the pump blade is spinning or the water is animating
void setPumpFillState(PumpFillState state); // water level shown inside the pump circle
void setWaterLevel(uint8_t percent);        // 0..100 shown in the left water-level gauge
void setWaterLevelPercent(uint8_t percent); // tank level (0-100) shown in the left circle
void drawWifiSetup(TFT_eSPI &tft, const String &payloadUrl);
void drawOtaScreen(TFT_eSPI &tft, const char *version);
#endif
}
