#include "HubNet.h"
#include "config.h"

#include <string.h>

/* Compile the whole module out when no network is configured, rather than
 * carrying an unused WiFi stack. Driven by an explicit switch rather than
 * inspecting the SSID string - sizeof() and string contents are not available
 * to the preprocessor, so "leave the SSID empty" cannot actually be tested
 * here. */
#if WIFI_ENABLED
#include <WiFi.h>
#include <ArduinoOTA.h>

static NetState s_state = NetState::Connecting;
static uint32_t s_lastAttempt = 0;
static bool     s_otaActive = false;
static uint8_t  s_otaPct = 0;
static bool     s_otaStarted = false;

static void startOta()
{
    if (s_otaStarted) return;

    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
        s_otaActive = true;
        s_otaPct = 0;
        Serial.println("[OTA] update starting - do not power off");
    });
    ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
        s_otaPct = total ? (uint8_t)((done * 100U) / total) : 0;
    });
    ArduinoOTA.onEnd([]() {
        s_otaPct = 100;
        Serial.println("[OTA] written, rebooting");
    });
    ArduinoOTA.onError([](ota_error_t e) {
        s_otaActive = false;
        Serial.printf("[OTA] failed (%u) - the running firmware is untouched\n", e);
    });

    ArduinoOTA.begin();
    s_otaStarted = true;
    Serial.printf("[OTA] ready as \"%s\" - it appears under Tools > Port\n", OTA_HOSTNAME);
}

void hubNetBegin()
{
    Serial.printf("[WIFI] connecting to \"%s\"\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);          /* keeps OTA responsive */
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    s_lastAttempt = millis();
    s_state = NetState::Connecting;
}

void hubNetLoop()
{
    const bool up = (WiFi.status() == WL_CONNECTED);

    if (up && s_state != NetState::Online) {
        s_state = NetState::Online;
        Serial.printf("[WIFI] online at %s\n", WiFi.localIP().toString().c_str());
        startOta();
    } else if (!up && s_state == NetState::Online) {
        s_state = NetState::Connecting;
        Serial.println("[WIFI] lost - the Hub keeps receiving and displaying");
    }

    if (!up) {
        /* Retry slowly. Calling WiFi.begin() again too soon aborts the
         * association already in progress, which presents as a device that
         * never connects. */
        if (millis() - s_lastAttempt > WIFI_RETRY_MS) {
            s_lastAttempt = millis();
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
        return;
    }

    if (s_otaStarted) ArduinoOTA.handle();
}

NetState hubNetState() { return s_state; }
String   hubNetAddress() { return (s_state == NetState::Online) ? WiFi.localIP().toString() : String(); }
bool     hubNetOtaActive() { return s_otaActive; }
uint8_t  hubNetOtaPercent() { return s_otaPct; }

#else   /* WIFI_ENABLED 0 */

void hubNetBegin() { Serial.println("[WIFI] disabled (WIFI_ENABLED 0) - display only"); }
void hubNetLoop() {}
NetState hubNetState() { return NetState::Disabled; }
String hubNetAddress() { return String(); }
bool hubNetOtaActive() { return false; }
uint8_t hubNetOtaPercent() { return 0; }

#endif
