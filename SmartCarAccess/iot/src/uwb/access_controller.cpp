#include "uwb/access_controller.h"
#include <Arduino.h>
#include <cmath>

namespace AccessController {

// ===== State Trackers =====
static int consecutive_close_reads = 0;
static bool is_door_unlocked = false;
static double last_x_m = 0.0;
static double last_y_m = 0.0;
static double last_distance_m = 0.0;
static double last_radial_mps = 0.0;
static uint32_t relay_deactivate_time_ms = 0;
static bool relay_active = false;

static double distanceToUnlockPoint(double x, double y) {
  const double dx = x - kUnlockPointX;
  const double dy = y - kUnlockPointY;
  return std::sqrt(dx * dx + dy * dy);
}

void begin() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  consecutive_close_reads = 0;
  is_door_unlocked = false;
  last_x_m = 0.0;
  last_y_m = 0.0;
  last_distance_m = 0.0;
  relay_active = false;
  Serial.printf(
    "[DOOR] Initialized: unlock_point=(%.2f,%.2f) radius=%.1fm reset=%.1fm hits=%d relay_pin=%d\n",
    kUnlockPointX, kUnlockPointY, UNLOCK_RADIUS_M, RESET_RADIUS_M,
    REQUIRED_CONSECUTIVE_HITS, RELAY_PIN);
}

static void fireRelayPulse() {
  Serial.println("[DOOR] *** FIRING UNLOCK RELAY ***");
  digitalWrite(RELAY_PIN, HIGH);
  relay_active = true;
  relay_deactivate_time_ms = millis() + RELAY_PULSE_MS;
}

void handlePosition(double x, double y) {
  handlePosition(x, y, 0.0, 0.0);
}

void handlePosition(double x, double y, double vx, double vy) {
  last_x_m = x;
  last_y_m = y;
  last_distance_m = distanceToUnlockPoint(x, y);

  // Radial velocity along the door line: >0 moving away, <0 approaching.
  const double dx = x - kUnlockPointX;
  const double dy = y - kUnlockPointY;
  last_radial_mps =
      (last_distance_m > 1e-6) ? (vx * dx + vy * dy) / last_distance_m : 0.0;

  // 1. User walked away -> re-arm the lock state
  if (last_distance_m > RESET_RADIUS_M) {
    if (is_door_unlocked) {
      Serial.printf(
        "[DOOR] User left the zone (d=%.2fm > reset=%.2fm). Re-arming.\n",
        last_distance_m, RESET_RADIUS_M);
      is_door_unlocked = false;  // Ready to unlock again on next approach
    }
    consecutive_close_reads = 0;
    return;
  }

  // 2. Already unlocked and standing near the car -> do nothing
  if (is_door_unlocked) {
    Serial.printf("[DOOR] Already unlocked. Ignoring (d=%.2fm)\n", last_distance_m);
    return;
  }

  // 3. Approach gate: reject readings while the user is moving away.
  if (ENABLE_APPROACH_GATE && last_radial_mps > APPROACH_SPEED_MIN_MPS) {
    if (consecutive_close_reads > 0) {
      Serial.printf(
        "[DOOR] Moving away (vr=%.2fm/s). Resetting counter.\n",
        last_radial_mps);
      consecutive_close_reads = 0;
    }
    return;
  }

  // 4. Approaching. Count consecutive in-zone hits.
  if (last_distance_m <= UNLOCK_RADIUS_M) {
    consecutive_close_reads++;
    Serial.printf("[DOOR] In zone! Hit count: %d/%d (d=%.2fm vr=%.2fm/s)\n",
                  consecutive_close_reads, REQUIRED_CONSECUTIVE_HITS,
                  last_distance_m, last_radial_mps);

    if (consecutive_close_reads >= REQUIRED_CONSECUTIVE_HITS) {
      // --- FIRE THE DOOR RELAY ---
      fireRelayPulse();
      is_door_unlocked = true;
      consecutive_close_reads = 0;
    }
  } else {
    // Bounce between zones -> reset the counter
    if (consecutive_close_reads > 0) {
      Serial.printf(
        "[DOOR] Distance bounced out of zone (%.2fm). Resetting counter.\n",
        last_distance_m);
      consecutive_close_reads = 0;
    }
  }
}

void tick() {
  // Deactivate relay after pulse duration
  if (relay_active && millis() >= relay_deactivate_time_ms) {
    digitalWrite(RELAY_PIN, LOW);
    relay_active = false;
    Serial.printf("[DOOR] Relay pulse complete\n");
  }
}

bool isDoorUnlocked() {
  return is_door_unlocked;
}

int getConsecutiveReadCount() {
  return consecutive_close_reads;
}

double getLastDistance() {
  return last_distance_m;
}

double getLastX() {
  return last_x_m;
}

double getLastY() {
  return last_y_m;
}

double getLastRadialSpeed() {
  return last_radial_mps;
}

void manualUnlock() {
  Serial.println("[DOOR] Manual unlock triggered");
  fireRelayPulse();
  is_door_unlocked = true;
}

void resetDoorState() {
  Serial.println("[DOOR] Door state reset");
  consecutive_close_reads = 0;
  is_door_unlocked = false;
  last_x_m = 0.0;
  last_y_m = 0.0;
  last_distance_m = 0.0;
  last_radial_mps = 0.0;
  if (relay_active) {
    digitalWrite(RELAY_PIN, LOW);
    relay_active = false;
  }
}

}  // namespace AccessController
