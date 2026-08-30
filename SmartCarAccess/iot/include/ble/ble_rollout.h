#pragma once

#include <stdint.h>

// Rollout flags for staged PKE deployment.
// These are compile-time defaults that can be overridden in platformio.ini.

#ifndef PKE_ROLLOUT_BACKGROUND_MODE
#define PKE_ROLLOUT_BACKGROUND_MODE 0
#endif

#ifndef PKE_ROLLOUT_FAST_TRANSACTION
#define PKE_ROLLOUT_FAST_TRANSACTION 1
#endif

#ifndef PKE_ROLLOUT_BONDING_ENFORCE
#define PKE_ROLLOUT_BONDING_ENFORCE 0
#endif

#ifndef PKE_ROLLOUT_RSSI_MONITOR_ONLY
#define PKE_ROLLOUT_RSSI_MONITOR_ONLY 1
#endif

#ifndef PKE_ROLLOUT_RSSI_THRESHOLD_DBM
#define PKE_ROLLOUT_RSSI_THRESHOLD_DBM -70
#endif

#ifndef PKE_ROLLOUT_ADV_FAST_MIN_MS
#define PKE_ROLLOUT_ADV_FAST_MIN_MS 30
#endif

#ifndef PKE_ROLLOUT_ADV_FAST_MAX_MS
#define PKE_ROLLOUT_ADV_FAST_MAX_MS 60
#endif

#ifndef PKE_ROLLOUT_ADV_SLOW_MIN_MS
#define PKE_ROLLOUT_ADV_SLOW_MIN_MS 250
#endif

#ifndef PKE_ROLLOUT_ADV_SLOW_MAX_MS
#define PKE_ROLLOUT_ADV_SLOW_MAX_MS 500
#endif

#ifndef PKE_ROLLOUT_ADV_FAST_WINDOW_MS
#define PKE_ROLLOUT_ADV_FAST_WINDOW_MS 15000
#endif

namespace BLERollout {
struct Flags {
  bool backgroundMode;
  bool fastTransaction;
  bool bondingEnforce;
  bool rssiMonitorOnly;
  int rssiThresholdDbm;
  uint16_t advFastMinMs;
  uint16_t advFastMaxMs;
  uint16_t advSlowMinMs;
  uint16_t advSlowMaxMs;
  uint32_t advFastWindowMs;
};

inline Flags flags() {
  Flags f;
  f.backgroundMode = (PKE_ROLLOUT_BACKGROUND_MODE != 0);
  f.fastTransaction = (PKE_ROLLOUT_FAST_TRANSACTION != 0);
  f.bondingEnforce = (PKE_ROLLOUT_BONDING_ENFORCE != 0);
  f.rssiMonitorOnly = (PKE_ROLLOUT_RSSI_MONITOR_ONLY != 0);
  f.rssiThresholdDbm = PKE_ROLLOUT_RSSI_THRESHOLD_DBM;
  f.advFastMinMs = PKE_ROLLOUT_ADV_FAST_MIN_MS;
  f.advFastMaxMs = PKE_ROLLOUT_ADV_FAST_MAX_MS;
  f.advSlowMinMs = PKE_ROLLOUT_ADV_SLOW_MIN_MS;
  f.advSlowMaxMs = PKE_ROLLOUT_ADV_SLOW_MAX_MS;
  f.advFastWindowMs = PKE_ROLLOUT_ADV_FAST_WINDOW_MS;
  return f;
}
}  // namespace BLERollout
