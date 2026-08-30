#include "uwb/trilateration.h"
#include <math.h>

namespace Trilateration {
namespace {

struct Sample { double x; double y; double d; };

double residualRms(const Sample* s, int n, double x, double y) {
  if (n < 1) return 0.0;
  double acc = 0.0;
  for (int i = 0; i < n; ++i) {
    double dx = x - s[i].x;
    double dy = y - s[i].y;
    double err = hypot(dx, dy) - s[i].d;
    acc += err * err;
  }
  return sqrt(acc / n);
}

// 2-circle geometric solution (matches _trilaterate_2 in the golden model).
void trilaterate2(const Sample& s1, const Sample& s2, double& outX, double& outY) {
  double dx = s2.x - s1.x;
  double dy = s2.y - s1.y;
  double d = hypot(dx, dy);
  if (d > s1.d + s2.d || d < fabs(s1.d - s2.d)) {
    double t = s1.d / (s1.d + s2.d);
    outX = s1.x + t * dx;
    outY = s1.y + t * dy;
    return;
  }
  double a = (s1.d * s1.d - s2.d * s2.d + d * d) / (2.0 * d);
  double h2 = s1.d * s1.d - a * a;
  if (h2 < 0.0) h2 = 0.0;
  double h = sqrt(h2);
  double px = s1.x + a * dx / d;
  double py = s1.y + a * dy / d;
  outX = px + h * (-dy) / d;
  outY = py + h * dx / d;
}

// Least-squares refinement (matches _gauss_newton in the golden model).
void gaussNewton(const Sample* s, int n, double& outX, double& outY) {
  double x = 0.0, y = 0.0, totalW = 0.0;
  for (int i = 0; i < n; ++i) {
    double w = 1.0 / (s[i].d + 1e-6);
    x += s[i].x * w;
    y += s[i].y * w;
    totalW += w;
  }
  x /= totalW;
  y /= totalW;

  for (int iter = 0; iter < 10; ++iter) {
    double sxx = 0.0, sxy = 0.0, syy = 0.0, sxe = 0.0, sye = 0.0;
    for (int i = 0; i < n; ++i) {
      double dx = x - s[i].x;
      double dy = y - s[i].y;
      double cur = hypot(dx, dy);
      if (cur < 1e-6) continue;
      double err = cur - s[i].d;
      double inv = 1.0 / cur;
      double jx = dx * inv;
      double jy = dy * inv;
      sxx += jx * jx;
      sxy += jx * jy;
      syy += jy * jy;
      sxe += jx * err;
      sye += jy * err;
    }
    double det = sxx * syy - sxy * sxy;
    if (fabs(det) < 1e-12) break;
    double deltaX = -(sxe * syy - sye * sxy) / det;
    double deltaY = -(sye * sxx - sxe * sxy) / det;
    x += deltaX;
    y += deltaY;
    if (fabs(deltaX) < 1e-6 && fabs(deltaY) < 1e-6) break;
  }
  outX = x;
  outY = y;
}

}  // namespace

Result solve(const double anchorX[3], const double anchorY[3],
             const double d[3], uint8_t validMask) {
  Sample s[3];
  int n = 0;
  for (int i = 0; i < 3; ++i) {
    if ((validMask & (1u << i)) && d[i] > 0.0) {
      s[n].x = anchorX[i];
      s[n].y = anchorY[i];
      s[n].d = d[i];
      ++n;
    }
  }

  Result r = {0.0, 0.0, 0.0, false};
  if (n < 2) return r;

  if (n == 2) {
    trilaterate2(s[0], s[1], r.x, r.y);
  } else {
    gaussNewton(s, n, r.x, r.y);
  }
  r.rms = residualRms(s, n, r.x, r.y);
  r.valid = true;
  return r;
}

}  // namespace Trilateration