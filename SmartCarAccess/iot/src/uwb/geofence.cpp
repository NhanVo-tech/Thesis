#include "uwb/geofence.h"
#include <cmath>

namespace Geofence {
namespace {

double distSq(double x, double y, double cx, double cy) {
  const double dx = x - cx;
  const double dy = y - cy;
  return dx * dx + dy * dy;
}

}  // namespace

Zone classify(double x, double y) {
  if (distSq(x, y, kSeatX, kSeatY) <= kSeatRadiusM * kSeatRadiusM)
    return Zone::DRIVER_SEAT;
  if (distSq(x, y, kDoorX, kDoorY) <= kDoorRadiusM * kDoorRadiusM)
    return Zone::DRIVER_DOOR;
  if (distSq(x, y, 0.0, 0.0) <= kWelcomeRadiusM * kWelcomeRadiusM)
    return Zone::WELCOME;
  return Zone::OUTSIDE;
}

const char* zoneName(Zone z) {
  switch (z) {
    case Zone::DRIVER_SEAT: return "DRIVER_SEAT";
    case Zone::DRIVER_DOOR: return "DRIVER_DOOR";
    case Zone::WELCOME:     return "WELCOME";
    default:                return "OUTSIDE";
  }
}

void Hysteresis::reset() {
  current_ = Zone::OUTSIDE;
  candidate_ = Zone::OUTSIDE;
  stable_ = 0;
}

Zone Hysteresis::update(double x, double y, int debounce) {
  if (debounce < 1) debounce = 1;
  const Zone z = classify(x, y);
  if (z == candidate_) {
    if (++stable_ >= debounce) current_ = z;
  } else {
    candidate_ = z;
    stable_ = 1;
  }
  return current_;
}

}  // namespace Geofence
