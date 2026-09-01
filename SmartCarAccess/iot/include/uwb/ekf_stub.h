#pragma once
#include <stdint.h>

// 2D constant-velocity Kalman/EKF that fuses the noisy (x, y) fixes coming out
// of trilateration into a smoothed track. State is [x, y, vx, vy] (metres,
// metres/second); the measurement is the trilateration position, trusted less
// when its residual RMS is large.
namespace Ekf {

// Reset the filter to an uninitialised state (call when ranging (re)starts).
void reset();

// Fuse one 2D position fix.
//   x, y        : trilateration position (metres, car frame)
//   t_ms        : measurement timestamp (millis()); used to derive dt
//   measNoiseStd: 1-sigma measurement noise (metres). Pass the trilateration
//                 RMS so noisy fixes are trusted less; <= 0 uses the default.
void update(double x, double y, uint32_t t_ms, double measNoiseStd);

// Convenience overload (keeps the original call site working): synthesises a
// nominal timestep and uses the default measurement noise.
void update(double x, double y);

// Advance the track by prediction only (no measurement) up to t_ms. Lets the
// filter keep running at a fixed rate when a RANGE frame is dropped. Returns
// true while the estimate is still fresh (last fix newer than the max gap);
// false once the track is stale and should not be trusted.
bool predictTo(uint32_t t_ms);

// Latest fused estimate.
bool   initialized();
double x();
double y();
double vx();
double vy();
double speed();  // sqrt(vx^2 + vy^2)

}  // namespace Ekf