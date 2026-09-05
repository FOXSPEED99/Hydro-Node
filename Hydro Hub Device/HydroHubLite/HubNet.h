/*
 * HubNet.h - WiFi and over-the-air updates.
 *
 * Entirely optional. Leave WIFI_SSID empty in config.h and none of this runs:
 * the Hub never brings up the radio stack and behaves exactly as it did
 * before. That matters, because the receiver and the display must not depend
 * on the network - a Hub that stops showing the tank because the router
 * rebooted would be a worse device than one with no WiFi at all.
 *
 * So: everything here is non-blocking, and every failure is survivable. WiFi
 * down means no OTA, nothing more.
 */
#pragma once

#include <Arduino.h>

enum class NetState : uint8_t { Disabled, Connecting, Online };

void      hubNetBegin();
void      hubNetLoop();          /* call often; never blocks */
NetState  hubNetState();
String    hubNetAddress();       /* empty unless online */

/* True while a firmware update is being written. The caller must stop drawing
 * and stop touching the radio - an OTA that gets interrupted half-written
 * leaves a device that will not boot. */
bool      hubNetOtaActive();
uint8_t   hubNetOtaPercent();
