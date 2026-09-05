/*
 * Hydro Hub Lite - receive the Node, do the maths, show it.
 *
 * A deliberately small Hub for the current stage. The full Hub in
 * ../Hydro-Hub keeps the cloud, the phone app, OTA, WiFi provisioning and the
 * pump relay; none of that is here, and this is not a replacement for it.
 *
 * The split with the Node is the whole point of the architecture: the Node
 * measures and reports RAW values - echo microseconds, the DS18B20's own
 * register, the flow switch's ADC count - and every conversion happens here.
 * Tank height, blind zone, capacity and the speed-of-sound correction all live
 * on this device, which can be reflashed from the kitchen table. The Node is
 * sealed on a roof and must never need to know any of it.
 *
 * Board: ESP32-S3, 480x320 TFT (TFT_eSPI), Ra-02 SX1278 (RadioLib).
 * Libraries: TFT_eSPI, RadioLib. Nothing else - WiFi, OTA and NVS are part of
 * the ESP32 core.
 */

#include "config.h"
#include "NodeLink.h"
#include "TankMath.h"
#include "Dashboard.h"
#include "HubNet.h"
#include "FieldLog.h"
#include "hn_packet.h"

#include <esp_task_wdt.h>

#include <TFT_eSPI.h>

static TFT_eSPI tft;
static DashModel model;

/* Per-tank capacities, summed once at boot. Listing them individually is what
 * lets tanks of different sizes add up correctly. */
static const uint32_t kTankLiters[] = TANK_LITERS_LIST;
static_assert(sizeof(kTankLiters) / sizeof(kTankLiters[0]) == TANK_COUNT,
              "TANK_LITERS_LIST must have exactly TANK_COUNT entries");

static tank_config_t gTank;
static bool gRadioOk = false;
static uint32_t gLastPacketMs = 0;

/*
 * Reboot if the main loop stops running for HUB_WDT_TIMEOUT_S. A hung Hub on a
 * wall for three weeks is indistinguishable from a dead one, and a reboot
 * costs nothing here - the field log lives in NVS and survives it.
 *
 * Both API shapes, because the ESP32 core changed it at 3.0 and this has to
 * build on whichever is installed.
 */
static void setupWatchdog()
{
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    esp_task_wdt_config_t cfg = {
        .timeout_ms = HUB_WDT_TIMEOUT_S * 1000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
        .trigger_panic = true,
    };
    if (esp_task_wdt_reconfigure(&cfg) == ESP_ERR_INVALID_STATE) esp_task_wdt_init(&cfg);
#else
    esp_task_wdt_init(HUB_WDT_TIMEOUT_S, true);
#endif
    esp_task_wdt_add(nullptr);
}

/* ------------------------------------------------------------------------- */

static void buildTankConfig()
{
    uint32_t total = 0;
    for (uint8_t i = 0; i < TANK_COUNT; ++i) total += kTankLiters[i];

    gTank.tank_count        = TANK_COUNT;
    gTank.total_liters      = total;
    gTank.water_height_cm   = TANK_WATER_HEIGHT_CM;
    gTank.blind_cm          = TANK_BLIND_CM;
    gTank.transducer_sep_mm = TANK_TRANSDUCER_SEP_MM;

    Serial.printf("[TANK] %u tank(s), %lu L total, %u cm water, %u cm blind\n",
                  gTank.tank_count, (unsigned long)gTank.total_liters,
                  gTank.water_height_cm, gTank.blind_cm);
}

/*
 * Print every accepted packet as JSON. Binary on the air is an energy
 * decision, not a debuggability one - a bench session should look exactly like
 * it would have if the Node were sending text.
 */
static void logPacketAsJson(const hn_packet_t &p, const tank_result_t &t, float rssi, float snr)
{
    Serial.printf(
        "{\"node\":%u,\"seq\":%u,\"echoUs\":%u,\"tempRaw\":%d,\"flowAdc\":%u,"
        "\"st\":{\"level\":%u,\"temp\":%u,\"flow\":%u},"
        "\"presence\":{\"level\":%u,\"temp\":%u,\"flow\":%u},"
        "\"flowState\":%u,\"gated\":%s,"
        "\"calc\":{\"distanceCm\":%.1f,\"levelPct\":%.1f,\"liters\":%lu,\"valid\":%s},"
        "\"rssi\":%.0f,\"snr\":%.1f}\n",
        p.node_id, p.seq, p.echo_us, p.temp_raw, p.flow_adc8,
        HN_ST_STATUS(p.st_us), HN_ST_STATUS(p.st_tp), HN_ST_STATUS(p.st_fl),
        HN_ST_PRESENCE(p.st_us), HN_ST_PRESENCE(p.st_tp), HN_ST_PRESENCE(p.st_fl),
        HN_ST_FLOW(p.st_fl), (p.flags & HN_FLAG_GATED_BY_FLOW) ? "true" : "false",
        t.distance_cm, t.level_pct, (unsigned long)t.volume_liters,
        t.valid ? "true" : "false", rssi, snr);
}

/* Rebuild everything the screen shows from the newest packet. */
static void refreshModel()
{
    const LinkStats &st = nodeLinkStats();
    const uint32_t now = millis();

    model.radioOk  = gRadioOk;
    model.link     = nodeLinkState(now);
    model.ageMs    = nodeLinkAgeMs(now);
    model.rssi     = st.rssi;
    model.snr      = st.snr;
    model.accepted = st.accepted;
    model.missed   = st.missed;
    model.rejected = st.rejected;
    model.foreign  = st.foreign;

    model.totalLiters = gTank.total_liters;
    model.tankCount   = gTank.tank_count;
    model.haveReading = st.haveLast;

    model.netOnline = (hubNetState() == NetState::Online);
    strncpy(model.netAddr, hubNetAddress().c_str(), sizeof(model.netAddr) - 1);
    model.netAddr[sizeof(model.netAddr) - 1] = '\0';
    model.stats = &fieldLogStats();
    model.reliabilityPct = fieldLogReliabilityPct();

    if (!st.haveLast) return;

    const hn_packet_t &p = st.last;
    model.echoUs   = p.echo_us;
    model.tempRaw  = p.temp_raw;
    model.flowAdc  = p.flow_adc8;
    model.seq      = p.seq;
    model.nodeId   = p.node_id;
    model.stUs     = p.st_us;
    model.stTp     = p.st_tp;
    model.stFl     = p.st_fl;
    model.flowState = HN_ST_FLOW(p.st_fl);
    model.gated    = (p.flags & HN_FLAG_GATED_BY_FLOW) != 0;
    model.batteryMv = (p.battery_dv == HN_BATT_NONE) ? 0 : HN_BATT_MV(p.battery_dv);

    /* Temperature drives the speed of sound. When the Node has none, fall back
     * to the configured constant and SAY SO - an assumed temperature is a
     * systematic distance error, and the screen should not hide that. */
    if (p.temp_raw != HN_TEMP_RAW_NONE) {
        model.tempC = tank_temp_c(p.temp_raw);
        model.tempValid = true;
        model.tempAssumed = false;
    } else {
        model.tempC = TANK_FALLBACK_TEMP_C;
        model.tempValid = false;
        model.tempAssumed = true;
    }

    tank_compute(&gTank, p.echo_us, model.tempC, &model.tank);
}

/* ------------------------------------------------------------------------- */

static void pollButtons()
{
    static uint32_t lastA = 0;
    static bool prevA = true;

    const bool a = digitalRead(PIN_BUTTON_A);
    const uint32_t now = millis();
    if (prevA && !a && (now - lastA) > BUTTON_DEBOUNCE_MS) {
        lastA = now;
        DashScreen next;
        switch (dashboardScreen()) {
        case DashScreen::Main:        next = DashScreen::Diagnostics; break;
        case DashScreen::Diagnostics: next = DashScreen::Field;       break;
        default:                      next = DashScreen::Main;        break;
        }
        dashboardSetScreen(tft, next);
    }
    prevA = a;
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(300);
    Serial.println("\n[HUB] Hydro Hub Lite");

    pinMode(PIN_BUTTON_A, INPUT_PULLUP);
    pinMode(PIN_BUTTON_B, INPUT_PULLUP);

    buildTankConfig();
    fieldLogBegin();

    dashboardBegin(tft);
    refreshModel();
    dashboardRender(tft, model);   /* paint immediately - never a blank panel */

    gRadioOk = nodeLinkBegin();
    if (!gRadioOk) {
        /* Not fatal. The screen says so, and the Hub keeps running so the
         * failure is visible rather than looking like a dead device. */
        Serial.println("[HUB] radio unavailable - display only");
    }

    hubNetBegin();
    setupWatchdog();

    refreshModel();
    dashboardInvalidate();
}

void loop()
{
    esp_task_wdt_reset();
    hubNetLoop();
    fieldLogTick(millis());

    /*
     * While firmware is being written, nothing else may draw or touch the
     * radio. An OTA interrupted half-way leaves a device that will not boot,
     * and this one is on a wall.
     */
    if (hubNetOtaActive()) {
        dashboardDrawOta(tft, hubNetOtaPercent());
        return;
    }

    if (nodeLinkPoll()) {
        const LinkStats &st = nodeLinkStats();
        const uint32_t gap = gLastPacketMs ? (millis() - gLastPacketMs) : 0;
        gLastPacketMs = millis();

        refreshModel();
        fieldLogOnPacket(st.last, st.rssi, st.snr, gap);
        logPacketAsJson(st.last, model.tank, st.rssi, st.snr);
    } else {
        /* Even with no packet the link ages, and the header has to keep up. */
        refreshModel();
    }

    pollButtons();
    dashboardRender(tft, model);
    delay(50);
}
