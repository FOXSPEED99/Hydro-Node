#pragma once

#include "config.h"

#include <Arduino.h>
#include <stdarg.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace DebugLog {
enum Level : uint8_t {
  Error = 0,
  Warn = 1,
  Info = 2,
  Debug = 3,
  Verbose = 4
};

inline const char *levelName(Level level) {
  switch (level) {
    case Error: return "ERR";
    case Warn: return "WRN";
    case Info: return "INF";
    case Debug: return "DBG";
    case Verbose: return "VRB";
    default: return "LOG";
  }
}

inline const char *resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "POWERON";
    case ESP_RST_EXT: return "EXTERNAL";
    case ESP_RST_SW: return "SOFTWARE";
    case ESP_RST_PANIC: return "PANIC";
    case ESP_RST_INT_WDT: return "INT_WDT";
    case ESP_RST_TASK_WDT: return "TASK_WDT";
    case ESP_RST_WDT: return "OTHER_WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT: return "BROWNOUT";
    case ESP_RST_SDIO: return "SDIO";
    default: return "UNKNOWN";
  }
}

inline bool enabled(Level level) {
#if ENABLE_SERIAL_DEBUG
  return static_cast<uint8_t>(level) <= DEBUG_LOG_LEVEL;
#else
  (void)level;
  return false;
#endif
}

inline void begin() {
#if ENABLE_SERIAL_DEBUG
  Serial.begin(SERIAL_BAUD_RATE);
  delay(300);
#endif
}

inline void logf(Level level, const char *tag, int line, const char *format, ...) {
  if (!enabled(level)) {
    return;
  }

  char message[192];
  va_list args;
  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  Serial.printf("[%10lu] [%s] %-8s L%-4d %s\n", millis(), levelName(level), tag, line, message);
}

inline void heap(const char *tag) {
  if (!enabled(Debug)) {
    return;
  }
  Serial.printf("[%10lu] [DBG] %-8s heap=%lu minHeap=%lu psram=%lu\n",
                millis(),
                tag,
                static_cast<unsigned long>(ESP.getFreeHeap()),
                static_cast<unsigned long>(ESP.getMinFreeHeap()),
                static_cast<unsigned long>(ESP.getFreePsram()));
}

inline void stack(const char *tag) {
  if (!enabled(Debug)) {
    return;
  }
  Serial.printf("[%10lu] [DBG] %-8s stackHighWater=%lu words\n",
                millis(),
                tag,
                static_cast<unsigned long>(uxTaskGetStackHighWaterMark(nullptr)));
}

inline void resetReason(const char *tag) {
  esp_reset_reason_t reason = esp_reset_reason();
  logf(Info, tag, __LINE__, "Reset reason: %s (%d)", resetReasonName(reason), static_cast<int>(reason));
}
}

#define LOGE(tag, fmt, ...) DebugLog::logf(DebugLog::Error, tag, __LINE__, fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...) DebugLog::logf(DebugLog::Warn, tag, __LINE__, fmt, ##__VA_ARGS__)
#define LOGI(tag, fmt, ...) DebugLog::logf(DebugLog::Info, tag, __LINE__, fmt, ##__VA_ARGS__)
#define LOGD(tag, fmt, ...) DebugLog::logf(DebugLog::Debug, tag, __LINE__, fmt, ##__VA_ARGS__)
#define LOGV(tag, fmt, ...) DebugLog::logf(DebugLog::Verbose, tag, __LINE__, fmt, ##__VA_ARGS__)
