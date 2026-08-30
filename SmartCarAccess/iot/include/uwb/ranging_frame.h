#pragma once
#include <stdint.h>

// One multi-anchor ranging round forwarded by the PC bridge.
// valid_mask bit i set => d[i] is fresh and in-bounds.
struct RangingFrame {
  uint32_t t_ms;
  double d[3];
  uint8_t valid_mask;
};