#include "DisplayUI.h"

#include "DebugLog.h"
#include "config.h"
#include "ArabicLabels.h"


#if ENABLE_TFT_DASHBOARD
#if ENABLE_QR_SETUP_SCREEN
#include <qrcode.h>
#endif

#include <LittleFS.h>
#include <PNGdec.h>

namespace {
static constexpr uint16_t COLOR_BG = 0x0841;
static constexpr uint16_t COLOR_CARD = 0x10A2;
static constexpr uint16_t COLOR_CARD_2 = 0x18E3;
static constexpr uint16_t COLOR_TEXT = 0xFFFF;
static constexpr uint16_t COLOR_MUTED = 0x9CF3;
static constexpr uint16_t COLOR_CYAN = 0x07FF;
static constexpr uint16_t COLOR_GREEN = 0x07E0;
static constexpr uint16_t COLOR_ORANGE = 0xFD20;
static constexpr uint16_t COLOR_RED = 0xF800;
static constexpr uint16_t COLOR_DARK_GRAY = 0x18E3;
static constexpr uint16_t COLOR_LIGHT_BLUE = 0x5DFF;
static constexpr uint16_t COLOR_DARK_BLUE = 0x033F;
static constexpr uint16_t COLOR_PUMP_BLUE = 0x3CBA;  // matches the pump body ring (RGB 59,151,210)

// ── Pump graphic geometry ────────────────────────────────────────
// The pump is drawn in three layers inside the right-hand card:
//   1. Body     — Pump-No-Filling.png (blue frame + empty circle), static.
//   2. Water    — drawn by us inside the circle, wavy surface, level by state.
//   3. Blade    — pump.png, spins in the circle centre when the pump is on.
// All measurements below were taken from the artwork (circle centre/radius in
// the body PNG; hub/centre in the blade PNG).
static constexpr int PUMP_BODY_W = 121;   // Pump-No-Filling.png native size
static constexpr int PUMP_BODY_H = 175;
static constexpr int PUMP_BODY_X = 296;   // top-left on screen (centres body in the 245..470 card)
static constexpr int PUMP_BODY_Y = 68;

static constexpr int PUMP_CIRCLE_IMG_CX = 64;   // circle centre X within the body PNG
static constexpr int PUMP_CIRCLE_IMG_CY = 87;   // circle centre Y within the body PNG
static constexpr int PUMP_CIRCLE_R      = 46;   // water disc radius (kept just inside the ring)

static constexpr int PUMP_CX = PUMP_BODY_X + PUMP_CIRCLE_IMG_CX;   // circle centre on screen X (360)
static constexpr int PUMP_CY = PUMP_BODY_Y + PUMP_CIRCLE_IMG_CY;   // circle centre on screen Y (155)

static constexpr int PUMP_SCENE     = 2 * PUMP_CIRCLE_R + 1;  // composite sprite side (93)
static constexpr int PUMP_SCENE_PIV = PUMP_CIRCLE_R;          // its centre (46,46)

static constexpr int BLADE_HUB_X   = 35;   // hub centre within pump.png (70x74)
static constexpr int BLADE_HUB_Y   = 37;
static constexpr int BLADE_CANVAS  = 80;   // square canvas the blade is decoded into
static constexpr int BLADE_PIV     = BLADE_CANVAS / 2;   // 40
static constexpr int BLADE_MASK_R  = 39;   // trim blade canvas to this radius

// Water surface colours (sampled from the reference fill artwork): light cyan at
// the top of the water column, deeper cyan toward the bottom.
static constexpr uint16_t WATER_TOP  = 0x7F3F;
static constexpr uint16_t WATER_DEEP = 0x0E7F;

// Icon configuration for LittleFS PNG system
struct IconConfig {
  const char* file;      // PNG filename in LittleFS
  int x, y;              // Draw position
  int size;              // Target draw size in pixels
  int nativeW, nativeH;  // Native PNG dimensions
};

enum IconID {
  ICON_WATER_DROP,
  ICON_PUMP_BODY,
  ICON_PUMP,
  ICON_POWER_SWITCH,
  ICON_COUNT
};

// Icon configuration table - edit this to change positions, sizes, or files
static constexpr IconConfig iconConfigs[ICON_COUNT] = {
  { "/water_drop.png", 195, 60, 30, 30, 30 },                       // Water drop: native 30x30
  { "/Pump-No-Filling.png", PUMP_BODY_X, PUMP_BODY_Y, PUMP_BODY_W, PUMP_BODY_W, PUMP_BODY_H }, // Pump body (static frame + empty circle)
  { "/pump.png", 0, 0, 70, 70, 74 },                                // Pump blade: decoded separately, pos unused
  { "/power_switch.png", 0, 0, 32, 32, 32 }                         // Power switch: native 32x32, position set at draw time
};

// LittleFS and PNGdec state
static bool littlefsMounted = false;
static PNG png;
static TFT_eSprite *iconSprite = nullptr;
static fs::File pngFile;

// pngDraw decodes into this sprite; drawIconFromPNG/ensurePumpAssets point it
// at the right target before calling png.decode(). Offsets let us place the
// decoded image at an arbitrary origin inside a larger sprite.
static TFT_eSprite *decodeSprite = nullptr;
static int pngOffX = 0;
static int pngOffY = 0;

// ── Pump animation: spinning blade + wavy water ──────────────────
// The blade (pump.png) is decoded once, hub-centred, into bladeSprite with real
// transparency in its gaps. Every frame the scene sprite is rebuilt in RAM
// (empty card + wavy water disc), the rotated blade is composited on top, and
// the finished disc is pushed to the panel in one pass — so the glass never
// shows a half-drawn frame (no tearing) and water shows through the blade gaps.
static TFT_eSprite *bladeSprite = nullptr;   // decoded blade, hub at centre
static TFT_eSprite *sceneSprite = nullptr;   // off-screen composite (water + blade)
static bool  pumpAssetsReady   = false;
static float bladeAngle        = 0.0f;   // current blade rotation, degrees
static float bladeSpeed        = 0.0f;   // current angular velocity, deg/s
static float wavePhase         = 0.0f;   // animates the water surface
static float waterLevel        = 0.0f;   // displayed fill 0..1 (eased toward target)
static uint32_t pumpLastStepMs = 0;      // timestamp of previous animation step
static uint16_t waterColY[PUMP_SCENE];   // precomputed vertical water gradient
static PumpFillState pumpFillState = PumpFillState::None; // set via LoRa / setter

// ═══════════════════════════════════════════════════════════════
//  BLADE SPEED — one number, in RPM (full turns per minute).
//  Smoothness rule of thumb: the spin looks liquid while the blade moves
//  no more than ~2 degrees per frame. At the ~80fps animation rate that means
//  up to ~27 RPM; above that the per-frame jump itself becomes visible — the
//  panel can't accept frames any faster over SPI, so that is the hard ceiling.
// ═══════════════════════════════════════════════════════════════
static constexpr float PUMP_RPM       = 30.0f;
static constexpr float PUMP_MAX_SPEED = PUMP_RPM * 6.0f;   // deg/s (RPM * 360 / 60)

// Spin-up / spin-down feel (independent of top speed):
static constexpr float PUMP_ACCEL     = 500.0f;   // deg/s^2 ramp up   (~brisk start)
static constexpr float PUMP_DECEL     = 75.0f;    // deg/s^2 ramp down  (~slow coast to stop)

// ── DEMO: no sender device yet ───────────────────────────────────
// With PUMP_FILL_DEMO_CYCLE == true the circle cycles None → Weak → Good →
// Strong (each shown for PUMP_FILL_DEMO_MS) so you can see all four states with
// no sender. Set it false and either edit PUMP_FILL_FIXED or call
// DisplayUI::setPumpFillState(...) from your LoRa handler to drive it for real.
static constexpr bool           PUMP_FILL_DEMO_CYCLE = false;  // production: driven by real telemetry
static constexpr PumpFillState  PUMP_FILL_FIXED      = PumpFillState::Good;
static constexpr uint32_t       PUMP_FILL_DEMO_MS    = 3500;

// Working-time timer blink: once per cycle it winks off briefly, for a live feel.
static constexpr uint32_t PUMP_TIMER_BLINK_MS     = 5000;   // full blink cycle
static constexpr uint32_t PUMP_TIMER_BLINK_OFF_MS = 600;    // how long it stays hidden
// True when the timer should currently be shown (visible portion of the cycle).
static inline bool pumpTimerBlinkOn() {
  return (millis() % PUMP_TIMER_BLINK_MS) >= PUMP_TIMER_BLINK_OFF_MS;
}

// Water fill fraction (0..1) for each state — how high the water rises.
static float fillTargetLevel(PumpFillState s) {
  switch (s) {
    case PumpFillState::None:   return 0.00f;
    case PumpFillState::Weak:   return 0.32f;
    case PumpFillState::Good:   return 0.60f;
    case PumpFillState::Strong: return 1.00f;
  }
  return 0.0f;
}

// The state currently driving the water: demo cycle, or the last value set.
static PumpFillState currentFillState() {
  if (PUMP_FILL_DEMO_CYCLE) {
    return (PumpFillState)((millis() / PUMP_FILL_DEMO_MS) % 4);
  }
  return pumpFillState;
}

// Linear interpolate two RGB565 colours (t in 0..1).
static inline uint16_t lerp565(uint16_t a, uint16_t b, float t) {
  int ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
  int br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
  int r = ar + (int)((br - ar) * t + 0.5f);
  int g = ag + (int)((bg - ag) * t + 0.5f);
  int bl = ab + (int)((bb - ab) * t + 0.5f);
  return (uint16_t)((r << 11) | (g << 5) | bl);
}

// PNGdec callback functions for LittleFS
static void *pngOpen(const char *filename, int32_t *size) {
  pngFile = LittleFS.open(filename);
  if (!pngFile) {
    LOGE("TFT", "Failed to open PNG: %s", filename);
    return nullptr;
  }
  *size = pngFile.size();
  return &pngFile;
}

static void pngClose(void *handle) {
  if (pngFile) {
    pngFile.close();
  }
}

static int32_t pngRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!pngFile) return 0;
  return pngFile.read(buffer, length);
}

static int32_t pngSeek(PNGFILE *handle, int32_t position) {
  if (!pngFile) return 0;
  return pngFile.seek(position);
}

// Convert an RGB565 color into the 0x00BBGGRR format PNGdec's background-blend expects
static inline uint32_t rgb565ToPngBkgd(uint16_t c565) {
  uint8_t r5 = (c565 >> 11) & 0x1F;
  uint8_t g6 = (c565 >> 5)  & 0x3F;
  uint8_t b5 =  c565        & 0x1F;
  uint8_t r8 = (r5 * 255) / 31;
  uint8_t g8 = (g6 * 255) / 63;
  uint8_t b8 = (b5 * 255) / 31;
  return ((uint32_t)b8 << 16) | ((uint32_t)g8 << 8) | r8;
}

// PNG drawing callback - decodes directly into sprite
static int pngDraw(PNGDRAW *pDraw) {
  if (!decodeSprite) return 0;

  uint16_t lineBuffer[pDraw->iWidth];
  // Let PNGdec alpha-blend transparent/semi-transparent pixels against the card
  // background itself - this gives smooth anti-aliased edges instead of a hard cutoff.
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_LITTLE_ENDIAN, rgb565ToPngBkgd(COLOR_CARD));

  // Every pixel is now already correctly blended - just draw the whole line
  for (int x = 0; x < pDraw->iWidth; x++) {
    decodeSprite->drawPixel(x + pngOffX, pDraw->y + pngOffY, lineBuffer[x]);
  }
  return 1;
}

// Initialize LittleFS and PNGdec system
static bool initIconSystem(TFT_eSPI &tft) {
  // Try to mount LittleFS
  if (!littlefsMounted) {
    littlefsMounted = LittleFS.begin();
    if (!littlefsMounted) {
      LOGE("TFT", "LittleFS mount failed, using fallback icons");
      return false;
    }
    LOGI("TFT", "LittleFS mounted successfully");
  }
  
  // Create sprite for icon decoding
  if (!iconSprite) {
    iconSprite = new TFT_eSprite(&tft);
    if (!iconSprite) {
      LOGE("TFT", "Failed to create icon sprite");
      return false;
    }
  }
  
  // PNGdec callbacks are set when calling open()
  
  return true;
}

// Draw icon from LittleFS PNG with scaling support
static bool drawIconFromPNG(TFT_eSPI &tft, IconID iconId, int overrideX = -1, int overrideY = -1, uint16_t tintColor = 0xFFFF) {
  if (iconId >= ICON_COUNT) {
    LOGE("TFT", "Invalid icon ID: %d", iconId);
    return false;
  }
  
  const IconConfig &config = iconConfigs[iconId];
  int x = (overrideX >= 0) ? overrideX : config.x;
  int y = (overrideY >= 0) ? overrideY : config.y;
  
  // Initialize icon system if needed
  if (!initIconSystem(tft)) {
    // Fallback to simple rectangle if LittleFS failed
    tft.fillRect(x, y, config.size, config.size, COLOR_CARD);
    tft.drawRoundRect(x, y, config.size, config.size, 4, COLOR_CARD_2);
    return false;
  }
  
  // Check if PNG file exists
  if (!LittleFS.exists(config.file)) {
    LOGE("TFT", "PNG file not found: %s", config.file);
    return false;
  }
  
  // Create sprite at native resolution
  if (!iconSprite->createSprite(config.nativeW, config.nativeH)) {
    LOGE("TFT", "Failed to create sprite for icon: %s", config.file);
    return false;
  }

  iconSprite->fillSprite(COLOR_CARD);  // match card background so transparent areas blend in

  // Route pngDraw output into iconSprite at native origin
  decodeSprite = iconSprite;
  pngOffX = 0;
  pngOffY = 0;

  // Decode PNG into sprite
  int rc = png.open(config.file, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
  if (rc == PNG_SUCCESS) {
    png.decode(NULL, 0);
    png.close();
    
    // Apply color tint if requested (for power switch)
    if (tintColor != 0xFFFF) {
      for (int py = 0; py < config.nativeH; py++) {
        for (int px = 0; px < config.nativeW; px++) {
          uint16_t pixel = iconSprite->readPixel(px, py);
          if (pixel != 0) {  // Non-transparent pixel
            // Simple tint: blend with target color
            uint8_t r = ((pixel >> 11) & 0x1F);
            uint8_t g = ((pixel >> 5) & 0x3F);
            uint8_t b = (pixel & 0x1F);
            uint8_t tr = ((tintColor >> 11) & 0x1F);
            uint8_t tg = ((tintColor >> 5) & 0x3F);
            uint8_t tb = (tintColor & 0x1F);
            // 50% blend
            r = (r + tr) / 2;
            g = (g + tg) / 2;
            b = (b + tb) / 2;
            iconSprite->drawPixel(px, py, (r << 11) | (g << 5) | b);
          }
        }
      }
    }
    
    // Calculate zoom factor for scaling
    float zoomX = (float)config.size / config.nativeW;
    float zoomY = (float)config.size / config.nativeH;
    
    // Draw sprite to TFT (no scaling - use native size)
    iconSprite->pushSprite(x, y);
    
    iconSprite->deleteSprite();
    return true;
  } else {
    LOGE("TFT", "Failed to decode PNG: %s (rc=%d)", config.file, rc);
    iconSprite->deleteSprite();
    return false;
  }
}

// Decode the blade (pump.png) into a hub-centred sprite with real transparency
// in its gaps, allocate the composite scene sprite, and precompute the vertical
// water gradient. Runs once; returns true when everything is ready.
static bool ensurePumpAssets(TFT_eSPI &tft) {
  if (pumpAssetsReady) return true;
  if (!initIconSystem(tft)) return false;   // mounts LittleFS, creates iconSprite

  // ── Blade sprite ──
  if (bladeSprite == nullptr) {
    bladeSprite = new TFT_eSprite(&tft);
    if (bladeSprite == nullptr) return false;
  }
  // Prefer fast internal RAM; pushRotated reads the source pixel-by-pixel and
  // PSRAM reads are slow enough to make the spin stutter.
  bladeSprite->setAttribute(PSRAM_ENABLE, 0);
  if (!bladeSprite->createSprite(BLADE_CANVAS, BLADE_CANVAS)) {
    bladeSprite->setAttribute(PSRAM_ENABLE, 1);
    if (!bladeSprite->createSprite(BLADE_CANVAS, BLADE_CANVAS)) {
      LOGE("TFT", "Failed to allocate blade sprite");
      return false;
    }
  }
  bladeSprite->fillSprite(TFT_TRANSPARENT);

  // Decode pump.png so its hub lands at the canvas centre. pngDraw blends
  // transparent source pixels against COLOR_CARD, so after decoding we turn
  // those card-coloured pixels back into real transparency — that way water
  // shows through the gaps in the gear when the blade is composited on top.
  const char *bladeFile = iconConfigs[ICON_PUMP].file;
  if (LittleFS.exists(bladeFile)) {
    decodeSprite = bladeSprite;
    pngOffX = BLADE_PIV - BLADE_HUB_X;
    pngOffY = BLADE_PIV - BLADE_HUB_Y;
    int rc = png.open(bladeFile, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    if (rc == PNG_SUCCESS) { png.decode(NULL, 0); png.close(); }
    else LOGE("TFT", "Failed to decode blade PNG (rc=%d)", rc);
    decodeSprite = iconSprite;
    pngOffX = 0;
    pngOffY = 0;
  } else {
    LOGE("TFT", "Blade PNG not found: %s", bladeFile);
  }

  // Card-blended (fully transparent) pixels -> real transparent; trim to a disc
  // so the square canvas corners never paint over the water ring.
  const long bmr2 = (long)BLADE_MASK_R * BLADE_MASK_R;
  for (int y = 0; y < BLADE_CANVAS; y++) {
    int dy = y - BLADE_PIV;
    for (int x = 0; x < BLADE_CANVAS; x++) {
      int dx = x - BLADE_PIV;
      if ((long)dx * dx + (long)dy * dy > bmr2) { bladeSprite->drawPixel(x, y, TFT_TRANSPARENT); continue; }
      if (bladeSprite->readPixel(x, y) == COLOR_CARD) bladeSprite->drawPixel(x, y, TFT_TRANSPARENT);
    }
  }
  bladeSprite->setPivot(BLADE_PIV, BLADE_PIV);

  // ── Composite scene sprite ──
  if (sceneSprite == nullptr) {
    sceneSprite = new TFT_eSprite(&tft);
    if (sceneSprite == nullptr) return false;
  }
  sceneSprite->setAttribute(PSRAM_ENABLE, 0);
  if (!sceneSprite->createSprite(PUMP_SCENE, PUMP_SCENE)) {
    sceneSprite->setAttribute(PSRAM_ENABLE, 1);
    if (!sceneSprite->createSprite(PUMP_SCENE, PUMP_SCENE)) {
      LOGE("TFT", "Failed to allocate pump scene sprite");
      return false;
    }
  }
  sceneSprite->setPivot(PUMP_SCENE_PIV, PUMP_SCENE_PIV);

  // Precompute the water colour for each row (depth depends only on Y): light
  // cyan at the top of the disc grading to deeper cyan at the bottom.
  for (int y = 0; y < PUMP_SCENE; y++) {
    float depth = (float)y / (float)(PUMP_SCENE - 1);
    waterColY[y] = lerp565(WATER_TOP, WATER_DEEP, depth);
  }

  pumpAssetsReady = true;
  return true;
}

// Rebuild the pump scene (empty circle + wavy water + spinning blade) in the
// off-screen sprite and push the finished disc to the panel in one pass. Because
// the whole frame is assembled in RAM before a single contiguous push, the glass
// never shows a half-drawn frame — no tearing — and the transparent corners mean
// the surrounding body ring and power button are never touched.
static void drawPumpScene(TFT_eSPI &tft) {
  if (!ensurePumpAssets(tft)) return;

  const int C = PUMP_SCENE_PIV;      // scene centre
  const int R = PUMP_CIRCLE_R;       // disc radius
  // Water surface baseline in scene-Y: level 0 -> bottom (2R), level 1 -> top (0).
  const float surfaceBase = (1.0f - waterLevel) * (2.0f * R);
  const float k   = 6.2831853f / (2.0f * R) * 1.7f;   // ~1.7 wavelengths across
  const float amp = (waterLevel > 0.02f && waterLevel < 0.99f) ? 3.0f : 1.2f;

  // 1. Transparent everywhere (corners stay transparent so only the disc lands).
  sceneSprite->fillSprite(TFT_TRANSPARENT);

  // 2. Fill the disc with the empty-card colour first.
  for (int y = 0; y < PUMP_SCENE; y++) {
    int dy = y - C;
    long rem = (long)R * R - (long)dy * dy;
    if (rem < 0) continue;
    int hw = (int)sqrtf((float)rem);
    sceneSprite->drawFastHLine(C - hw, y, 2 * hw + 1, COLOR_CARD);
  }

  // 3. Water: rows fully below the wave trough are solid HLines; only the few
  //    rows within the wave band need a per-pixel surface test.
  if (waterLevel > 0.01f) {
    for (int y = 0; y < PUMP_SCENE; y++) {
      if (y < surfaceBase - amp) continue;         // above the highest wave crest — dry
      int dy = y - C;
      long rem = (long)R * R - (long)dy * dy;
      if (rem < 0) continue;
      int hw = (int)sqrtf((float)rem);
      int xL = C - hw, xR = C + hw;
      uint16_t col = waterColY[y];
      if (y >= surfaceBase + amp) {
        sceneSprite->drawFastHLine(xL, y, xR - xL + 1, col);   // fully submerged row
      } else {
        for (int x = xL; x <= xR; x++) {
          float surf = surfaceBase + amp * sinf(k * x + wavePhase);
          if ((float)y >= surf) sceneSprite->drawPixel(x, y, col);
        }
      }
    }
  }

  // 4. Spinning blade on top (transparent gaps keep the water behind it).
  bladeSprite->pushRotated(sceneSprite, (int16_t)bladeAngle, TFT_TRANSPARENT);

  // 5. Push the disc to the panel; transparent corners preserve the ring.
  sceneSprite->pushSprite(PUMP_CX - C, PUMP_CY - C, TFT_TRANSPARENT);
}

// Advance the pump animation one frame: blade spins up/coasts down with the pump,
// the water level eases toward the current fill state, and the surface wave keeps
// shimmering while there is any water. Skips all work when fully at rest and dry.
static void stepPumpScene(TFT_eSPI &tft, bool pumpOn, PumpFillState fs) {
  float targetLevel = fillTargetLevel(fs);
  bool active = pumpOn || bladeSpeed > 0.0f ||
                waterLevel > 0.004f || fabsf(waterLevel - targetLevel) > 0.004f;
  if (!active) {
    pumpLastStepMs = 0;   // reset dt baseline; nothing moving, leave the bus alone
    return;
  }

  uint32_t now = millis();
  float dt = (pumpLastStepMs == 0) ? 0.0f : (now - pumpLastStepMs) / 1000.0f;
  if (dt > 0.05f) dt = 0.05f;   // clamp so an occasional slow frame can't jump the animation
  pumpLastStepMs = now;

  // Blade: ease toward target speed — brisk spin-up, gentle spin-down.
  float target = pumpOn ? PUMP_MAX_SPEED : 0.0f;
  if (bladeSpeed < target) {
    bladeSpeed += PUMP_ACCEL * dt;
    if (bladeSpeed > target) bladeSpeed = target;
  } else if (bladeSpeed > target) {
    bladeSpeed -= PUMP_DECEL * dt;
    if (bladeSpeed < target) bladeSpeed = target;
  }
  if (bladeSpeed < 0.0f) bladeSpeed = 0.0f;
  bladeAngle += bladeSpeed * dt;
  while (bladeAngle >= 360.0f) bladeAngle -= 360.0f;

  // Water level: ease toward the target (reaches full in ~1.7s).
  const float lvStep = 0.6f * dt;
  if (waterLevel < targetLevel) {
    waterLevel += lvStep; if (waterLevel > targetLevel) waterLevel = targetLevel;
  } else if (waterLevel > targetLevel) {
    waterLevel -= lvStep; if (waterLevel < targetLevel) waterLevel = targetLevel;
  }

  // Surface wave phase.
  wavePhase += 3.0f * dt;
  if (wavePhase > 6.2831853f) wavePhase -= 6.2831853f;

  drawPumpScene(tft);
}

// ── Water-level gauge: a ring that fills with a blue arc, % in the middle ─────
// The blue arc is centred on the bottom of the ring and grows symmetrically with
// the level (like a rising tank). Cheap: just two arcs + the number, redrawn only
// when the percentage changes — no per-frame work, so it never bogs down the pump.
static constexpr int GAUGE_CX = 122;   // gauge centre on screen (left card)
static constexpr int GAUGE_CY = 155;
static constexpr int GAUGE_R_OUTER    = 80;   // ring outer radius
static constexpr int GAUGE_RING_THICK = 16;   // ring thickness
static constexpr int GAUGE_SPR     = 2 * GAUGE_R_OUTER + 1;   // off-screen buffer side (161)
static constexpr int GAUGE_SPR_PIV = GAUGE_R_OUTER;           // its centre (80,80)

// Water temperature sits inside the ring, just under the percentage. The box is
// kept narrow enough that its corners stay clear of the ring's inner edge
// (inner radius 64: at dy=51 the ring leaves +/-38px of clearance).
static constexpr int TEMP_CY    = 196;
static constexpr int TEMP_BOX_W = 70;
static constexpr int TEMP_BOX_H = 20;

// ── DEMO: no sender device yet ───────────────────────────────────
// Sweep the level 0 -> 100 -> 0 so you can watch the gauge fill and drain. Set
// LEVEL_DEMO_SWEEP false and edit LEVEL_TEST_FIXED to hold one value, or call
// DisplayUI::setWaterLevel() from your LoRa handler once the sender exists.
static constexpr bool     LEVEL_DEMO_SWEEP = false;  // production: driven by real telemetry
static constexpr uint8_t  LEVEL_TEST_FIXED = 0;
static constexpr uint32_t LEVEL_DEMO_MS    = 30000;   // full 0->100->0 period (gentle)
static uint8_t gWaterLevel = LEVEL_TEST_FIXED;        // set via setWaterLevel()

// The level percentage currently driving the gauge (demo sweep or last set).
static uint8_t currentLevelPercent() {
  if (LEVEL_DEMO_SWEEP) {
    uint32_t t = millis() % LEVEL_DEMO_MS;
    uint32_t half = LEVEL_DEMO_MS / 2;
    uint32_t up = (t < half) ? t : (LEVEL_DEMO_MS - t);   // triangle 0..half..0
    return (uint8_t)(up * 100 / half);
  }
  return gWaterLevel;
}

// The gauge is built in an off-screen buffer and pushed in ONE pass. Drawing the
// grey ring + rounded blue arc + number fresh into the sprite each change means:
//  - single-piece arcs -> no anti-aliased internal seams,
//  - roundEnds=true     -> rounded tips,
//  - one atomic push    -> the moving rounded tip is never drawn on the glass, so
//                          it can't shimmer/flicker as it advances.
// 0 deg is 6 o'clock (bottom); the blue arc runs from (360-half) through the
// bottom to (half). Undrawn sprite pixels stay transparent, so only the ring, arc
// and number land — the card background (and the water-drop icon) are untouched.
static TFT_eSprite *gaugeSprite = nullptr;
// Force the gauge to DIRECT DRAW (no 52KB sprite). PSRAM can't be used on this
// board (OPI PSRAM collides with the LoRa pins GPIO 36/37), and internal RAM is
// needed for the WiFi/TLS/BLE stack — so the gauge trades its buttery sprite for
// ~52KB of free heap. Set back to false only if you free RAM elsewhere.
static bool gaugeSpriteFailed  = true;

static void drawLevelGaugeDirect(TFT_eSPI &tft, uint8_t pct, bool levelOk);   // RAM-starved fallback

// Number shown in the middle of the ring: the percentage, or "--" when the
// Sensor Node reports the ultrasonic as not connected (showing 0% or a stale
// reading would be a lie).
static void formatGaugeNumber(char *buf, size_t len, uint8_t pct, bool levelOk) {
  if (levelOk) snprintf(buf, len, "%d%%", pct);
  else         strlcpy(buf, "--", len);
}

static void drawLevelGauge(TFT_eSPI &tft, uint8_t pct, bool levelOk) {
  if (!gaugeSpriteFailed && gaugeSprite == nullptr) {
    gaugeSprite = new TFT_eSprite(&tft);
    if (gaugeSprite) {
      // Put this 52KB sprite in PSRAM: it only redraws on % change (so PSRAM
      // speed is a non-issue) and this keeps ~52KB of internal RAM free for the
      // TLS/WiFi stack. If PSRAM is unavailable the createSprite fails and we
      // fall back to direct draw — which also uses no internal RAM.
      gaugeSprite->setAttribute(PSRAM_ENABLE, 1);
      if (!gaugeSprite->createSprite(GAUGE_SPR, GAUGE_SPR)) {
        gaugeSpriteFailed = true;   // no PSRAM — fall back to direct draw
        LOGW("TFT", "No PSRAM for gauge sprite - using direct draw");
      }
    } else {
      gaugeSpriteFailed = true;
    }
  }
  if (gaugeSpriteFailed || gaugeSprite == nullptr) { drawLevelGaugeDirect(tft, pct, levelOk); return; }

  const int C = GAUGE_SPR_PIV, ro = GAUGE_R_OUTER, ri = GAUGE_R_OUTER - GAUGE_RING_THICK;
  // No level sensor -> leave the track empty rather than drawing a fake arc.
  int newHalf = levelOk ? ((int)pct * 360 / 100 / 2) : 0;   // half-sweep, 0..180

  gaugeSprite->fillSprite(TFT_TRANSPARENT);
  gaugeSprite->drawSmoothArc(C, C, ro, ri, 0, 360, COLOR_CARD_2, COLOR_CARD, true);   // grey track
  if (newHalf >= 180) {
    gaugeSprite->drawSmoothArc(C, C, ro, ri, 0, 360, COLOR_LIGHT_BLUE, COLOR_CARD, true);
  } else if (newHalf > 0) {
    gaugeSprite->drawSmoothArc(C, C, ro, ri, 360 - newHalf, newHalf, COLOR_LIGHT_BLUE, COLOR_CARD, true);
  }

  // Fixed opaque patch behind the number (sized for the widest value, "100%").
  // We push the sprite with transparency, so without this the region a wider
  // number occupied wouldn't get cleared when the number shrinks (e.g. 17%->7%),
  // leaving a leftover digit. It sits well inside the ring, so it never touches
  // the arc, and COLOR_CARD matches the card so it's invisible.
  gaugeSprite->fillRect(C - 50, C - 22, 100, 44, COLOR_CARD);
  char buf[8];
  formatGaugeNumber(buf, sizeof(buf), pct, levelOk);
  gaugeSprite->setTextDatum(MC_DATUM);
  gaugeSprite->setFreeFont(&FreeSansBold18pt7b);
  gaugeSprite->setTextColor(levelOk ? COLOR_TEXT : COLOR_ORANGE, COLOR_CARD);
  gaugeSprite->drawString(buf, C, C);
  gaugeSprite->setFreeFont(NULL);

  gaugeSprite->pushSprite(GAUGE_CX - C, GAUGE_CY - C, TFT_TRANSPARENT);
}

// Fallback used only if the gauge sprite can't be allocated: draw straight to the
// screen. Flat ends and a plain centred number (may flicker), but functional.
static void drawLevelGaugeDirect(TFT_eSPI &tft, uint8_t pct, bool levelOk) {
  const int ro = GAUGE_R_OUTER, ri = GAUGE_R_OUTER - GAUGE_RING_THICK;
  int newHalf = levelOk ? ((int)pct * 360 / 100 / 2) : 0;
  tft.drawSmoothArc(GAUGE_CX, GAUGE_CY, ro, ri, 0, 360, COLOR_CARD_2, COLOR_CARD, true);
  if (newHalf > 0)
    tft.drawSmoothArc(GAUGE_CX, GAUGE_CY, ro, ri, 360 - newHalf, newHalf, COLOR_LIGHT_BLUE, COLOR_CARD, true);
  tft.fillRect(GAUGE_CX - 55, GAUGE_CY - 22, 110, 44, COLOR_CARD);
  char buf[8];
  formatGaugeNumber(buf, sizeof(buf), pct, levelOk);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSansBold18pt7b);
  tft.setTextColor(levelOk ? COLOR_TEXT : COLOR_ORANGE, COLOR_CARD);
  tft.drawString(buf, GAUGE_CX, GAUGE_CY);
  tft.setFreeFont(NULL);
}

// ── Water temperature, inside the ring under the percentage ──
// The GFX fonts carry no degree glyph, so the little ring is drawn by hand
// between the number and the "C". Shows a muted "-- C" when the Sensor Node
// reports no temperature probe connected.
static void drawWaterTemp(TFT_eSPI &tft, float tempC, bool tempOk) {
  tft.fillRect(GAUGE_CX - TEMP_BOX_W / 2, TEMP_CY - TEMP_BOX_H / 2,
               TEMP_BOX_W, TEMP_BOX_H, COLOR_CARD);

  char num[8];
  if (tempOk) snprintf(num, sizeof(num), "%d", (int)lroundf(tempC));
  else        strlcpy(num, "--", sizeof(num));
  const uint16_t color = tempOk ? COLOR_CYAN : COLOR_MUTED;

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(color, COLOR_CARD);

  const int wNum = tft.textWidth(num);
  const int wC   = tft.textWidth("C");
  int x = GAUGE_CX - (wNum + 4 + 7 + 3 + wC) / 2;   // number, gap, ring, gap, C

  tft.drawString(num, x, TEMP_CY);
  x += wNum + 4;
  tft.drawCircle(x + 3, TEMP_CY - 5, 3, color);     // degree sign, 2px ring
  tft.drawCircle(x + 3, TEMP_CY - 5, 2, color);
  x += 10;
  tft.drawString("C", x, TEMP_CY);
  tft.setFreeFont(NULL);
}

// Sensor presence from the node's mask (bit0 level, bit1 temp, bit2 flow).
// 0xFF = a legacy sender that reports no mask — assume everything is fine
// rather than showing false alarms.
static bool levelSensorOk(const TelemetrySnapshot &s) {
  if (!s.valid || s.sensorMask == 0xFF) return true;
  return (s.sensorMask & 0x01) != 0;
}

static bool tempSensorOk(const TelemetrySnapshot &s) {
  if (!s.valid) return false;
  if (s.sensorMask != 0xFF && !(s.sensorMask & 0x02)) return false;
  return s.waterTempC > -100;
}

// Draw fallback icon (simple rectangle) when LittleFS fails
static void drawFallbackIcon(TFT_eSPI &tft, IconID iconId, int x, int y, int size, uint16_t color) {
  tft.fillRoundRect(x, y, size, size, 4, COLOR_CARD);
  tft.drawRoundRect(x, y, size, size, 4, color);
  
  // Draw simple indicator
  if (iconId == ICON_WATER_DROP) {
    tft.fillCircle(x + size/2, y + size/2, size/3, COLOR_CYAN);
  } else if (iconId == ICON_PUMP_BODY || iconId == ICON_PUMP) {
    tft.fillRect(x + size/4, y + size/4, size/2, size/2, COLOR_MUTED);
  } else if (iconId == ICON_POWER_SWITCH) {
    tft.fillCircle(x + size/2, y + size/2, size/3, color);
  }
}

#if ENABLE_QR_SETUP_SCREEN && defined(ESP_QRCODE_CONFIG_DEFAULT)
struct QrRenderTarget {
  TFT_eSPI *tft;
  uint8_t scale;
  int startY;
};

QrRenderTarget qrRenderTarget = {nullptr, 0, 0};

void drawEspQrCode(esp_qrcode_handle_t qrCode) {
  if (qrRenderTarget.tft == nullptr) {
    return;
  }

  int qrModules = esp_qrcode_get_size(qrCode);
  int qrSize = qrModules * qrRenderTarget.scale;
  int startX = (TFT_WIDTH_PX - qrSize) / 2;

  qrRenderTarget.tft->fillRect(startX - 12, qrRenderTarget.startY - 12, qrSize + 24, qrSize + 24, TFT_WHITE);
  for (int y = 0; y < qrModules; y++) {
    for (int x = 0; x < qrModules; x++) {
      uint16_t color = esp_qrcode_get_module(qrCode, x, y) ? TFT_BLACK : TFT_WHITE;
      qrRenderTarget.tft->fillRect(
        startX + x * qrRenderTarget.scale,
        qrRenderTarget.startY + y * qrRenderTarget.scale,
        qrRenderTarget.scale,
        qrRenderTarget.scale,
        color
      );
    }
  }
}
#endif

String formatEta(uint32_t seconds) {
  if (seconds == 0) {
    return "--";
  }
  uint32_t hours = seconds / 3600;
  uint32_t minutes = (seconds % 3600) / 60;
  if (hours > 0) {
    return String(hours) + "h " + String(minutes) + "m";
  }
  return String(minutes) + "m";
}

String formatDuration(uint32_t ms) {
  if (ms == 0) return "00:00:00";
  uint32_t seconds = ms / 1000;
  uint32_t h = seconds / 3600;
  uint32_t m = (seconds % 3600) / 60;
  uint32_t s = seconds % 60;
  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s);
  return String(buf);
}

// Working-time label at minute resolution:
//   under an hour, 1 digit  -> "2 min"  (with a space)
//   under an hour, 2 digits -> "59min"  (no space)
//   whole hours             -> "1 hour", "2 hour"
//   hours + minutes         -> "1h 30m", "2h 30m"
String formatWorkingTime(uint32_t ms) {
  uint32_t totalMin = ms / 60000;
  uint32_t hours = totalMin / 60;
  uint32_t mins  = totalMin % 60;
  if (hours == 0) return String(mins) + (mins < 10 ? " min" : "min");
  if (mins == 0)  return String(hours) + " hour";
  return String(hours) + "h " + String(mins) + "m";
}

void drawRing(TFT_eSPI &tft, int x, int y, int r, int thickness, float startAngle, float endAngle, uint16_t color) {
  for (int i = 0; i < thickness; i++) {
    tft.drawCircle(x, y, r - i, color);
  }
}



void drawBadge(TFT_eSPI &tft, int x, int y, const char *text, uint16_t color) {
  tft.fillRoundRect(x, y, 80, 24, 8, color);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_BLACK, color);
  tft.setTextSize(1);
  tft.drawString(text, x + 40, y + 12, 2);
  tft.setTextDatum(TL_DATUM);
}

void drawCard(TFT_eSPI &tft, int x, int y, int w, int h, const uint8_t *bitmap, int bitmapWidth, int bitmapHeight) {
  tft.fillRoundRect(x, y, w, h, 14, COLOR_CARD);
  tft.drawRoundRect(x, y, w, h, 14, COLOR_CARD_2);
  if (bitmap != nullptr) {
    tft.drawXBitmap(x + 14, y + 10, bitmap, bitmapWidth, bitmapHeight, COLOR_MUTED);
  }
}

void drawProgressBar(TFT_eSPI &tft, uint8_t percent) {
  int x = 34;
  int y = 112;
  int w = 412;
  int h = 32;
  int fillWidth = map(percent, 0, 100, 0, w - 8);

  tft.fillRoundRect(x, y, w, h, 16, 0x0008);
  uint16_t fillColor = percent < 25 ? COLOR_RED : (percent < 55 ? COLOR_ORANGE : COLOR_CYAN);
  if (fillWidth > 0) {
    tft.fillRoundRect(x + 4, y + 4, fillWidth, h - 8, 12, fillColor);
  }
  tft.drawRoundRect(x, y, w, h, 16, COLOR_CARD_2);

  tft.fillRect(34, 82, 150, 26, COLOR_CARD);
  tft.setTextColor(COLOR_TEXT, COLOR_CARD);
  tft.drawString(String(percent) + "%", 34, 78, 6);
}

void drawQrCode(TFT_eSPI &tft, const String &payload) {
#if ENABLE_QR_SETUP_SCREEN
  static constexpr uint8_t QR_VERSION = 8;
  static constexpr uint8_t QR_SCALE = 4;

#if defined(ESP_QRCODE_CONFIG_DEFAULT)
  qrRenderTarget = {&tft, QR_SCALE, 58};
  esp_qrcode_config_t qrConfig = ESP_QRCODE_CONFIG_DEFAULT();
  qrConfig.display_func = drawEspQrCode;
  qrConfig.max_qrcode_version = QR_VERSION;
  qrConfig.qrcode_ecc_level = ESP_QRCODE_ECC_LOW;

  esp_err_t result = esp_qrcode_generate(&qrConfig, payload.c_str());
  qrRenderTarget.tft = nullptr;
  if (result != ESP_OK) {
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(payload, 30, 130, 2);
  }
#elif defined(ECC_LOW)
  QRCode qrCode;
  uint8_t qrData[320];
  int8_t result = qrcode_initText(&qrCode, qrData, QR_VERSION, ECC_LOW, payload.c_str());
  if (result < 0) {
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawString(payload, 30, 130, 2);
    return;
  }

  int qrSize = qrCode.size * QR_SCALE;
  int startX = (TFT_WIDTH_PX - qrSize) / 2;
  int startY = 58;

  tft.fillRect(startX - 12, startY - 12, qrSize + 24, qrSize + 24, TFT_WHITE);
  for (uint8_t y = 0; y < qrCode.size; y++) {
    for (uint8_t x = 0; x < qrCode.size; x++) {
      uint16_t color = qrcode_getModule(&qrCode, x, y) ? TFT_BLACK : TFT_WHITE;
      tft.fillRect(startX + x * QR_SCALE, startY + y * QR_SCALE, QR_SCALE, QR_SCALE, color);
    }
  }
#else
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.drawString(payload, 30, 130, 2);
#endif
#else
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.drawString(payload, 30, 130, 2);
#endif
}

// ═══════════════════════════════════════════════════════════════
// DIRTY-TRACKING STATE for incremental updates
// ═══════════════════════════════════════════════════════════════
struct DisplayCache {
  // Values we displayed last time
  bool     wasAutoMode      = false;
  bool     wasWifiConnected = false;
  char     wasWifiSSID[33]  = {0};
  uint8_t  wasLevelPercent  = 255;   // 255 = uninitialized
  bool     wasPumpOn        = false;
  uint32_t wasLastReadingS  = 0xFFFFFFFF;
  uint32_t wasWorkingTimeS  = 0xFFFFFFFF;   // last working time shown, in MINUTES
  bool     wasTimerShown    = false; // was the working-time timer visible last frame
  uint8_t  wasFillState     = 255;   // last PumpFillState shown (255 = uninitialized)
  bool     wasFilling       = false;
  bool     wasValid         = false;
  bool     wasLevelOk       = true;  // ultrasonic reported connected last frame
  int16_t  wasTempDeci      = 0x7FFF;   // last water temp shown, in 0.1 C (0x7FFF = uninit)
  bool     wasTempOk        = false;

  // Force full redraw on first call or mode switch
  bool forceFull = true;

  void invalidate() { forceFull = true; }
};

static DisplayCache gCache;

// Helper: draw top bar text only (no full clear)
void drawTopBarIncremental(TFT_eSPI &tft, const AppState &state) {
  tft.setTextSize(1);
  
  // Left: Version
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COLOR_TEXT, COLOR_CARD);  // bg: COLOR_CARD (grey) not COLOR_BG
  tft.drawString("Version : " + String(FIRMWARE_VERSION), 10, 10, 2);
  
  // Middle: Mode
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COLOR_ORANGE, COLOR_CARD);  // bg: COLOR_CARD
  tft.drawString(state.isAutoMode ? "AUTO " : "MANUAL", TFT_WIDTH_PX / 2, 10, 2);
  
  // Right: WiFi
  tft.setTextDatum(TR_DATUM);
  if (state.wifiConnected) {
    tft.setTextColor(COLOR_TEXT, COLOR_CARD);   // bg: COLOR_CARD
    tft.drawString(String(state.wifiSSID) + "  ", TFT_WIDTH_PX - 35, 10, 2);
    tft.setTextColor(COLOR_GREEN, COLOR_CARD);  // bg: COLOR_CARD
    tft.drawString("Connected  ", TFT_WIDTH_PX - 35, 25, 2);
    tft.fillCircle(TFT_WIDTH_PX - 20, 20, 8, COLOR_GREEN);
  } else {
    tft.setTextColor(COLOR_RED, COLOR_CARD);    // bg: COLOR_CARD
    tft.drawString("Not Connected  ", TFT_WIDTH_PX - 35, 10, 2);
    tft.drawCircle(TFT_WIDTH_PX - 20, 20, 8, COLOR_RED);
    tft.drawLine(TFT_WIDTH_PX - 25, 15, TFT_WIDTH_PX - 15, 25, COLOR_RED);
  }
}

// Draw text inside a rectangular region by rendering into an off-screen sprite
// and pushing it in ONE pass. The clear+draw happens in RAM, so the panel can
// never catch a half-cleared region — no flicker (unlike the old
// fillRect-then-drawString, which flashed when the refresh landed between the
// two steps; that was the intermittent timer flicker).
static void drawTextRegion(TFT_eSPI &tft, int x, int y, int w, int h,
                           const String &text, uint16_t fg, uint16_t bg,
                           const GFXfont *font, uint8_t gfxFont, uint8_t datum) {
  bool centred = (datum == MC_DATUM || datum == TC_DATUM || datum == BC_DATUM);
  bool rightAl = (datum == MR_DATUM || datum == TR_DATUM || datum == BR_DATUM);
  int tx = centred ? (w / 2) : (rightAl ? (w - 2) : 2);   // x within the region
  TFT_eSprite spr(&tft);
  spr.setAttribute(PSRAM_ENABLE, 0);
  if (spr.createSprite(w, h)) {
    spr.fillSprite(bg);
    if (font) spr.setFreeFont(font); else spr.setTextFont(gfxFont);
    spr.setTextDatum(datum);
    spr.setTextColor(fg, bg);
    if (text.length()) spr.drawString(text, tx, h / 2);
    spr.pushSprite(x, y);
    spr.deleteSprite();
  } else {
    // Low on RAM: fall back to direct draw (may flicker, but still correct).
    tft.fillRect(x, y, w, h, bg);
    if (text.length()) {
      if (font) tft.setFreeFont(font); else tft.setTextFont(gfxFont);
      tft.setTextDatum(datum);
      tft.setTextColor(fg, bg);
      tft.drawString(text, x + tx, y + h / 2);
      tft.setFreeFont(NULL);
    }
  }
}

// Text + colour for each filling state shown in the pump card's bottom strip.
static const char *fillStateText(PumpFillState s) {
  switch (s) {
    case PumpFillState::None:   return "No Filling";
    case PumpFillState::Weak:   return "Weak Filling";
    case PumpFillState::Good:   return "Good Filling";
    case PumpFillState::Strong: return "Strong Filling";
  }
  return "";
}
static uint16_t fillStateColor(PumpFillState s) {
  switch (s) {
    case PumpFillState::None:   return COLOR_MUTED;
    case PumpFillState::Weak:   return COLOR_LIGHT_BLUE;
    case PumpFillState::Good:   return COLOR_CYAN;
    case PumpFillState::Strong: return COLOR_GREEN;
  }
  return COLOR_MUTED;
}

// Bottom-left strip: "Last Reading" elapsed time (flicker-free).
void drawBottomLeftTime(TFT_eSPI &tft, uint32_t lastReadingMs, bool valid) {
  uint32_t elapsed = valid ? (millis() - lastReadingMs) : 0;
  drawTextRegion(tft, 146, 279, 88, 22, formatDuration(elapsed),
                 COLOR_CYAN, COLOR_CARD, &FreeSansBold9pt7b, 2, ML_DATUM);
}

// Pump card bottom strip: the current filling state, centred and colour-coded.
void drawPumpFillStrip(TFT_eSPI &tft, PumpFillState fs) {
  drawTextRegion(tft, 249, 279, 217, 32, fillStateText(fs),
                 fillStateColor(fs), COLOR_CARD, &FreeSansBold9pt7b, 2, MC_DATUM);
}

// Working-time timer, top-right of the pump card (where the power icon used to
// be). Compact minute resolution ("5min", "1h 30m"), grey. Drawn only when
// `show` is true; otherwise an empty string clears the region (off, or blinked
// off). The region sits above the pump body (body starts y68) so clearing it
// never touches the artwork.
void drawWorkingTimeCaption(TFT_eSPI &tft, uint32_t pumpStateChangedMs, bool show) {
  String s = show ? formatWorkingTime(millis() - pumpStateChangedMs) : "";
  drawTextRegion(tft, 352, 53, 107, 19, s,
                 COLOR_MUTED, COLOR_CARD, &FreeSansBold9pt7b, 2, MR_DATUM);
}

} // anonymous namespace

namespace DisplayUI {

void setupPanel(TFT_eSPI &tft) {
  if (TFT_BACKLIGHT_PIN >= 0) {
    pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
    analogWrite(TFT_BACKLIGHT_PIN, TFT_BACKLIGHT_LEVEL);
  }

#if defined(TFT_RST) && (TFT_RST >= 0)
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH);
  delay(80);
  digitalWrite(TFT_RST, LOW);
  delay(180);
  digitalWrite(TFT_RST, HIGH);
  delay(350);
#endif

  LOGI("TFT", "Initializing display");
  tft.init();
  tft.setRotation(TFT_ROTATION);
  tft.setSwapBytes(true);  // required for pushImage() RGB565 icon data to show correct colors
  LOGI("TFT", "Initialized rotation=%u", TFT_ROTATION);
}

void runSelfTest(TFT_eSPI &tft) {
#if ENABLE_BOOT_TFT_SELF_TEST
  LOGI("TFT", "Running RGB self-test");
  tft.fillScreen(TFT_RED);
  delay(180);
  tft.fillScreen(TFT_GREEN);
  delay(180);
  tft.fillScreen(TFT_BLUE);
  delay(180);
#else
  (void)tft;
#endif
}

void drawMainShell(TFT_eSPI &tft) {
  LOGD("TFT", "Draw main shell");
  tft.fillScreen(COLOR_BG);
}

void drawMain(TFT_eSPI &tft, const TelemetrySnapshot &snapshot, const AppState &state) {
  // ── FULL REDRAW: backgrounds, borders, icons, everything ──
  gCache.invalidate();  // mark cache dirty since we're doing full draw

  // 1. Top Bar — now with grey card container like other sections
  tft.fillRect(0, 0, TFT_WIDTH_PX, 44, COLOR_BG);  // clear slightly taller area
  tft.fillRoundRect(8, 4, TFT_WIDTH_PX - 16, 34, 10, COLOR_CARD);
  tft.drawRoundRect(8, 4, TFT_WIDTH_PX - 16, 34, 10, COLOR_CARD_2);
  drawTopBarIncremental(tft, state);

  // 2. Main Content
  if (snapshot.valid && snapshot.filling) {
    // OLD STYLE (FILLING MODE)
    tft.fillRoundRect(10, 50, 460, 260, 14, COLOR_CARD);
    tft.drawRoundRect(10, 50, 460, 260, 14, COLOR_CARD_2);

    drawProgressBar(tft, snapshot.levelPercent);

    tft.setTextColor(COLOR_TEXT, COLOR_CARD);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Volume: " + String(snapshot.volumeLiters, 0) + " L", 34, 160, 4);
    tft.drawString("Flow: " + String(snapshot.flowLpm, 1) + " L/min", 34, 200, 4);
    tft.drawString("ETA: " + formatEta(snapshot.etaSeconds), 34, 240, 4);

    tft.setTextColor(COLOR_CYAN, COLOR_CARD);
    tft.drawString("FILLING", 338, 248, 4);

    drawBadge(tft, 374, 145, state.pumpOn ? "PUMP ON" : "PUMP OFF", state.pumpOn ? COLOR_GREEN : COLOR_RED);
  } else {
    // NEW STYLE (IDLE MODE)
    // Left Section: Water Level
    tft.fillRoundRect(10, 45, 225, 220, 14, COLOR_CARD);
    tft.drawRoundRect(10, 45, 225, 220, 14, COLOR_CARD_2);
    // Draw water drop icon from PNG (with fallback if LittleFS fails)
    if (!drawIconFromPNG(tft, ICON_WATER_DROP)) {
      drawFallbackIcon(tft, ICON_WATER_DROP, 195, 60, 30, COLOR_CYAN);
    }
    // Water-level gauge: grey ring + blue fill arc + % in the middle, with the
    // water temperature from the Sensor Node just below the percentage.
    drawLevelGauge(tft, currentLevelPercent(), levelSensorOk(snapshot));
    drawWaterTemp(tft, snapshot.waterTempC, tempSensorOk(snapshot));

    tft.fillRoundRect(10, 275, 225, 40, 10, COLOR_CARD);
    tft.drawRoundRect(10, 275, 225, 40, 10, COLOR_CARD_2);
    tft.setTextDatum(ML_DATUM);
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextColor(COLOR_TEXT, COLOR_CARD);
    tft.drawString("Last Reading:", 20, 295);
    tft.setTextColor(COLOR_CYAN, COLOR_CARD);
    uint32_t lastUpdate = snapshot.valid ? (millis() - snapshot.lastReadingMs) : 0;
    tft.drawString(formatDuration(lastUpdate), 150, 295);

    // Right Section: Pump State
    tft.fillRoundRect(245, 45, 225, 220, 14, COLOR_CARD);
    tft.drawRoundRect(245, 45, 225, 220, 14, COLOR_CARD_2);
    // Draw the pump body (blue frame + empty circle) as the static background.
    if (!drawIconFromPNG(tft, ICON_PUMP_BODY)) {
      drawFallbackIcon(tft, ICON_PUMP_BODY, PUMP_BODY_X, PUMP_BODY_Y, PUMP_BODY_W, COLOR_MUTED);
    }
    // Build the blade + scene sprites now, during the first full draw, while
    // internal RAM is least fragmented so they land in fast RAM (smoother spin).
    ensurePumpAssets(tft);
    // Paint the current water + blade into the freshly drawn empty circle so the
    // first frame isn't blank; the incremental loop animates it from here.
    pumpLastStepMs = 0;
    drawPumpScene(tft);
    // Working-time timer at the top of the card (replaces the old power icon).
    drawWorkingTimeCaption(tft, state.pumpStateChangedMs, state.pumpOn && pumpTimerBlinkOn());

    // Bottom strip: current filling state (No / Weak / Good / Strong).
    tft.fillRoundRect(245, 275, 225, 40, 10, COLOR_CARD);
    tft.drawRoundRect(245, 275, 225, 40, 10, COLOR_CARD_2);
    drawPumpFillStrip(tft, currentFillState());

    tft.setFreeFont(NULL);
  }

  // Update cache after full draw
  gCache.wasAutoMode      = state.isAutoMode;
  gCache.wasWifiConnected = state.wifiConnected;
  strncpy(gCache.wasWifiSSID, state.wifiSSID, sizeof(gCache.wasWifiSSID) - 1);
  gCache.wasWifiSSID[sizeof(gCache.wasWifiSSID) - 1] =  '\0';
  gCache.wasLevelPercent  = (snapshot.valid && snapshot.filling) ? snapshot.levelPercent : currentLevelPercent();
  gCache.wasPumpOn        = state.pumpOn;
  gCache.wasLastReadingS  = snapshot.valid ? ((millis() - snapshot.lastReadingMs) / 1000) : 0;
  gCache.wasWorkingTimeS  = (millis() - state.pumpStateChangedMs) / 60000;  // minutes
  gCache.wasTimerShown    = state.pumpOn && pumpTimerBlinkOn();
  gCache.wasFillState     = (uint8_t)currentFillState();
  gCache.wasFilling       = snapshot.valid && snapshot.filling;
  gCache.wasValid         = snapshot.valid;
  gCache.wasLevelOk       = levelSensorOk(snapshot);
  gCache.wasTempOk        = tempSensorOk(snapshot);
  gCache.wasTempDeci      = (int16_t)lroundf(snapshot.waterTempC * 10.0f);
  gCache.forceFull        = false;
}

void drawMainUpdateOnly(TFT_eSPI &tft, const TelemetrySnapshot &snapshot, const AppState &state) {
  // If cache says full redraw needed, delegate to drawMain
  if (gCache.forceFull) {
    drawMain(tft, snapshot, state);
    return;
  }

  bool isFilling = snapshot.valid && snapshot.filling;
  bool layoutChanged = (isFilling != gCache.wasFilling) || (snapshot.valid != gCache.wasValid);

  if (layoutChanged) {
    // Filling/idle mode changed — need full redraw
    drawMain(tft, snapshot, state);
    return;
  }

  // ── INCREMENTAL UPDATE: only what changed ──

  // 1. TOP BAR: only if mode or WiFi changed
  bool modeChanged   = (state.isAutoMode != gCache.wasAutoMode);
  bool wifiChanged   = (state.wifiConnected != gCache.wasWifiConnected);
  bool ssidChanged   = (strncmp(state.wifiSSID, gCache.wasWifiSSID, sizeof(gCache.wasWifiSSID)) != 0);

  if (modeChanged || wifiChanged || ssidChanged) {
    // Clear only the text regions, not the whole 40px bar
    tft.fillRect(0, 0, TFT_WIDTH_PX, 44, COLOR_BG);
    tft.fillRoundRect(8, 2, TFT_WIDTH_PX - 16, 38, 10, COLOR_CARD);
    tft.drawRoundRect(8, 2, TFT_WIDTH_PX - 16, 38, 10, COLOR_CARD_2);
    drawTopBarIncremental(tft, state);
    gCache.wasAutoMode      = state.isAutoMode;
    gCache.wasWifiConnected = state.wifiConnected;
    strncpy(gCache.wasWifiSSID, state.wifiSSID, sizeof(gCache.wasWifiSSID) - 1);
    gCache.wasWifiSSID[sizeof(gCache.wasWifiSSID) - 1] =  '\0';
  }

  if (isFilling) {
    // ── FILLING MODE incremental ──
    uint8_t pct = snapshot.levelPercent;
    if (pct != gCache.wasLevelPercent || state.pumpOn != gCache.wasPumpOn) {
      // Clear progress bar area and redraw
      tft.fillRect(34, 78, 150, 26, COLOR_CARD);
      tft.fillRect(34, 160, 300, 100, COLOR_CARD);
      drawProgressBar(tft, pct);

      tft.setTextColor(COLOR_TEXT, COLOR_CARD);
      tft.setTextDatum(TL_DATUM);
      tft.drawString("Volume: " + String(snapshot.volumeLiters, 0) + " L", 34, 160, 4);
      tft.drawString("Flow: " + String(snapshot.flowLpm, 1) + " L/min", 34, 200, 4);
      tft.drawString("ETA: " + formatEta(snapshot.etaSeconds), 34, 240, 4);

      tft.setTextColor(COLOR_CYAN, COLOR_CARD);
      tft.drawString("FILLING", 338, 248, 4);

      drawBadge(tft, 374, 145, state.pumpOn ? "PUMP ON" : "PUMP OFF", state.pumpOn ? COLOR_GREEN : COLOR_RED);

      gCache.wasLevelPercent = pct;
      gCache.wasPumpOn = state.pumpOn;
    }
  } else {
    // ── IDLE MODE incremental ──

    // Water-level gauge: redraw only when the percentage (or the ultrasonic's
    // presence, which switches the number to "--") changes.
    uint8_t levelPct = currentLevelPercent();
    bool levelOk = levelSensorOk(snapshot);
    if (levelPct != gCache.wasLevelPercent || levelOk != gCache.wasLevelOk) {
      drawLevelGauge(tft, levelPct, levelOk);
      gCache.wasLevelPercent = levelPct;
      gCache.wasLevelOk = levelOk;
    }

    // Water temperature under the percentage: only on a real change (0.1 C).
    bool tempOk = tempSensorOk(snapshot);
    int16_t tempDeci = (int16_t)lroundf(snapshot.waterTempC * 10.0f);
    if (tempOk != gCache.wasTempOk || (tempOk && tempDeci != gCache.wasTempDeci)) {
      drawWaterTemp(tft, snapshot.waterTempC, tempOk);
      gCache.wasTempOk = tempOk;
      gCache.wasTempDeci = tempDeci;
    }

    gCache.wasPumpOn = state.pumpOn;

    // Animate the pump every frame: blade spins with the pump, water level and
    // surface wave follow the current fill state.
    stepPumpScene(tft, state.pumpOn, currentFillState());

    // Bottom-left card always shows "Last Reading: Xs". Missing sensors are
    // reported where their value belongs instead — an orange "--" in the gauge
    // for the ultrasonic, a muted "-- C" for the temperature probe.

    // Bottom-left time: only if seconds changed
    uint32_t lastReadingS = snapshot.valid ? ((millis() - snapshot.lastReadingMs) / 1000) : 0;
    if (lastReadingS != gCache.wasLastReadingS) {
      drawBottomLeftTime(tft, snapshot.lastReadingMs, snapshot.valid);
      gCache.wasLastReadingS = lastReadingS;
    }

    // Working-time timer (top): blink for a live feel and tick each minute.
    // Redraw whenever it winks on/off or the displayed minute changes.
    bool showTimer = state.pumpOn && pumpTimerBlinkOn();
    uint32_t workingTimeMin = (millis() - state.pumpStateChangedMs) / 60000;
    if (showTimer != gCache.wasTimerShown ||
        (showTimer && workingTimeMin != gCache.wasWorkingTimeS)) {
      drawWorkingTimeCaption(tft, state.pumpStateChangedMs, showTimer);
      gCache.wasTimerShown = showTimer;
      gCache.wasWorkingTimeS = workingTimeMin;
    }

    // Bottom strip filling state: only when it changes.
    PumpFillState fs = currentFillState();
    if ((uint8_t)fs != gCache.wasFillState) {
      drawPumpFillStrip(tft, fs);
      gCache.wasFillState = (uint8_t)fs;
    }
  }
}

bool isPumpAnimating() {
  // True while the pump blade is spinning or the pump water is waving, so the
  // render loop ticks fast for smooth motion and drops back once it settles. The
  // level gauge is a cheap on-change redraw, so it doesn't need the fast cadence.
  return bladeSpeed > 0.0f || waterLevel > 0.004f;
}

void setWaterLevel(uint8_t percent) {
  // Drive the level gauge from real data (e.g. a LoRa packet). Ignored while the
  // demo sweep is enabled (LEVEL_DEMO_SWEEP); the gauge eases toward it.
  gWaterLevel = percent > 100 ? 100 : percent;
}

void setPumpFillState(PumpFillState state) {
  // Drive the water level from real data (e.g. a LoRa packet). Ignored while the
  // demo cycle is enabled (PUMP_FILL_DEMO_CYCLE); the animation eases to it.
  pumpFillState = state;
}

void drawWifiSetup(TFT_eSPI &tft, const String &payloadUrl) {
  LOGI("TFT", "Draw WiFi setup QR screen");
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Pair Hydro Hub", 240, 24, 4);
  drawQrCode(tft, payloadUrl);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.drawString("Scan this code in the app", 240, 268, 4);
  tft.setTextColor(COLOR_MUTED, COLOR_BG);
  tft.drawString("Bleu Land - Hydro Hub", 240, 296, 2);
  tft.setTextDatum(TL_DATUM);
}

void drawOtaScreen(TFT_eSPI &tft, const char *version) {
  LOGI("TFT", "Draw OTA update screen (target %s)", version);
  tft.fillScreen(COLOR_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COLOR_CYAN, COLOR_BG);
  tft.drawString("Updating firmware...", 240, 130, 4);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.drawString(String("Version ") + version, 240, 170, 4);
  tft.setTextColor(COLOR_MUTED, COLOR_BG);
  tft.drawString("Do not power off the device", 240, 210, 2);
  tft.setTextDatum(TL_DATUM);
}

} // namespace DisplayUI
#endif // ENABLE_TFT_DASHBOARD