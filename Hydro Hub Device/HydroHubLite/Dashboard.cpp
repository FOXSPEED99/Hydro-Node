#include "Dashboard.h"
#include "config.h"

#include <math.h>

/* ------------------------------------------------------------------------- */
/* Layout                                                                     */
/* ------------------------------------------------------------------------- */
#define HEADER_H     34
#define FOOTER_Y     262
#define FOOTER_H     (SCREEN_H - FOOTER_Y)

#define GAUGE_X      16
#define GAUGE_Y      48
#define GAUGE_W      126
#define GAUGE_H      202

#define RIGHT_X      162
#define RIGHT_W      (SCREEN_W - RIGHT_X - 16)

#define HERO_Y       50
#define TILES_Y      140
#define TILE_H       58
#define TILE_GAP     8
#define STRIP_Y      208

static DashScreen s_screen = DashScreen::Main;
static bool  s_full = true;
static float s_drawnPct = 0.0f;      /* animated toward the real level */

/* Previous values, so only genuinely changed fields are repainted. Redrawing
 * everything every frame on a 480x320 SPI panel is visibly slow and flickers. */
struct Prev {
    int32_t volume = -1, level = -1, waterCm = -1, tempDeci = -32768;
    int32_t gaugePct = -1;
    uint8_t stUs = 0xFF, stTp = 0xFF, stFl = 0xFF, flow = 0xFF;
    int8_t  link = -1;
    int32_t ageBucket = -1, rssi = -999;
    bool    gated = false, valid = false;
    uint16_t seq = 0xFFFF;
};
static Prev s_prev;

/* ------------------------------------------------------------------------- */
/* Small helpers                                                              */
/* ------------------------------------------------------------------------- */

static uint16_t statusColour(uint8_t st)
{
    switch (HN_ST_STATUS(st)) {
    case HN_W_OK:           return C_GOOD;
    case HN_W_NOT_MEASURED: return C_INK_MUTED;
    case HN_W_UNSTABLE:     return C_WARNING;
    case HN_W_OUT_OF_RANGE: return C_WARNING;
    case HN_W_NO_TARGET:    return C_SERIOUS;
    case HN_W_FAULT:        return C_CRITICAL;
    case HN_W_ABSENT:       return C_CRITICAL;
    default:                return C_INK_MUTED;
    }
}

/* The glyph is what makes the colour redundant rather than load-bearing. */
static const char *statusGlyph(uint8_t st)
{
    switch (HN_ST_STATUS(st)) {
    case HN_W_OK:           return "OK";
    case HN_W_NOT_MEASURED: return "--";
    case HN_W_UNSTABLE:     return "~";
    case HN_W_OUT_OF_RANGE: return "!";
    case HN_W_NO_TARGET:    return "?";
    case HN_W_FAULT:        return "X";
    case HN_W_ABSENT:       return "X";
    default:                return "?";
    }
}

static const char *statusWord(uint8_t st)
{
    switch (HN_ST_STATUS(st)) {
    case HN_W_OK:           return "ok";
    case HN_W_NOT_MEASURED: return "not read";
    case HN_W_UNSTABLE:     return "noisy";
    case HN_W_OUT_OF_RANGE: return "range";
    case HN_W_NO_TARGET:    return "no echo";
    case HN_W_FAULT:        return "fault";
    case HN_W_ABSENT:       return "MISSING";
    default:                return "?";
    }
}

/* Human ages read far better across a room than a timestamp. */
static void ageText(uint32_t ms, char *buf, size_t n)
{
    if (ms == UINT32_MAX)       snprintf(buf, n, "never");
    else if (ms < 60000UL)      snprintf(buf, n, "%lus ago", (unsigned long)(ms / 1000UL));
    else if (ms < 3600000UL)    snprintf(buf, n, "%lum ago", (unsigned long)(ms / 60000UL));
    else                        snprintf(buf, n, "%luh ago", (unsigned long)(ms / 3600000UL));
}

static void panel(TFT_eSPI &tft, int x, int y, int w, int h)
{
    tft.fillRoundRect(x, y, w, h, 6, C_PANEL);
}

/* A label above a value: the pattern used for every small statistic. */
static void tile(TFT_eSPI &tft, int x, int y, int w, const char *label,
                 const char *value, uint16_t valueColour)
{
    panel(tft, x, y, w, TILE_H);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_INK_MUTED, C_PANEL);
    tft.setTextPadding(0);
    tft.drawString(label, x + 10, y + 7, 2);

    tft.setTextDatum(BL_DATUM);
    tft.setTextColor(valueColour, C_PANEL);
    tft.setTextPadding(w - 18);
    tft.drawString(value, x + 10, y + TILE_H - 8, 4);
    tft.setTextPadding(0);
}

/* ------------------------------------------------------------------------- */
/* The tank gauge                                                             */
/* ------------------------------------------------------------------------- */

enum class Fill : uint8_t { Solid, Hatched, Hollow };

static void drawGauge(TFT_eSPI &tft, float pct, Fill mode, bool clampedFull)
{
    const int x = GAUGE_X, y = GAUGE_Y, w = GAUGE_W, h = GAUGE_H;

    tft.fillRoundRect(x, y, w, h, 8, C_SURFACE);
    tft.drawRoundRect(x, y, w, h, 8, C_HAIRLINE);
    tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 7, C_HAIRLINE);

    const int inX = x + 4, inY = y + 4, inW = w - 8, inH = h - 8;
    const int fillH = (int)((float)inH * pct / 100.0f + 0.5f);
    const int fillY = inY + inH - fillH;

    if (mode == Fill::Hollow) {
        /* No trustworthy reading at all. Deliberately not "0%" - an empty
         * gauge with a number would claim the tank is empty. */
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(C_INK_MUTED, C_SURFACE);
        tft.drawString("--", x + w / 2, y + h / 2 - 10, 6);
        tft.setTextColor(C_INK_MUTED, C_SURFACE);
        tft.drawString("no reading", x + w / 2, y + h / 2 + 28, 2);
        return;
    }

    if (fillH > 0) {
        if (mode == Fill::Solid) {
            tft.fillRect(inX, fillY, inW, fillH, C_WATER_DEEP);
            /* A lighter band at the surface reads as a water line rather than
             * a flat block, and makes the level edge easy to find quickly. */
            const int band = (fillH < 10) ? fillH : 10;
            tft.fillRect(inX, fillY, inW, band, C_WATER);
        } else {
            /* Hatched: the data is real but not to be trusted right now. */
            tft.fillRect(inX, fillY, inW, fillH, C_SURFACE);
            for (int yy = fillY; yy < fillY + fillH; yy += 5) {
                tft.drawFastHLine(inX, yy, inW, C_WATER_GHOST);
            }
            tft.drawFastHLine(inX, fillY, inW, C_WATER);
        }
    }

    /* Quarter marks, recessive - orientation without clutter. */
    for (int q = 1; q <= 3; ++q) {
        const int ty = inY + inH - (inH * q / 4);
        tft.drawFastHLine(inX, ty, 8, C_HAIRLINE);
        tft.drawFastHLine(inX + inW - 8, ty, 8, C_HAIRLINE);
    }

    /* The percentage sits inside the gauge, above the water line where there
     * is room, below it when the tank is nearly full. */
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)(pct + 0.5f));
    const bool high = pct > 62.0f;
    const int ty = high ? (fillY + 26) : (fillY - 26);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_INK, high ? C_WATER_DEEP : C_SURFACE);
    tft.setTextPadding(inW - 8);
    tft.drawString(buf, x + w / 2, ty, 4);
    tft.setTextPadding(0);

    if (clampedFull) {
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(C_WARNING, C_WATER);
        tft.drawString("OVER", x + w / 2, inY + 2, 2);
    }
}

/* ------------------------------------------------------------------------- */
/* Chrome                                                                     */
/* ------------------------------------------------------------------------- */

static void drawHeader(TFT_eSPI &tft, const DashModel &m)
{
    tft.fillRect(0, 0, SCREEN_W, HEADER_H, C_SURFACE);
    tft.drawFastHLine(0, HEADER_H, SCREEN_W, C_HAIRLINE);

    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(C_INK, C_SURFACE);
    tft.drawString("HYDRO HUB", 16, HEADER_H / 2, 4);

    const char *word;
    uint16_t col;
    switch (m.link) {
    case LinkState::Live:       word = "LINK OK";   col = C_GOOD;     break;
    case LinkState::Stale:      word = "OVERDUE";   col = C_WARNING;  break;
    case LinkState::Lost:       word = "LINK LOST"; col = C_CRITICAL; break;
    default:                    word = "WAITING";   col = C_INK_MUTED; break;
    }
    if (!m.radioOk) { word = "NO RADIO"; col = C_CRITICAL; }

    char age[16];
    ageText(m.ageMs, age, sizeof(age));

    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(C_INK_MUTED, C_SURFACE);
    tft.setTextPadding(110);
    tft.drawString(age, SCREEN_W - 122, HEADER_H / 2, 2);
    tft.setTextPadding(0);

    const int pw = 104;
    tft.fillRoundRect(SCREEN_W - 16 - pw, 5, pw, HEADER_H - 11, 5, col);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_SURFACE, col);
    tft.drawString(word, SCREEN_W - 16 - pw / 2, HEADER_H / 2, 2);
}

/* Sensor health: colour + glyph + name + word. Four cues, so no single one is
 * load-bearing. */
static void drawSensorPills(TFT_eSPI &tft, const DashModel &m)
{
    tft.fillRect(0, FOOTER_Y, SCREEN_W, FOOTER_H, C_SURFACE);
    tft.drawFastHLine(0, FOOTER_Y, SCREEN_W, C_HAIRLINE);

    struct { const char *name; uint8_t st; } s[3] = {
        { "LEVEL", m.stUs }, { "TEMP", m.stTp }, { "FLOW", m.stFl },
    };

    const int w = 150, gap = 7, y = FOOTER_Y + 8, h = FOOTER_H - 15;
    for (int i = 0; i < 3; ++i) {
        const int x = 8 + i * (w + gap);
        const uint16_t col = statusColour(s[i].st);

        tft.fillRoundRect(x, y, w, h, 5, C_PANEL);
        tft.fillRoundRect(x, y, 5, h, 2, col);          /* colour spine */

        tft.setTextDatum(ML_DATUM);
        tft.setTextColor(col, C_PANEL);
        tft.drawString(statusGlyph(s[i].st), x + 12, y + h / 2, 4);

        tft.setTextColor(C_INK, C_PANEL);
        tft.drawString(s[i].name, x + 36, y + h / 2 - 8, 2);

        tft.setTextColor(C_INK_DIM, C_PANEL);
        tft.setTextPadding(w - 44);
        tft.drawString(statusWord(s[i].st), x + 36, y + h / 2 + 9, 2);
        tft.setTextPadding(0);
    }
}

/* One line of plain language when something needs saying. This is the part a
 * non-technical user actually reads. */
static void drawBanner(TFT_eSPI &tft, const DashModel &m)
{
    const int y = STRIP_Y, h = 42;
    const char *msg = nullptr;
    uint16_t col = C_PANEL, ink = C_INK;

    if (!m.radioOk) {
        msg = "Radio hardware not responding"; col = C_CRITICAL; ink = C_SURFACE;
    } else if (m.link == LinkState::NeverHeard) {
        msg = "Waiting for the first reading from the tank";
        col = C_PANEL; ink = C_INK_DIM;
    } else if (m.link == LinkState::Lost) {
        msg = "No signal from the tank - data below is old";
        col = C_CRITICAL; ink = C_SURFACE;
    } else if (HN_ST_STATUS(m.stUs) == HN_W_ABSENT) {
        msg = "Level sensor is not connected"; col = C_CRITICAL; ink = C_SURFACE;
    } else if (HN_ST_STATUS(m.stUs) == HN_W_FAULT) {
        msg = "Level sensor fault - check the cable"; col = C_CRITICAL; ink = C_SURFACE;
    } else if (HN_ST_STATUS(m.stFl) == HN_W_FAULT) {
        msg = "Flow switch fault - water in the connector?"; col = C_SERIOUS; ink = C_SURFACE;
    } else if (HN_ST_STATUS(m.stUs) == HN_W_NO_TARGET) {
        msg = "No echo - tank may be full, or surface unreadable";
        col = C_SERIOUS; ink = C_SURFACE;
    } else if (m.gated) {
        msg = "Filling now - level settling, not exact"; col = C_WATER_DEEP; ink = C_INK;
    } else if (m.link == LinkState::Stale) {
        msg = "Reading is overdue"; col = C_WARNING; ink = C_SURFACE;
    } else if (m.tank.clamped_empty) {
        msg = "Reads past empty - check the tank height setting";
        col = C_WARNING; ink = C_SURFACE;
    } else if (HN_ST_STATUS(m.stUs) == HN_W_UNSTABLE) {
        msg = "Surface moving - reading is approximate"; col = C_WARNING; ink = C_SURFACE;
    } else if (m.flowState == HN_W_FLOW_FILLING) {
        msg = "Water is flowing in"; col = C_WATER_DEEP; ink = C_INK;
    } else if (m.tempAssumed) {
        msg = "No temperature - distance uses an assumed value";
        col = C_PANEL; ink = C_INK_DIM;
    }

    if (msg == nullptr) {
        tft.fillRect(RIGHT_X, y, RIGHT_W, h, C_SURFACE);
        return;
    }

    tft.fillRect(RIGHT_X, y, RIGHT_W, h, C_SURFACE);
    tft.fillRoundRect(RIGHT_X, y, RIGHT_W, h, 6, col);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(ink, col);
    tft.drawString(msg, RIGHT_X + RIGHT_W / 2, y + h / 2, 2);
}

/* ------------------------------------------------------------------------- */
/* Screens                                                                    */
/* ------------------------------------------------------------------------- */

static void drawMain(TFT_eSPI &tft, const DashModel &m, bool full)
{
    const bool trusted = m.tank.valid &&
                         (m.link == LinkState::Live) && !m.gated &&
                         HN_ST_STATUS(m.stUs) != HN_W_UNSTABLE;

    Fill mode = Fill::Hollow;
    if (m.tank.valid) mode = trusted ? Fill::Solid : Fill::Hatched;

    /* Ease the drawn level toward the real one: a jump looks like a glitch,
     * a slide reads as the tank actually changing. */
    const float target = m.tank.valid ? m.tank.level_pct : 0.0f;
    s_drawnPct += (target - s_drawnPct) * 0.25f;
    if (fabsf(target - s_drawnPct) < 0.4f) s_drawnPct = target;

    if (full || (int)(s_drawnPct * 2) != s_prev.gaugePct || m.tank.valid != s_prev.valid) {
        drawGauge(tft, s_drawnPct, mode, m.tank.clamped_full);
        s_prev.gaugePct = (int)(s_drawnPct * 2);
        s_prev.valid = m.tank.valid;
    }

    /* Hero: litres. The single number this product exists to answer. */
    const int32_t vol = m.tank.valid ? (int32_t)m.tank.volume_liters : -1;
    if (full || vol != s_prev.volume) {
        tft.fillRect(RIGHT_X, HERO_Y, RIGHT_W, 82, C_SURFACE);
        tft.setTextDatum(TL_DATUM);
        if (vol >= 0) {
            char buf[12];
            snprintf(buf, sizeof(buf), "%ld", (long)vol);
            tft.setTextColor(C_INK, C_SURFACE);
            const int wNum = tft.drawString(buf, RIGHT_X, HERO_Y, 6);
            tft.setTextColor(C_INK_DIM, C_SURFACE);
            tft.drawString("L", RIGHT_X + wNum + 8, HERO_Y + 22, 4);
        } else {
            tft.setTextColor(C_INK_MUTED, C_SURFACE);
            tft.drawString("--", RIGHT_X, HERO_Y, 6);
        }
        char sub[40];
        snprintf(sub, sizeof(sub), "of %lu L in %u tank%s",
                 (unsigned long)m.totalLiters, m.tankCount, m.tankCount == 1 ? "" : "s");
        tft.setTextColor(C_INK_MUTED, C_SURFACE);
        tft.drawString(sub, RIGHT_X, HERO_Y + 62, 2);
        s_prev.volume = vol;
    }

    /* Supporting statistics. */
    const int tw = (RIGHT_W - 2 * TILE_GAP) / 3;
    const int32_t lvl  = m.tank.valid ? (int32_t)(m.tank.level_pct + 0.5f) : -1;
    const int32_t wcm  = m.tank.valid ? (int32_t)(m.tank.water_height_cm + 0.5f) : -1;
    const int32_t tdec = m.tempValid ? (int32_t)(m.tempC * 10.0f) : -32768;

    if (full || lvl != s_prev.level) {
        char b[8];
        if (lvl >= 0) snprintf(b, sizeof(b), "%ld%%", (long)lvl); else snprintf(b, sizeof(b), "--");
        tile(tft, RIGHT_X, TILES_Y, tw, "LEVEL", b, lvl >= 0 ? C_INK : C_INK_MUTED);
        s_prev.level = lvl;
    }
    if (full || wcm != s_prev.waterCm) {
        char b[10];
        if (wcm >= 0) snprintf(b, sizeof(b), "%ld cm", (long)wcm); else snprintf(b, sizeof(b), "--");
        tile(tft, RIGHT_X + tw + TILE_GAP, TILES_Y, tw, "DEPTH", b, wcm >= 0 ? C_INK : C_INK_MUTED);
        s_prev.waterCm = wcm;
    }
    if (full || tdec != s_prev.tempDeci) {
        char b[12];
        if (m.tempValid) snprintf(b, sizeof(b), "%.1f C", m.tempC); else snprintf(b, sizeof(b), "--");
        tile(tft, RIGHT_X + 2 * (tw + TILE_GAP), TILES_Y, tw, "TEMP", b,
             m.tempValid ? C_INK : C_INK_MUTED);
        s_prev.tempDeci = tdec;
    }

    if (full || m.gated != s_prev.gated || m.flowState != s_prev.flow ||
        m.stUs != s_prev.stUs || m.stFl != s_prev.stFl || (int8_t)m.link != s_prev.link) {
        drawBanner(tft, m);
        s_prev.gated = m.gated;
        s_prev.flow = m.flowState;
    }

    if (full || m.stUs != s_prev.stUs || m.stTp != s_prev.stTp || m.stFl != s_prev.stFl) {
        drawSensorPills(tft, m);
        s_prev.stUs = m.stUs; s_prev.stTp = m.stTp; s_prev.stFl = m.stFl;
    }
}

static void drawDiag(TFT_eSPI &tft, const DashModel &m, bool full)
{
    if (!full && m.seq == s_prev.seq) return;
    s_prev.seq = m.seq;

    tft.fillRect(0, HEADER_H + 1, SCREEN_W, SCREEN_H - HEADER_H - 1, C_SURFACE);

    char b[64];
    int y = HEADER_H + 12;
    const int lh = 22;

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_INK_MUTED, C_SURFACE);
    tft.drawString("RAW FROM THE NODE - nothing here is interpreted", 16, y, 2);
    y += lh + 4;

    tft.setTextColor(C_INK, C_SURFACE);
    snprintf(b, sizeof(b), "node %u   seq %u   packet %u B", m.nodeId, m.seq, (unsigned)HN_PACKET_BYTES);
    tft.drawString(b, 16, y, 2); y += lh;
    snprintf(b, sizeof(b), "echo %u us    -> %.1f cm", m.echoUs, m.tank.distance_cm);
    tft.drawString(b, 16, y, 2); y += lh;
    snprintf(b, sizeof(b), "temp raw %d  -> %.2f C%s", m.tempRaw, m.tempC,
             m.tempAssumed ? "  (assumed)" : "");
    tft.drawString(b, 16, y, 2); y += lh;
    snprintf(b, sizeof(b), "flow adc %u/255   state %s", m.flowAdc,
             m.flowState == HN_W_FLOW_FILLING ? "FILLING" :
             m.flowState == HN_W_FLOW_IDLE ? "idle" : "unknown");
    tft.drawString(b, 16, y, 2); y += lh + 8;

    tft.setTextColor(C_INK_MUTED, C_SURFACE);
    tft.drawString("LINK", 16, y, 2); y += lh;
    tft.setTextColor(C_INK, C_SURFACE);
    snprintf(b, sizeof(b), "rssi %.0f dBm   snr %.1f dB", m.rssi, m.snr);
    tft.drawString(b, 16, y, 2); y += lh;
    snprintf(b, sizeof(b), "ok %lu  missed %lu  bad %lu  foreign %lu",
             (unsigned long)m.accepted, (unsigned long)m.missed,
             (unsigned long)m.rejected, (unsigned long)m.foreign);
    tft.drawString(b, 16, y, 2); y += lh + 8;

    /* Decoded status names beside the raw bytes: the fastest way to tell a
     * protocol bug from a genuine sensor fault. */
    tft.setTextColor(C_INK_MUTED, C_SURFACE);
    tft.drawString("SENSORS", 16, y, 2); y += lh;
    struct { const char *n; uint8_t st; } s[3] = {
        {"level", m.stUs}, {"temp", m.stTp}, {"flow", m.stFl} };
    for (int i = 0; i < 3; ++i) {
        tft.setTextColor(statusColour(s[i].st), C_SURFACE);
        snprintf(b, sizeof(b), "%-6s %-9s presence %u   (0x%02X)",
                 s[i].n, statusWord(s[i].st), HN_ST_PRESENCE(s[i].st), s[i].st);
        tft.drawString(b, 16, y, 2);
        y += lh;
    }
}

/* ------------------------------------------------------------------------- */
/* Public                                                                     */
/* ------------------------------------------------------------------------- */

void dashboardBegin(TFT_eSPI &tft)
{
    tft.init();
    tft.setRotation(TFT_ROTATION);
    tft.fillScreen(C_SURFACE);
    dashboardInvalidate();
}

void dashboardInvalidate() { s_full = true; s_prev = Prev(); }

DashScreen dashboardScreen() { return s_screen; }

void dashboardSetScreen(TFT_eSPI &tft, DashScreen s)
{
    s_screen = s;
    tft.fillScreen(C_SURFACE);
    dashboardInvalidate();
}

void dashboardRender(TFT_eSPI &tft, const DashModel &m)
{
    const bool full = s_full;
    if (full) tft.fillScreen(C_SURFACE);

    /* The header carries link state, which changes on its own timetable, so it
     * is refreshed whenever the state or the age bucket moves. */
    const int32_t bucket = (m.ageMs == UINT32_MAX) ? -1 : (int32_t)(m.ageMs / 1000UL);
    if (full || (int8_t)m.link != s_prev.link || bucket != s_prev.ageBucket) {
        drawHeader(tft, m);
        s_prev.link = (int8_t)m.link;
        s_prev.ageBucket = bucket;
    }

    if (s_screen == DashScreen::Main) drawMain(tft, m, full);
    else                              drawDiag(tft, m, full);

    s_full = false;
}
