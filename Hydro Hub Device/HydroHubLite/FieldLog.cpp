#include "FieldLog.h"
#include "config.h"

#if FIELDLOG_ENABLED
#include <Preferences.h>

static Preferences s_nvs;
static FieldStats s_st;
static uint32_t s_sinceSave = 0;
static uint32_t s_lastTickMs = 0;
static bool s_open = false;

static const char *NS = "hydrofield";
static const char *KEY = "stats";

void fieldLogBegin()
{
    s_open = s_nvs.begin(NS, false);
    if (!s_open) {
        Serial.println("[LOG] NVS unavailable - counters will not survive a reboot");
        return;
    }
    size_t got = s_nvs.getBytes(KEY, &s_st, sizeof(s_st));
    if (got != sizeof(s_st)) {
        /* First run, or the struct changed shape between firmware versions.
         * Starting clean is the honest choice - silently reinterpreting old
         * bytes as a new layout would produce numbers that look real. */
        s_st = FieldStats();
        Serial.println("[LOG] starting a fresh field log");
    }
    s_st.bootCount++;
    s_lastTickMs = millis();
    fieldLogSaveNow();

    Serial.printf("[LOG] boot #%lu, %lu packets so far, %lu s uptime\n",
                  (unsigned long)s_st.bootCount, (unsigned long)s_st.accepted,
                  (unsigned long)s_st.upSeconds);
}

void fieldLogSaveNow()
{
    if (!s_open) return;
    s_nvs.putBytes(KEY, &s_st, sizeof(s_st));
    s_sinceSave = 0;
}

void fieldLogTick(uint32_t nowMs)
{
    if (nowMs - s_lastTickMs >= 1000UL) {
        s_st.upSeconds += (nowMs - s_lastTickMs) / 1000UL;
        s_lastTickMs = nowMs;
    }
}

void fieldLogOnReject(bool foreign)
{
    if (foreign) s_st.foreign++;
    else         s_st.rejected++;
}

void fieldLogOnPacket(const hn_packet_t &p, float rssi, float snr, uint32_t gapMs)
{
    const bool first = (s_st.accepted == 0);
    s_st.accepted++;

    /* The worst outage is the single most useful reliability number: an
     * average hides a three-hour hole, and a three-hour hole is what you
     * actually need to design around. */
    const uint32_t gapSec = gapMs / 1000UL;
    if (gapSec > s_st.worstGapSec && !first) s_st.worstGapSec = gapSec;

    const int16_t r = (int16_t)rssi;
    if (first || r < s_st.rssiMin) s_st.rssiMin = r;
    if (first || r > s_st.rssiMax) s_st.rssiMax = r;
    const int16_t sn = (int16_t)(snr * 10.0f);
    if (first || sn < s_st.snrMinTenths) s_st.snrMinTenths = sn;

    if (p.battery_dv != HN_BATT_NONE) {
        const uint16_t mv = HN_BATT_MV(p.battery_dv);
        if (s_st.battFirstMv == 0) s_st.battFirstMv = mv;
        s_st.battLastMv = mv;
        if (s_st.battMinMv == 0 || mv < s_st.battMinMv) s_st.battMinMv = mv;
    }

    if (HN_ST_STATUS(p.st_us) != HN_W_OK) s_st.faultLevel++;
    if (HN_ST_STATUS(p.st_tp) != HN_W_OK) s_st.faultTemp++;
    if (HN_ST_STATUS(p.st_fl) != HN_W_OK) s_st.faultFlow++;
    if (HN_ST_STATUS(p.st_us) == HN_W_NO_TARGET) s_st.noEcho++;
    if (HN_ST_FLOW(p.st_fl) == HN_W_FLOW_FILLING) s_st.fillingCycles++;

    if (++s_sinceSave >= FIELDLOG_SAVE_EVERY) fieldLogSaveNow();
}

const FieldStats &fieldLogStats() { return s_st; }

int fieldLogReliabilityPct()
{
    const uint32_t expected = s_st.accepted + s_st.missed;
    if (expected == 0) return -1;
    return (int)((s_st.accepted * 100UL) / expected);
}

#else   /* !FIELDLOG_ENABLED */

static FieldStats s_st;
void fieldLogBegin() {}
void fieldLogOnPacket(const hn_packet_t &, float, float, uint32_t) {}
void fieldLogOnReject(bool) {}
void fieldLogTick(uint32_t) {}
void fieldLogSaveNow() {}
const FieldStats &fieldLogStats() { return s_st; }
int fieldLogReliabilityPct() { return -1; }

#endif
