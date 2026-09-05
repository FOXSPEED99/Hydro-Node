#include "Dashboard.h"
#include "config.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Fonts                                                                      */
/* ------------------------------------------------------------------------- */
/*
 * Only fonts 2 and 4 are used, scaled with setTextSize() where something needs
 * to be big.
 *
 * TFT_eSPI only compiles in the fonts its User_Setup asks for, and 6, 7 and 8
 * are commonly left out - they are large and digit-only. Drawing with a font
 * that was not loaded does not fail loudly: it draws nothing and returns a
 * width of zero, so anything positioned after it lands on top of something
 * else. That is exactly what produced a missing litres figure and a stray "L"
 * floating in the middle of the screen. Fonts 2 and 4 are in every setup.
 */
#define F_SMALL   2      /* ~16 px */
#define F_MED     4      /* ~26 px */

/* ------------------------------------------------------------------------- */
/* Layout                                                                     */
/* ------------------------------------------------------------------------- */
#define HEADER_H     34

#define GAUGE_X      16
#define GAUGE_Y      48
#define GAUGE_W      126
#define GAUGE_H      204

#define RIGHT_X      162
#define RIGHT_W      (SCREEN_W - RIGHT_X - 16)

#define HERO_Y       48
#define HERO_H       56
#define SUB_Y        108
#define TILES_Y      140
#define TILE_H       60
#define TILE_GAP     8
#define BANNER_Y     212
#define BANNER_H     40

#define FOOTER_Y     262
#define FOOTER_H     (SCREEN_H - FOOTER_Y)

static DashScreen s_screen = DashScreen::Main;
static bool  s_full = true;
static float s_drawnPct = 0.0f;

/*
 * The gauge is drawn into an off-screen sprite and pushed in one operation.
 * Drawing it piece by piece straight to the panel is what made the water
 * animation flicker and leave stray lines behind: every frame briefly showed a
 * half-erased tank, and text that moved with the water line left its previous
 * position behind. 126x204 at 16 bpp is ~51 kB, against ~300 kB free.
 */
static TFT_eSprite *s_gauge = nullptr;
static bool s_gaugeReady = false;

struct Prev {
    int32_t volume = -1, level = -1, tempDeci = -32768;
    int32_t gaugePct = -1;
    uint8_t stUs = 0xFF, stTp = 0xFF, stFl = 0xFF, flow = 0xFF;
    int8_t  link = -1;
    bool    gated = false, valid = false, clampFull = false;
    char    age[16] = "";
    char    banner[64] = "";
    uint16_t seq = 0xFFFF;
    bool    subDrawn = false;
};
static Prev s_prev;

/* ------------------------------------------------------------------------- */
/* Status vocabulary                                                          */
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

static const char *statusWord(uint8_t st)
{
    switch (HN_ST_STATUS(st)) {
    case HN_W_OK:           return "ok";
    case HN_W_NOT_MEASURED: return "not read";
    case HN_W_UNSTABLE:     return "noisy";
    case HN_W_OUT_OF_RANGE: return "out of range";
    case HN_W_NO_TARGET:    return "no echo";
    case HN_W_FAULT:        return "FAULT";
    case HN_W_ABSENT:       return "MISSING";
    default:                return "unknown";
    }
}

static void ageText(uint32_t ms, char *buf, size_t n)
{
    if (ms == UINT32_MAX)       snprintf(buf, n, "no data yet");
    else if (ms < 60000UL)      snprintf(buf, n, "%lus ago", (unsigned long)(ms / 1000UL));
    else if (ms < 3600000UL)    snprintf(buf, n, "%lum ago", (unsigned long)(ms / 60000UL));
    else                        snprintf(buf, n, "%luh ago", (unsigned long)(ms / 3600000UL));
}

/* ------------------------------------------------------------------------- */
/* The tank gauge, rendered into the sprite                                   */
/* ------------------------------------------------------------------------- */

enum class Fill : uint8_t { Solid, Hatched, Hollow };

static void drawGaugeInto(TFT_eSPI &g, int ox, int oy, float pct, Fill mode, bool clampedFull)
{
    const int w = GAUGE_W, h = GAUGE_H;

    g.fillRect(ox, oy, w, h, C_SURFACE);
    g.drawRoundRect(ox, oy, w, h, 8, C_HAIRLINE);
    g.drawRoundRect(ox + 1, oy + 1, w - 2, h - 2, 7, C_HAIRLINE);

    const int inX = ox + 4, inY = oy + 4, inW = w - 8, inH = h - 8;

    if (mode == Fill::Hollow) {
        /* No trustworthy reading. Deliberately not "0%" - an empty gauge with
         * a number on it would claim the tank is empty. */
        g.setTextDatum(MC_DATUM);
        g.setTextColor(C_INK_MUTED, C_SURFACE);
        g.setTextSize(2);
        g.drawString("--", ox + w / 2, oy + h / 2 - 14, F_MED);
        g.setTextSize(1);
        g.drawString("no reading", ox + w / 2, oy + h / 2 + 22, F_SMALL);
        return;
    }

    const int fillH = (int)((float)inH * pct / 100.0f + 0.5f);
    const int fillY = inY + inH - fillH;

    if (fillH > 0) {
        if (mode == Fill::Solid) {
            g.fillRect(inX, fillY, inW, fillH, C_WATER_DEEP);
            const int band = (fillH < 8) ? fillH : 8;
            g.fillRect(inX, fillY, inW, band, C_WATER);   /* the water line */
        } else {
            for (int yy = fillY; yy < fillY + fillH; yy += 5) {
                g.drawFastHLine(inX, yy, inW, C_WATER_GHOST);
            }
            g.drawFastHLine(inX, fillY, inW, C_WATER);
        }
    }

    /* Quarter marks, recessive. */
    for (int q = 1; q <= 3; ++q) {
        const int ty = inY + inH - (inH * q / 4);
        g.drawFastHLine(inX, ty, 7, C_HAIRLINE);
        g.drawFastHLine(inX + inW - 7, ty, 7, C_HAIRLINE);
    }

    /*
     * The percentage sits at a FIXED position, not one that follows the water
     * line. Text that moves with the level cannot be erased cleanly by the
     * next frame, and that is what left fragments of old numbers scattered
     * inside the tank.
     */
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)(pct + 0.5f));
    g.setTextDatum(MC_DATUM);
    g.setTextColor(C_INK);
    g.setTextSize(2);
    g.drawString(buf, ox + w / 2, oy + h / 2, F_MED);
    g.setTextSize(1);

    if (clampedFull) {
        g.setTextDatum(TC_DATUM);
        g.setTextColor(C_WARNING);
        g.drawString("OVER FULL", ox + w / 2, inY + 4, F_SMALL);
    }
}

static void renderGauge(TFT_eSPI &tft, float pct, Fill mode, bool clampedFull)
{
    if (s_gaugeReady && s_gauge != nullptr) {
        s_gauge->fillSprite(C_SURFACE);
        drawGaugeInto(*s_gauge, 0, 0, pct, mode, clampedFull);
        s_gauge->pushSprite(GAUGE_X, GAUGE_Y);
    } else {
        drawGaugeInto(tft, GAUGE_X, GAUGE_Y, pct, mode, clampedFull);
    }
}

/* ------------------------------------------------------------------------- */
/* Header                                                                     */
/* ------------------------------------------------------------------------- */

#define PILL_W   108
#define PILL_X   (SCREEN_W - 14 - PILL_W)
#define AGE_W    120
#define AGE_X    (PILL_X - 10)

static void drawHeaderStatic(TFT_eSPI &tft)
{
    tft.fillRect(0, 0, SCREEN_W, HEADER_H + 1, C_SURFACE);
    tft.drawFastHLine(0, HEADER_H, SCREEN_W, C_HAIRLINE);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(C_INK, C_SURFACE);
    tft.drawString("HYDRO HUB", 16, HEADER_H / 2, F_MED);
}

/*
 * Only the two things that actually change are repainted, and each is written
 * over its own background rather than clearing a rectangle first.
 *
 * The whole header used to be erased and redrawn whenever the age string
 * changed - which is once a second - so the title and the pill were being
 * destroyed and rebuilt 60 times a minute. That was the visible flicker across
 * the top of the screen.
 */
static void drawHeaderLive(TFT_eSPI &tft, const DashModel &m, bool full)
{
    char age[16];
    ageText(m.ageMs, age, sizeof(age));
    if (full || strcmp(age, s_prev.age) != 0) {
        tft.setTextDatum(MR_DATUM);
        tft.setTextColor(C_INK_MUTED, C_SURFACE);
        tft.setTextPadding(AGE_W);
        tft.drawString(age, AGE_X, HEADER_H / 2, F_SMALL);
        tft.setTextPadding(0);
        strncpy(s_prev.age, age, sizeof(s_prev.age) - 1);
    }

    if (full || (int8_t)m.link != s_prev.link) {
        const char *word; uint16_t col;
        switch (m.link) {
        case LinkState::Live:  word = "LINK OK";   col = C_GOOD;      break;
        case LinkState::Stale: word = "OVERDUE";   col = C_WARNING;   break;
        case LinkState::Lost:  word = "LINK LOST"; col = C_CRITICAL;  break;
        default:               word = "WAITING";   col = C_INK_MUTED; break;
        }
        if (!m.radioOk) { word = "NO RADIO"; col = C_CRITICAL; }

        tft.fillRoundRect(PILL_X, 5, PILL_W, HEADER_H - 11, 5, col);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(C_SURFACE, col);
        tft.drawString(word, PILL_X + PILL_W / 2, HEADER_H / 2, F_SMALL);
        s_prev.link = (int8_t)m.link;
    }
}

/* ------------------------------------------------------------------------- */
/* Tiles and pills                                                            */
/* ------------------------------------------------------------------------- */

static void tile(TFT_eSPI &tft, int x, int y, int w, const char *label,
                 const char *value, uint16_t valueColour)
{
    tft.fillRoundRect(x, y, w, TILE_H, 6, C_PANEL);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_INK_MUTED, C_PANEL);
    tft.drawString(label, x + 10, y + 8, F_SMALL);
    tft.setTextDatum(BL_DATUM);
    tft.setTextColor(valueColour, C_PANEL);
    tft.drawString(value, x + 10, y + TILE_H - 9, F_MED);
}

/*
 * Sensor health. Colour, a dot, the sensor name and a word - four cues, so no
 * single one is load-bearing and the row still reads correctly for a
 * colour-blind viewer.
 *
 * The dot now has a column of its own. It previously sat 24 px from the text,
 * which at font 4 meant the glyph and the label overlapped.
 */
static void drawSensorPills(TFT_eSPI &tft, const DashModel &m)
{
    tft.fillRect(0, FOOTER_Y, SCREEN_W, FOOTER_H, C_SURFACE);
    tft.drawFastHLine(0, FOOTER_Y, SCREEN_W, C_HAIRLINE);

    struct { const char *name; uint8_t st; } s[3] = {
        { "LEVEL", m.stUs }, { "TEMP", m.stTp }, { "FLOW", m.stFl },
    };

    const int margin = 10, gap = 8;
    const int w = (SCREEN_W - 2 * margin - 2 * gap) / 3;
    const int y = FOOTER_Y + 7, h = FOOTER_H - 14;

    for (int i = 0; i < 3; ++i) {
        const int x = margin + i * (w + gap);
        const uint16_t col = statusColour(s[i].st);

        tft.fillRoundRect(x, y, w, h, 6, C_PANEL);
        tft.fillRoundRect(x, y, 4, h, 2, col);          /* colour spine */

        const int dotX = x + 22, dotCY = y + h / 2;
        tft.fillCircle(dotX, dotCY, 8, col);
        tft.fillCircle(dotX, dotCY, 4, C_PANEL);         /* ring, reads at distance */

        const int textX = x + 40;
        tft.setTextDatum(BL_DATUM);
        tft.setTextColor(C_INK, C_PANEL);
        tft.drawString(s[i].name, textX, dotCY, F_SMALL);

        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(col, C_PANEL);
        tft.setTextPadding(w - (textX - x) - 8);
        tft.drawString(statusWord(s[i].st), textX, dotCY + 3, F_SMALL);
        tft.setTextPadding(0);
    }
}

static void bannerText(const DashModel &m, char *out, size_t n, uint16_t *col, uint16_t *ink)
{
    const char *msg = "";
    *col = C_PANEL; *ink = C_INK_DIM;

    if (!m.radioOk)                                   { msg = "Radio hardware not responding";               *col = C_CRITICAL; *ink = C_SURFACE; }
    else if (m.link == LinkState::NeverHeard)         { msg = "Waiting for the first reading from the tank"; }
    else if (m.link == LinkState::Lost)               { msg = "No signal from the tank - data below is old"; *col = C_CRITICAL; *ink = C_SURFACE; }
    else if (HN_ST_STATUS(m.stUs) == HN_W_ABSENT)     { msg = "Level sensor is not connected";               *col = C_CRITICAL; *ink = C_SURFACE; }
    else if (HN_ST_STATUS(m.stUs) == HN_W_FAULT)      { msg = "Level sensor fault - check the cable";        *col = C_CRITICAL; *ink = C_SURFACE; }
    else if (HN_ST_STATUS(m.stFl) == HN_W_FAULT)      { msg = "Flow switch fault - water in the connector?"; *col = C_SERIOUS;  *ink = C_SURFACE; }
    else if (HN_ST_STATUS(m.stUs) == HN_W_NO_TARGET)  { msg = "No echo - tank may be full, or unreadable";   *col = C_SERIOUS;  *ink = C_SURFACE; }
    else if (m.gated)                                 { msg = "Filling now - level settling, not exact";     *col = C_WATER_DEEP; *ink = C_INK; }
    else if (m.link == LinkState::Stale)              { msg = "Reading is overdue";                          *col = C_WARNING;  *ink = C_SURFACE; }
    else if (m.tank.clamped_empty)                    { msg = "Reads past empty - check the tank height";    *col = C_WARNING;  *ink = C_SURFACE; }
    else if (HN_ST_STATUS(m.stUs) == HN_W_UNSTABLE)   { msg = "Surface moving - reading is approximate";     *col = C_WARNING;  *ink = C_SURFACE; }
    else if (m.flowState == HN_W_FLOW_FILLING)        { msg = "Water is flowing in";                         *col = C_WATER_DEEP; *ink = C_INK; }
    else if (m.tempAssumed)                           { msg = "No temperature - distance uses an estimate"; }

    strncpy(out, msg, n - 1);
    out[n - 1] = '\0';
}

/* ------------------------------------------------------------------------- */
/* Main screen                                                                */
/* ------------------------------------------------------------------------- */

static void drawMain(TFT_eSPI &tft, const DashModel &m, bool full)
{
    const bool trusted = m.tank.valid && (m.link == LinkState::Live) && !m.gated &&
                         HN_ST_STATUS(m.stUs) != HN_W_UNSTABLE;
    Fill mode = Fill::Hollow;
    if (m.tank.valid) mode = trusted ? Fill::Solid : Fill::Hatched;

    const float target = m.tank.valid ? m.tank.level_pct : 0.0f;
    s_drawnPct += (target - s_drawnPct) * 0.25f;
    if (fabsf(target - s_drawnPct) < 0.4f) s_drawnPct = target;

    const int32_t gpct = (int32_t)(s_drawnPct * 2.0f);
    if (full || gpct != s_prev.gaugePct || m.tank.valid != s_prev.valid ||
        m.tank.clamped_full != s_prev.clampFull) {
        renderGauge(tft, s_drawnPct, mode, m.tank.clamped_full);
        s_prev.gaugePct = gpct;
        s_prev.valid = m.tank.valid;
        s_prev.clampFull = m.tank.clamped_full;
    }

    /* Hero: litres. The one number this product exists to answer. */
    const int32_t vol = m.tank.valid ? (int32_t)m.tank.volume_liters : -1;
    if (full || vol != s_prev.volume) {
        tft.fillRect(RIGHT_X, HERO_Y, RIGHT_W, HERO_H, C_SURFACE);
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(2);
        int wNum;
        if (vol >= 0) {
            char buf[12];
            snprintf(buf, sizeof(buf), "%ld", (long)vol);
            tft.setTextColor(C_INK, C_SURFACE);
            wNum = tft.drawString(buf, RIGHT_X, HERO_Y, F_MED);
        } else {
            tft.setTextColor(C_INK_MUTED, C_SURFACE);
            wNum = tft.drawString("--", RIGHT_X, HERO_Y, F_MED);
        }
        tft.setTextSize(1);
        tft.setTextColor(C_INK_DIM, C_SURFACE);
        tft.drawString("L", RIGHT_X + wNum + 10, HERO_Y + 24, F_MED);
        s_prev.volume = vol;
    }

    if (full || !s_prev.subDrawn) {
        char sub[48];
        snprintf(sub, sizeof(sub), "of %lu L in %u tank%s",
                 (unsigned long)m.totalLiters, m.tankCount, m.tankCount == 1 ? "" : "s");
        tft.fillRect(RIGHT_X, SUB_Y, RIGHT_W, 20, C_SURFACE);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(C_INK_MUTED, C_SURFACE);
        tft.drawString(sub, RIGHT_X, SUB_Y, F_SMALL);
        s_prev.subDrawn = true;
    }

    /* Tiles. Depth is gone - it was a working number nobody needs, and the
     * space is better spent on flow state, which the level cannot be trusted
     * during. */
    const int tw = (RIGHT_W - 2 * TILE_GAP) / 3;
    const int32_t lvl  = m.tank.valid ? (int32_t)(m.tank.level_pct + 0.5f) : -1;
    const int32_t tdec = m.tempValid ? (int32_t)(m.tempC * 10.0f) : -32768;

    if (full || lvl != s_prev.level) {
        char b[8];
        if (lvl >= 0) snprintf(b, sizeof(b), "%ld%%", (long)lvl); else snprintf(b, sizeof(b), "--");
        tile(tft, RIGHT_X, TILES_Y, tw, "LEVEL", b, lvl >= 0 ? C_INK : C_INK_MUTED);
        s_prev.level = lvl;
    }
    if (full || tdec != s_prev.tempDeci) {
        char b[12];
        if (m.tempValid) snprintf(b, sizeof(b), "%.1f C", m.tempC); else snprintf(b, sizeof(b), "--");
        tile(tft, RIGHT_X + tw + TILE_GAP, TILES_Y, tw, "TEMP", b,
             m.tempValid ? C_INK : C_INK_MUTED);
        s_prev.tempDeci = tdec;
    }
    if (full || m.flowState != s_prev.flow) {
        const char *fw = (m.flowState == HN_W_FLOW_FILLING) ? "FILLING"
                       : (m.flowState == HN_W_FLOW_IDLE)    ? "idle" : "--";
        tile(tft, RIGHT_X + 2 * (tw + TILE_GAP), TILES_Y, tw, "FLOW", fw,
             (m.flowState == HN_W_FLOW_FILLING) ? C_WATER : C_INK_DIM);
        s_prev.flow = m.flowState;
    }

    char msg[64]; uint16_t bcol, bink;
    bannerText(m, msg, sizeof(msg), &bcol, &bink);
    if (full || strcmp(msg, s_prev.banner) != 0) {
        tft.fillRect(RIGHT_X, BANNER_Y, RIGHT_W, BANNER_H, C_SURFACE);
        if (msg[0] != '\0') {
            tft.fillRoundRect(RIGHT_X, BANNER_Y, RIGHT_W, BANNER_H, 6, bcol);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(bink, bcol);
            tft.drawString(msg, RIGHT_X + RIGHT_W / 2, BANNER_Y + BANNER_H / 2, F_SMALL);
        }
        strncpy(s_prev.banner, msg, sizeof(s_prev.banner) - 1);
    }

    if (full || m.stUs != s_prev.stUs || m.stTp != s_prev.stTp || m.stFl != s_prev.stFl) {
        drawSensorPills(tft, m);
        s_prev.stUs = m.stUs; s_prev.stTp = m.stTp; s_prev.stFl = m.stFl;
    }
}

/* ------------------------------------------------------------------------- */
/* Diagnostics                                                                */
/* ------------------------------------------------------------------------- */

static void drawDiag(TFT_eSPI &tft, const DashModel &m, bool full)
{
    if (!full && m.seq == s_prev.seq) return;
    s_prev.seq = m.seq;

    tft.fillRect(0, HEADER_H + 1, SCREEN_W, SCREEN_H - HEADER_H - 1, C_SURFACE);

    char b[72];
    int y = HEADER_H + 10;
    const int lh = 21;

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_INK_MUTED, C_SURFACE);
    tft.drawString("RAW FROM THE NODE - nothing here is interpreted", 14, y, F_SMALL);
    y += lh + 4;

    tft.setTextColor(C_INK, C_SURFACE);
    snprintf(b, sizeof(b), "node %u   seq %u   packet %u B", m.nodeId, m.seq, (unsigned)HN_PACKET_BYTES);
    tft.drawString(b, 14, y, F_SMALL); y += lh;
    snprintf(b, sizeof(b), "echo %u us  ->  %.1f cm", m.echoUs, m.tank.distance_cm);
    tft.drawString(b, 14, y, F_SMALL); y += lh;
    snprintf(b, sizeof(b), "temp raw %d  ->  %.2f C%s", m.tempRaw, m.tempC,
             m.tempAssumed ? "  (assumed)" : "");
    tft.drawString(b, 14, y, F_SMALL); y += lh;
    snprintf(b, sizeof(b), "water %.1f cm   level %.1f%%   %lu L",
             m.tank.water_height_cm, m.tank.level_pct, (unsigned long)m.tank.volume_liters);
    tft.drawString(b, 14, y, F_SMALL); y += lh;
    snprintf(b, sizeof(b), "flow adc %u/255   state %s", m.flowAdc,
             m.flowState == HN_W_FLOW_FILLING ? "FILLING" :
             m.flowState == HN_W_FLOW_IDLE ? "idle" : "unknown");
    tft.drawString(b, 14, y, F_SMALL); y += lh + 6;

    tft.setTextColor(C_INK_MUTED, C_SURFACE);
    tft.drawString("LINK", 14, y, F_SMALL); y += lh;
    tft.setTextColor(C_INK, C_SURFACE);
    snprintf(b, sizeof(b), "rssi %.0f dBm   snr %.1f dB", m.rssi, m.snr);
    tft.drawString(b, 14, y, F_SMALL); y += lh;
    snprintf(b, sizeof(b), "ok %lu   missed %lu   bad %lu   foreign %lu",
             (unsigned long)m.accepted, (unsigned long)m.missed,
             (unsigned long)m.rejected, (unsigned long)m.foreign);
    tft.drawString(b, 14, y, F_SMALL); y += lh + 6;

    tft.setTextColor(C_INK_MUTED, C_SURFACE);
    tft.drawString("SENSORS", 14, y, F_SMALL); y += lh;
    struct { const char *n; uint8_t st; } s[3] = {
        {"level", m.stUs}, {"temp", m.stTp}, {"flow", m.stFl} };
    for (int i = 0; i < 3; ++i) {
        tft.setTextColor(statusColour(s[i].st), C_SURFACE);
        snprintf(b, sizeof(b), "%-6s %-13s presence %u   (0x%02X)",
                 s[i].n, statusWord(s[i].st), HN_ST_PRESENCE(s[i].st), s[i].st);
        tft.drawString(b, 14, y, F_SMALL);
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

    if (s_gauge == nullptr) s_gauge = new TFT_eSprite(&tft);
    s_gauge->setColorDepth(16);
    s_gaugeReady = (s_gauge->createSprite(GAUGE_W, GAUGE_H) != nullptr);
    if (!s_gaugeReady) {
        /* Falls back to drawing straight to the panel: correct, just not as
         * smooth. Worth knowing about rather than silently looking worse. */
        Serial.println("[UI] gauge sprite allocation failed - drawing direct");
    }

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
    if (full) {
        tft.fillScreen(C_SURFACE);
        drawHeaderStatic(tft);
    }

    drawHeaderLive(tft, m, full);

    if (s_screen == DashScreen::Main) drawMain(tft, m, full);
    else                              drawDiag(tft, m, full);

    s_full = false;
}
