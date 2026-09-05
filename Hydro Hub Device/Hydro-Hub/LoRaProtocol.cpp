#include "LoRaProtocol.h"

#include "DebugLog.h"
#include "config.h"

#include <ArduinoJson.h>

namespace LoRaProtocol {
bool parseTelemetryPacket(const String &packet, TelemetrySnapshot &out) {
  if (packet.length() == 0 || packet.length() > LORA_MAX_PACKET_BYTES) {
    strlcpy(out.lastError, "Bad packet size", sizeof(out.lastError));
    LOGW("LORA", "Rejected packet length=%u", packet.length());
    return false;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, packet);
  if (error) {
    strlcpy(out.lastError, "JSON parse error", sizeof(out.lastError));
    LOGW("LORA", "JSON parse failed: %s | packet=%s", error.c_str(), packet.c_str());
    return false;
  }

  const char *pairId = doc["pairId"] | "";
  if (strcmp(pairId, DEVICE_PAIR_ID) != 0) {
    strlcpy(out.lastError, "Pair ID mismatch", sizeof(out.lastError));
    LOGW("LORA", "Pair mismatch received=%s expected=%s", pairId, DEVICE_PAIR_ID);
    return false;
  }

  // Node identity (node firmware >= 1.1.0). Older nodes send nothing here and
  // stay acceptable — the hub simply has no id to bind to.
  strlcpy(out.nodeId, doc["node"] | "", sizeof(out.nodeId));

  out.sequence = doc["seq"] | out.sequence;
  out.sourcePumpOn = doc.containsKey("pumpOn") ? doc["pumpOn"].as<bool>() : doc["pump_on"].as<bool>();

  // Preferred production format (spec §4.2): the Sensor Node sends RAW data
  // only — ultrasonic distance + flow reed switch — and the hub computes
  // everything. The caller runs TankCalc on these fields.
  if (doc.containsKey("distanceCm") || doc.containsKey("distance_cm")) {
    out.valid = true;
    out.rawMode = true;
    out.distanceCm = doc.containsKey("distanceCm") ? doc["distanceCm"].as<float>() : doc["distance_cm"].as<float>();
    out.flowSwitch = doc.containsKey("flow") ? doc["flow"].as<bool>()
                     : (doc.containsKey("flowing") ? doc["flowing"].as<bool>() : doc["flow_switch"].as<bool>());
    if (doc.containsKey("tempC")) out.waterTempC = doc["tempC"].as<float>();
    if (doc.containsKey("bat")) out.nodeBatteryV = doc["bat"].as<float>();
    if (doc.containsKey("sens")) out.sensorMask = doc["sens"].as<uint8_t>();
    out.signalLost = false;
    strlcpy(out.lastError, "LoRa online", sizeof(out.lastError));
    LOGD("LORA", "Parsed raw seq=%lu distance=%.1fcm flow=%d temp=%.1fC bat=%.2fV",
         static_cast<unsigned long>(out.sequence), out.distanceCm, out.flowSwitch,
         out.waterTempC, out.nodeBatteryV);
    return true;
  }

  // Legacy format: sender already computed level/volume/flow.
  int level = doc.containsKey("levelPct") ? doc["levelPct"].as<int>() : doc["level_percent"].as<int>();

  out.valid = true;
  out.rawMode = false;
  out.levelPercent = constrain(level, 0, 100);
  out.volumeLiters = doc.containsKey("volumeLiters") ? doc["volumeLiters"].as<float>() : doc["volume_liters"].as<float>();
  out.filling = doc["filling"] | false;
  out.flowLpm = doc.containsKey("flowLpm") ? doc["flowLpm"].as<float>() : doc["flow_lpm"].as<float>();
  out.etaSeconds = doc.containsKey("etaSeconds") ? doc["etaSeconds"].as<uint32_t>() : doc["eta_seconds"].as<uint32_t>();
  out.signalLost = false;
  strlcpy(out.lastError, "LoRa online", sizeof(out.lastError));

  LOGD("LORA", "Parsed seq=%lu level=%u volume=%.1f flow=%.1f filling=%d",
       static_cast<unsigned long>(out.sequence),
       out.levelPercent,
       out.volumeLiters,
       out.flowLpm,
       out.filling);
  return true;
}

String buildAckPacket(uint32_t sequence, bool pumpOn) {
  StaticJsonDocument<160> ackDoc;
  ackDoc["pairId"] = DEVICE_PAIR_ID;
  ackDoc["ack"] = sequence;
  ackDoc["deviceId"] = DEVICE_ID;
  ackDoc["pumpOn"] = pumpOn;

  String ack;
  serializeJson(ackDoc, ack);
  return ack;
}
}
