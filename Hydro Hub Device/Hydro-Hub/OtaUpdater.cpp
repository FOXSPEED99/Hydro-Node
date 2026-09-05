#include "OtaUpdater.h"

#include "DebugLog.h"
#include "config.h"

#if ENABLE_OTA_UPDATES

#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_task_wdt.h>

namespace {

// Version that failed to install since boot — don't retry it every sync.
char gFailedVersion[32] = {0};

}  // namespace

namespace OtaUpdater {

bool shouldUpdate(const char *version) {
  if (version == nullptr || version[0] == 0) return false;
  if (strcmp(version, FIRMWARE_VERSION) == 0) return false;
  if (strcmp(version, gFailedVersion) == 0) return false;
  return true;
}

bool applyUpdate(const char *url, const char *version) {
  if (WiFi.status() != WL_CONNECTED) return false;

  LOGI("OTA", "Updating %s -> %s from %s", FIRMWARE_VERSION, version, url);

  WiFiClientSecure client;
#if SUPABASE_TLS_INSECURE_DEV_MODE
  client.setInsecure();
#else
  if (strlen(SUPABASE_ROOT_CA) == 0) {
    LOGE("OTA", "Secure TLS requested but SUPABASE_ROOT_CA empty");
    return false;
  }
  client.setCACert(SUPABASE_ROOT_CA);
#endif

  HTTPClient http;
  http.setTimeout(30000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // storage URLs often redirect
  if (!http.begin(client, url)) {
    LOGE("OTA", "HTTP begin failed");
    strncpy(gFailedVersion, version, sizeof(gFailedVersion) - 1);
    return false;
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    LOGE("OTA", "Download failed HTTP %d", code);
    http.end();
    strncpy(gFailedVersion, version, sizeof(gFailedVersion) - 1);
    return false;
  }

  int total = http.getSize();
  if (total <= 0) {
    LOGE("OTA", "Missing Content-Length");
    http.end();
    strncpy(gFailedVersion, version, sizeof(gFailedVersion) - 1);
    return false;
  }

  if (!Update.begin(total)) {
    LOGE("OTA", "Not enough OTA space for %d bytes", total);
    http.end();
    strncpy(gFailedVersion, version, sizeof(gFailedVersion) - 1);
    return false;
  }

  // Stream in chunks, feeding the task watchdog — a full image at household
  // bandwidth can take well past the 20s watchdog window.
  WiFiClient *stream = http.getStreamPtr();
  uint8_t buf[2048];
  int written = 0;
  uint32_t lastDataMs = millis();
  while (written < total) {
#if ENABLE_TASK_WATCHDOG
    esp_task_wdt_reset();
#endif
    size_t avail = stream->available();
    if (avail == 0) {
      if (millis() - lastDataMs > 30000) {
        LOGE("OTA", "Stream stalled at %d/%d bytes", written, total);
        Update.abort();
        http.end();
        strncpy(gFailedVersion, version, sizeof(gFailedVersion) - 1);
        return false;
      }
      delay(50);
      continue;
    }
    int n = stream->readBytes(buf, min(avail, sizeof(buf)));
    if (n <= 0) continue;
    lastDataMs = millis();
    if (Update.write(buf, n) != static_cast<size_t>(n)) {
      LOGE("OTA", "Flash write failed at %d bytes: %s", written, Update.errorString());
      Update.abort();
      http.end();
      strncpy(gFailedVersion, version, sizeof(gFailedVersion) - 1);
      return false;
    }
    written += n;
    if ((written % (128 * 1024)) < static_cast<int>(sizeof(buf))) {
      LOGI("OTA", "Progress %d/%d bytes", written, total);
    }
  }
  http.end();

  if (!Update.end(true)) {
    LOGE("OTA", "Finalize failed: %s", Update.errorString());
    strncpy(gFailedVersion, version, sizeof(gFailedVersion) - 1);
    return false;
  }

  LOGI("OTA", "Update installed. Rebooting into %s", version);
  delay(300);  // let the log line flush
  ESP.restart();
  return true;  // unreachable
}

}  // namespace OtaUpdater

#else

namespace OtaUpdater {
bool shouldUpdate(const char *) { return false; }
bool applyUpdate(const char *, const char *) { return false; }
}  // namespace OtaUpdater

#endif  // ENABLE_OTA_UPDATES
