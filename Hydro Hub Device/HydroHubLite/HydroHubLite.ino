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
 * Libraries: TFT_eSPI, RadioLib. Nothing else.
 */

#include "config.h"
#include "NodeLink.h"
#include "TankMath.h"
#include "Dashboard.h"
#include "hn_packet.h"

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
        dashboardSetScreen(tft, dashboardScreen() == DashScreen::Main
                                    ? DashScreen::Diagnostics
                                    : DashScreen::Main);
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

    dashboardBegin(tft);
    refreshModel();
    dashboardRender(tft, model);   /* paint immediately - never a blank panel */

    gRadioOk = nodeLinkBegin();
    if (!gRadioOk) {
        /* Not fatal. The screen says so, and the Hub keeps running so the
         * failure is visible rather than looking like a dead device. */
        Serial.println("[HUB] radio unavailable - display only");
    }

    refreshModel();
    dashboardInvalidate();
}

void loop()
{
    if (nodeLinkPoll()) {
        refreshModel();
        const LinkStats &st = nodeLinkStats();
        logPacketAsJson(st.last, model.tank, st.rssi, st.snr);
    } else {
        /* Even with no packet, the link ages and the header has to keep up. */
        refreshModel();
    }

    pollButtons();
    dashboardRender(tft, model);
    delay(50);
}
