/*
 * Dashboard.h - the 480x320 display.
 *
 * DESIGN INTENT
 * -------------
 * This screen is read from across a room, by someone who wants one answer:
 * how much water is there. So the litres are a hero number and everything else
 * is subordinate to it.
 *
 * The rule that shapes the rest: THE SCREEN MUST NEVER LOOK CONFIDENT ABOUT
 * DATA IT DOES NOT HAVE. A tank monitor that keeps showing a comfortable 80%
 * because the sensor died an hour ago is worse than one showing nothing - it
 * is how a pump gets run dry. So the water in the gauge carries trust as well
 * as level:
 *
 *   solid blue    a fresh reading from a healthy sensor
 *   hatched       the reading is stale, or the tank was filling when it was
 *                 taken, so the surface was moving and it is not trustworthy
 *   hollow, dim   no usable reading at all - shows "--", never a number
 *
 * Status colour never carries meaning on its own. Every sensor pill and every
 * banner pairs its colour with a glyph and a word, so the screen still reads
 * correctly for a colour-blind viewer, and at a glance in the dark.
 */
#pragma once

#include "NodeLink.h"
#include "TankMath.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

enum class DashScreen : uint8_t { Main, Diagnostics };

/* Everything the screen draws, assembled by the sketch so the renderer has no
 * opinions of its own about what the numbers mean. */
struct DashModel {
    bool      radioOk = false;
    LinkState link = LinkState::NeverHeard;
    uint32_t  ageMs = UINT32_MAX;
    float     rssi = 0, snr = 0;
    uint32_t  accepted = 0, missed = 0, rejected = 0, foreign = 0;

    bool      haveReading = false;   /* a packet has ever been decoded        */
    tank_result_t tank{};            /* .valid false when no usable echo      */
    uint32_t  totalLiters = 0;
    uint8_t   tankCount = 1;

    bool      tempValid = false;
    float     tempC = 0;
    bool      tempAssumed = false;   /* fell back to the configured constant  */

    uint8_t   stUs = 0, stTp = 0, stFl = 0;  /* packed status bytes           */
    uint8_t   flowState = 0;
    bool      gated = false;

    /* Raw, for the diagnostics screen. */
    uint16_t  echoUs = 0;
    int16_t   tempRaw = 0;
    uint8_t   flowAdc = 0;
    uint16_t  seq = 0;
    uint16_t  nodeId = 0;
};

void dashboardBegin(TFT_eSPI &tft);
void dashboardSetScreen(TFT_eSPI &tft, DashScreen s);
DashScreen dashboardScreen();

/* Call at ~20 Hz. Redraws only what changed, and steps the water animation. */
void dashboardRender(TFT_eSPI &tft, const DashModel &m);

/* Full repaint on the next render. */
void dashboardInvalidate();
