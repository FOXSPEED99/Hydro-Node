#pragma once

#include "SmartWaterTypes.h"

#include <Arduino.h>

namespace LoRaProtocol {
bool parseTelemetryPacket(const String &packet, TelemetrySnapshot &out);
String buildAckPacket(uint32_t sequence, bool pumpOn);
}
