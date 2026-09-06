#pragma once
#include <stdint.h>

// Virtual zone classification around the car, based purely on the fused (x, y)
// position (car frame, metres). Backs the deterministic access state machine
// and feeds the AI intent classifier later; no ML involved here.
namespace Geofence {

enum class Zone : uint8_t {
  OUTSIDE,       // beyond the welcome radius
  WELCOME,       // near the car but not at a specific feature
  DRIVER_DOOR,   // near the driver-door unlock point
  DRIVER_SEAT,   // "inside" at the driver seat
};

// ---- Geometry (car frame, metres) -----------------------------------------
// Single source of truth for the access zones. The door point coincides with
// anchor 2 (left B-pillar) in uwb_geometry.h; the seat sits slightly "inside".
constexpr double kDoorX = -0.95;
constexpr double kDoorY = 0.0;
constexpr double kDoorRadiusM = 1.0;      // unlock radius (matches the door zone)

constexpr double kSeatX = -0.35;
constexpr double kSeatY = 0.0;
constexpr double kSeatRadiusM = 0.45;     // "sitting" radius around the seat

constexpr double kWelcomeRadiusM = 4.0;   // from car centre (0,0)

// Pure geometric classification (no hysteresis). DRIVER_SEAT is checked first
// because it is a subset of DRIVER_DOOR.
Zone classify(double x, double y);

const char* zoneName(Zone z);

// Stateful hysteresis: only transitions after the candidate zone has been
// stable for `debounce` consecutive updates, to avoid boundary chatter.
class Hysteresis {
 public:
  void reset();
  Zone update(double x, double y, int debounce = 3);
  Zone current() const { return current_; }

 private:
  Zone current_ = Zone::OUTSIDE;
  Zone candidate_ = Zone::OUTSIDE;
  int stable_ = 0;
};

}  // namespace Geofence
