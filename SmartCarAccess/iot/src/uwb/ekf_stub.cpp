#include "uwb/ekf_stub.h"
#include <math.h>

namespace Ekf {
namespace {

// ---- Tunables --------------------------------------------------------------
// Process (manoeuvre) noise: 1-sigma acceleration a walking user can produce.
constexpr double kAccelStd = 1.2;        // m/s^2
// Measurement-noise bounds (metres). RMS from trilateration is clamped here.
constexpr double kDefaultMeasStd = 0.15; // used when RMS is unavailable
constexpr double kMinMeasStd = 0.05;
constexpr double kMaxMeasStd = 1.00;
// Initial velocity uncertainty (m/s)^2 and nominal step for the 2-arg overload.
constexpr double kInitVelVar = 4.0;
constexpr double kNominalDtS = 0.10;     // ~10 Hz fallback
// Gaps larger than this (s) reinitialise the track instead of predicting.
constexpr double kMaxGapS = 2.0;

// ---- State -----------------------------------------------------------------
bool     g_init = false;
uint32_t g_lastMs = 0;
double   g_x[4] = {0, 0, 0, 0};   // [px, py, vx, vy]
double   g_P[4][4] = {{0}};       // state covariance

// ---- Small fixed-size matrix helpers (n = 4) -------------------------------
void mul44(const double a[4][4], const double b[4][4], double out[4][4]) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      double acc = 0.0;
      for (int k = 0; k < 4; ++k) acc += a[i][k] * b[k][j];
      out[i][j] = acc;
    }
  }
}

void transpose44(const double a[4][4], double out[4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) out[i][j] = a[j][i];
}

double clampd(double v, double lo, double hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

double measVarFromStd(double std) {
  if (!(std > 0.0)) std = kDefaultMeasStd;
  std = clampd(std, kMinMeasStd, kMaxMeasStd);
  return std * std;
}

void initFrom(double zx, double zy, double measVar) {
  g_x[0] = zx;
  g_x[1] = zy;
  g_x[2] = 0.0;
  g_x[3] = 0.0;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) g_P[i][j] = 0.0;
  g_P[0][0] = measVar;
  g_P[1][1] = measVar;
  g_P[2][2] = kInitVelVar;
  g_P[3][3] = kInitVelVar;
  g_init = true;
}

// Predict state and covariance forward by dt seconds (constant velocity).
void predict(double dt) {
  g_x[0] += g_x[2] * dt;
  g_x[1] += g_x[3] * dt;

  double F[4][4] = {{1, 0, dt, 0}, {0, 1, 0, dt}, {0, 0, 1, 0}, {0, 0, 0, 1}};
  double FP[4][4], Ft[4][4], FPFt[4][4];
  mul44(F, g_P, FP);
  transpose44(F, Ft);
  mul44(FP, Ft, FPFt);

  // Discrete white-noise acceleration process covariance Q.
  const double q = kAccelStd * kAccelStd;
  const double dt2 = dt * dt, dt3 = dt2 * dt, dt4 = dt2 * dt2;
  double Q[4][4] = {{0}};
  Q[0][0] = q * dt4 / 4.0;
  Q[0][2] = q * dt3 / 2.0;
  Q[1][1] = q * dt4 / 4.0;
  Q[1][3] = q * dt3 / 2.0;
  Q[2][0] = q * dt3 / 2.0;
  Q[2][2] = q * dt2;
  Q[3][1] = q * dt3 / 2.0;
  Q[3][3] = q * dt2;

  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) g_P[i][j] = FPFt[i][j] + Q[i][j];
}

// Correct with a 2D position measurement z = [zx, zy], noise variance measVar.
// H selects (px, py), so the maths is specialised to that structure.
void correct(double zx, double zy, double measVar) {
  // Innovation covariance S = H P H^T + R (2x2, top-left of P plus R).
  const double S00 = g_P[0][0] + measVar;
  const double S01 = g_P[0][1];
  const double S10 = g_P[1][0];
  const double S11 = g_P[1][1] + measVar;
  const double det = S00 * S11 - S01 * S10;
  if (fabs(det) < 1e-12) return;

  const double iS00 = S11 / det;
  const double iS01 = -S01 / det;
  const double iS10 = -S10 / det;
  const double iS11 = S00 / det;

  // Kalman gain K = P H^T S^-1 (4x2). P H^T is columns 0,1 of P.
  double K[4][2];
  for (int i = 0; i < 4; ++i) {
    const double a = g_P[i][0];
    const double b = g_P[i][1];
    K[i][0] = a * iS00 + b * iS10;
    K[i][1] = a * iS01 + b * iS11;
  }

  // State update x += K (z - H x).
  const double yx = zx - g_x[0];
  const double yy = zy - g_x[1];
  for (int i = 0; i < 4; ++i) g_x[i] += K[i][0] * yx + K[i][1] * yy;

  // Covariance update P = (I - K H) P; K H only touches columns 0,1.
  double M[4][4];
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) M[i][j] = (i == j) ? 1.0 : 0.0;
  for (int i = 0; i < 4; ++i) {
    M[i][0] -= K[i][0];
    M[i][1] -= K[i][1];
  }
  double newP[4][4];
  mul44(M, g_P, newP);

  // Enforce symmetry to curb round-off drift.
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) g_P[i][j] = 0.5 * (newP[i][j] + newP[j][i]);
}

}  // namespace

void reset() {
  g_init = false;
  g_lastMs = 0;
  for (int i = 0; i < 4; ++i) {
    g_x[i] = 0.0;
    for (int j = 0; j < 4; ++j) g_P[i][j] = 0.0;
  }
}

void update(double x, double y, uint32_t t_ms, double measNoiseStd) {
  const double measVar = measVarFromStd(measNoiseStd);

  if (!g_init) {
    initFrom(x, y, measVar);
    g_lastMs = t_ms;
    return;
  }

  double dt = (double)(t_ms - g_lastMs) / 1000.0;
  g_lastMs = t_ms;

  if (dt <= 0.0 || dt > kMaxGapS) {
    // Clock glitch or long gap: restart the track from this fix.
    initFrom(x, y, measVar);
    return;
  }

  predict(dt);
  correct(x, y, measVar);
}

void update(double x, double y) {
  const uint32_t synthMs =
      g_lastMs + (uint32_t)(kNominalDtS * 1000.0 + 0.5);
  update(x, y, synthMs, 0.0);
}

bool   initialized() { return g_init; }
double x()  { return g_x[0]; }
double y()  { return g_x[1]; }
double vx() { return g_x[2]; }
double vy() { return g_x[3]; }
double speed() { return hypot(g_x[2], g_x[3]); }

}  // namespace Ekf