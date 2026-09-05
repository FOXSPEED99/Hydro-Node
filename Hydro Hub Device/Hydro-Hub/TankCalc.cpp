#include "TankCalc.h"

#include "DebugLog.h"
#include "config.h"

namespace {

TankConfig gConfig;

// Ring buffer of recent (timestamp, level%) points used to estimate how fast
// the level is rising. Readings arrive every ~2 minutes, so a handful of
// entries covers a long enough window to smooth ultrasonic jitter.
struct LevelPoint {
  uint32_t ms = 0;
  float levelPct = 0;
  bool used = false;
};
LevelPoint gHistory[LEVEL_HISTORY_LEN];
uint8_t gHistoryHead = 0;

void pushHistory(uint32_t nowMs, float levelPct) {
  gHistory[gHistoryHead] = {nowMs, levelPct, true};
  gHistoryHead = (gHistoryHead + 1) % LEVEL_HISTORY_LEN;
}

// Level change rate in %/minute across the oldest usable history point.
// Returns 0 when there isn't enough fresh history for a meaningful estimate.
float levelRatePctPerMin(uint32_t nowMs, float currentPct) {
  const LevelPoint *oldest = nullptr;
  for (uint8_t i = 0; i < LEVEL_HISTORY_LEN; i++) {
    const LevelPoint &p = gHistory[(gHistoryHead + i) % LEVEL_HISTORY_LEN];  // oldest first
    if (!p.used) continue;
    if (nowMs - p.ms > LEVEL_HISTORY_MAX_AGE_MS) continue;  // stale
    oldest = &p;
    break;
  }
  if (oldest == nullptr) return 0;

  float dtMin = (nowMs - oldest->ms) / 60000.0f;
  if (dtMin < 0.5f) return 0;  // need at least ~30s of separation
  return (currentPct - oldest->levelPct) / dtMin;
}

}  // namespace

namespace TankCalc {

void setConfig(const TankConfig &config) {
  bool geometryChanged = gConfig.valid &&
                         (config.heightCm != gConfig.heightCm || config.blindCm != gConfig.blindCm);
  gConfig = config;
  if (geometryChanged) {
    resetHistory();  // old level%s were computed against different geometry
  }
  LOGI("CALC", "Tank config v%lu: height=%ucm blind=%ucm capacity=%luL tanks=%u",
       static_cast<unsigned long>(config.version), config.heightCm, config.blindCm,
       static_cast<unsigned long>(config.capacityLiters), config.tankCount);
}

const TankConfig &config() { return gConfig; }

bool hasConfig() { return gConfig.valid && gConfig.heightCm > 0 && gConfig.capacityLiters > 0; }

void resetHistory() {
  for (auto &p : gHistory) p.used = false;
  gHistoryHead = 0;
}

bool applyRawReading(TelemetrySnapshot &snapshot, float distanceCm, bool flowOn, uint32_t nowMs) {
  snapshot.rawMode = true;
  snapshot.distanceCm = distanceCm;
  snapshot.flowSwitch = flowOn;

  // Negative distance = the node had no valid echo (sensor missing/faulty).
  // Keep the last known level/volume; only the flow state stays live.
  if (distanceCm < 0) {
    snapshot.filling = flowOn;
    strlcpy(snapshot.lastError, "Level sensor fault", sizeof(snapshot.lastError));
    LOGW("CALC", "No valid distance from the node; keeping last level");
    return false;
  }

  if (!hasConfig()) {
    strlcpy(snapshot.lastError, "Tank not configured", sizeof(snapshot.lastError));
    LOGW("CALC", "Raw reading dropped: no tank config yet (distance=%.1fcm)", distanceCm);
    return false;
  }

  // Spec §5.1: sensor reads blindCm when 100% full and blindCm+heightCm when empty.
  float waterHeightCm = static_cast<float>(gConfig.heightCm) - (distanceCm - static_cast<float>(gConfig.blindCm));
  float levelPct = 100.0f * waterHeightCm / static_cast<float>(gConfig.heightCm);
  levelPct = constrain(levelPct, 0.0f, 100.0f);

  float ratePctPerMin = levelRatePctPerMin(nowMs, levelPct);
  pushHistory(nowMs, levelPct);

  // Spec §5.2: volume follows level % of the total capacity (all tanks summed).
  float volumeLiters = static_cast<float>(gConfig.capacityLiters) * levelPct / 100.0f;

  // Spec §5.3: flow state from the reed switch + observed level rise.
  // The reed switch is authoritative for "is water flowing"; the rate of rise
  // translated to liters/minute grades the strength.
  float flowLpm = 0;
  if (ratePctPerMin > 0) {
    flowLpm = static_cast<float>(gConfig.capacityLiters) * ratePctPerMin / 100.0f;
  }

  // Spec §5.4: time to full from the observed rate of rise.
  uint32_t etaSeconds = 0;
  if (flowOn && ratePctPerMin > 0.01f && levelPct < 100.0f) {
    float minutesToFull = (100.0f - levelPct) / ratePctPerMin;
    if (minutesToFull < 48.0f * 60.0f) {  // cap at 48h; beyond that the estimate is noise
      etaSeconds = static_cast<uint32_t>(minutesToFull * 60.0f);
    }
  }

  snapshot.levelPercent = static_cast<uint8_t>(levelPct + 0.5f);
  snapshot.volumeLiters = volumeLiters;
  snapshot.filling = flowOn;
  snapshot.flowLpm = flowLpm;
  snapshot.etaSeconds = etaSeconds;

  LOGD("CALC", "distance=%.1fcm -> level=%u%% volume=%.0fL rate=%.2f%%/min flow=%.1fL/min eta=%lus",
       distanceCm, snapshot.levelPercent, volumeLiters, ratePctPerMin, flowLpm,
       static_cast<unsigned long>(etaSeconds));
  return true;
}

}  // namespace TankCalc
