#include "uwb/access_controller.h"
#include "uwb/geofence.h"
#include <Arduino.h>
#include <cmath>

namespace AccessController {
namespace {

State g_state = State::LOCKED;

// Unlock / leave debounce counters.
int consecutive_close_reads = 0;
int leave_hits = 0;

// Seat-settle timer (DOOR_UNLOCKED -> OCCUPIED).
bool seat_timer_running = false;
uint32_t seat_settle_start_ms = 0;

// Telemetry.
double last_x_m = 0.0;
double last_y_m = 0.0;
double last_distance_m = 0.0;
double last_radial_mps = 0.0;
double last_speed_mps = 0.0;

// Relay + ignition.
uint32_t relay_deactivate_time_ms = 0;
bool relay_active = false;
bool ignition_authorized = false;

// Geofence hysteresis (feeds [ZONE] logging + transitions).
Geofence::Hysteresis zone_hyst;
Geofence::Zone last_zone = Geofence::Zone::OUTSIDE;

double dist(double x, double y, double cx, double cy) {
  const double dx = x - cx;
  const double dy = y - cy;
  return std::sqrt(dx * dx + dy * dy);
}

const char* stateName(State s) {
  switch (s) {
    case State::DOOR_UNLOCKED: return "DOOR_UNLOCKED";
    case State::OCCUPIED:      return "OCCUPIED";
    default:                   return "LOCKED";
  }
}

void setState(State s) {
  if (s == g_state) return;
  g_state = s;
  Serial.printf("[STATE] %s\n", stateName(s));
}

void fireRelayPulse() {
  Serial.println("[DOOR] *** FIRING UNLOCK RELAY ***");
  digitalWrite(RELAY_PIN, HIGH);
  relay_active = true;
  relay_deactivate_time_ms = millis() + RELAY_PULSE_MS;
}

void lockDoor() {
  if (relay_active) {
    digitalWrite(RELAY_PIN, LOW);
    relay_active = false;
  }
}

void setIgnition(bool on) {
  if (on == ignition_authorized) return;
  ignition_authorized = on;
  digitalWrite(IGNITION_PIN, on ? HIGH : LOW);
  Serial.printf("[IGNITION] %s\n", on ? "ON" : "OFF");
}

}  // namespace

void begin() {
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(IGNITION_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(IGNITION_PIN, LOW);
  g_state = State::LOCKED;
  consecutive_close_reads = 0;
  leave_hits = 0;
  seat_timer_running = false;
  ignition_authorized = false;
  relay_active = false;
  zone_hyst.reset();
  last_zone = Geofence::Zone::OUTSIDE;
  Serial.printf("[DOOR] Init: door=(%.2f,%.2f) seat=(%.2f,%.2f) doorR=%.1f seatR=%.2f still=%.2f settle=%ums relay=%d ign=%d\n",
                Geofence::kDoorX, Geofence::kDoorY,
                Geofence::kSeatX, Geofence::kSeatY,
                Geofence::kDoorRadiusM, Geofence::kSeatRadiusM,
                STILL_SPEED_MPS, (unsigned)SEAT_SETTLE_MS,
                RELAY_PIN, IGNITION_PIN);
}

void handlePosition(double x, double y) {
  handlePosition(x, y, 0.0, 0.0);
}

void handlePosition(double x, double y, double vx, double vy) {
  last_x_m = x;
  last_y_m = y;
  last_distance_m = dist(x, y, Geofence::kDoorX, Geofence::kDoorY);
  last_speed_mps = std::hypot(vx, vy);

  const double dx = x - Geofence::kDoorX;
  const double dy = y - Geofence::kDoorY;
  last_radial_mps =
      (last_distance_m > 1e-6) ? (vx * dx + vy * dy) / last_distance_m : 0.0;

  // Stable zone (debounced) — drives transitions and the [ZONE] log.
  const Geofence::Zone zone = zone_hyst.update(x, y, 3);
  if (zone != last_zone) {
    last_zone = zone;
    Serial.printf("[ZONE] %s\n", Geofence::zoneName(zone));
  }

  const bool moving_away = ENABLE_APPROACH_GATE &&
                           last_radial_mps > APPROACH_SPEED_MIN_MPS;

  switch (g_state) {
    case State::LOCKED: {
      if (zone == Geofence::Zone::OUTSIDE) {
        consecutive_close_reads = 0;
        return;
      }
      if (moving_away) {
        consecutive_close_reads = 0;
        return;
      }
      const bool near_door = (zone == Geofence::Zone::DRIVER_DOOR ||
                              zone == Geofence::Zone::DRIVER_SEAT);
      if (near_door && last_speed_mps < STILL_SPEED_MPS) {
        if (++consecutive_close_reads >= REQUIRED_CONSECUTIVE_HITS) {
          fireRelayPulse();
          setState(State::DOOR_UNLOCKED);
          consecutive_close_reads = 0;
        }
      } else {
        consecutive_close_reads = 0;
      }
      break;
    }

    case State::DOOR_UNLOCKED: {
      // Sit still at the driver seat -> lock + authorize ignition.
      if (zone == Geofence::Zone::DRIVER_SEAT &&
          last_speed_mps < STILL_SPEED_MPS) {
        if (!seat_timer_running) {
          seat_timer_running = true;
          seat_settle_start_ms = millis();
        } else if (millis() - seat_settle_start_ms >= SEAT_SETTLE_MS) {
          lockDoor();
          setIgnition(true);
          setState(State::OCCUPIED);
          seat_timer_running = false;
        }
      } else {
        seat_timer_running = false;
      }
      // Walk away without sitting -> re-lock.
      if (zone == Geofence::Zone::OUTSIDE && moving_away) {
        lockDoor();
        setState(State::LOCKED);
      }
      break;
    }

    case State::OCCUPIED: {
      // User leaves the car -> revoke ignition + lock.
      if (zone == Geofence::Zone::OUTSIDE && moving_away) {
        if (++leave_hits >= LEAVE_CONSECUTIVE_HITS) {
          setIgnition(false);
          lockDoor();
          setState(State::LOCKED);
          leave_hits = 0;
        }
      } else {
        leave_hits = 0;
      }
      break;
    }
  }
}

void tick() {
  if (relay_active && millis() >= relay_deactivate_time_ms) {
    digitalWrite(RELAY_PIN, LOW);
    relay_active = false;
    Serial.printf("[DOOR] Relay pulse complete\n");
  }
}

State state() { return g_state; }
bool isIgnitionAuthorized() { return ignition_authorized; }
bool isDoorUnlocked() { return g_state == State::DOOR_UNLOCKED; }

int getConsecutiveReadCount() { return consecutive_close_reads; }
double getLastDistance() { return last_distance_m; }
double getLastX() { return last_x_m; }
double getLastY() { return last_y_m; }
double getLastRadialSpeed() { return last_radial_mps; }
double getLastSpeed() { return last_speed_mps; }

void manualUnlock() {
  Serial.println("[DOOR] Manual unlock triggered");
  fireRelayPulse();
  setState(State::DOOR_UNLOCKED);
  consecutive_close_reads = 0;
}

void resetDoorState() {
  Serial.println("[DOOR] Door state reset");
  lockDoor();
  setIgnition(false);
  setState(State::LOCKED);
  consecutive_close_reads = 0;
  leave_hits = 0;
  seat_timer_running = false;
  zone_hyst.reset();
  last_zone = Geofence::Zone::OUTSIDE;
}

}  // namespace AccessController
