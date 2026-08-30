#pragma once
#include <stdint.h>

// Gauss-Newton trilateration ported from run_fira_multianchor.py (golden model).
// Self-contained: fixed-size arrays, no STL, no dynamic allocation.
namespace Trilateration {

struct Result {
  double x;
  double y;
  double rms;    // RMS of per-anchor residuals (metres)
  bool   valid;  // false if fewer than 2 usable anchors
};

// anchorX/anchorY: fixed anchor positions (metres). d[i]: measured distance (metres).
// validMask: bit i set => d[i] is usable (also requires d[i] > 0).
Result solve(const double anchorX[3], const double anchorY[3],
             const double d[3], uint8_t validMask);

}  // namespace Trilateration