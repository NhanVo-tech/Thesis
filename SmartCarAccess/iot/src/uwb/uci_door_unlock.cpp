#include "uwb/uci_door_unlock.h"
#include <Arduino.h>
#include <cstring>

namespace UwbDoorUnlock {

// ===== State Trackers =====
static int consecutive_close_reads = 0;
static bool is_door_unlocked = false;
static double last_distance_m = 0.0;
static double last_filtered_distance_m = 0.0;
static double last_residual_m = 0.0;
static uint32_t relay_deactivate_time_ms = 0;
static bool relay_active = false;

void begin() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  consecutive_close_reads = 0;
  is_door_unlocked = false;
  last_distance_m = 0.0;
  last_filtered_distance_m = 0.0;
  last_residual_m = 0.0;
  relay_active = false;
  Serial.printf("[DOOR] Initialized: threshold=%.1fm reset=%.1fm hits=%d relay_pin=%d\n",
                UNLOCK_THRESHOLD_M, RESET_THRESHOLD_M, REQUIRED_CONSECUTIVE_HITS, RELAY_PIN);
}

static void fireRelayPulse() {
  Serial.println("[DOOR] *** FIRING UNLOCK RELAY ***");
  digitalWrite(RELAY_PIN, HIGH);
  relay_active = true;
  relay_deactivate_time_ms = millis() + RELAY_PULSE_MS;
}

void handleRangingDistance(double distanceM) {
  // Input is expected to already be filtered by the session manager
  last_distance_m = distanceM;            // raw/received (may be equal to filtered if no raw kept)
  last_filtered_distance_m = distanceM;   // already-filtered value
  last_residual_m = 0.0;                  // residual not computed here

  // 1. Check if user is walking away to reset the lock state
  if (last_filtered_distance_m > RESET_THRESHOLD_M) {
    if (is_door_unlocked) {
      Serial.printf("[DOOR] User left the zone (distance=%.2fm > reset=%.2fm). Resetting lock state.\n",
                    last_filtered_distance_m, RESET_THRESHOLD_M);
      is_door_unlocked = false;  // Ready to unlock again next time they approach
    }
    consecutive_close_reads = 0;
    return;
  }

  // 2. If already unlocked and standing near the car, do nothing
  if (is_door_unlocked) {
    Serial.printf("[DOOR] Already unlocked. Ignoring (distance=%.2fm)\n", last_filtered_distance_m);
    return;
  }

  // 3. User is approaching. Check if they crossed the Unlock Threshold
  if (last_filtered_distance_m <= UNLOCK_THRESHOLD_M) {
    consecutive_close_reads++;
    Serial.printf("[DOOR] In zone! Hit count: %d/%d (distance=%.2fm)\n",
                  consecutive_close_reads, REQUIRED_CONSECUTIVE_HITS, last_filtered_distance_m);

    if (consecutive_close_reads >= REQUIRED_CONSECUTIVE_HITS) {
      // --- FIRE THE DOOR RELAY ---
      fireRelayPulse();
      is_door_unlocked = true;
      consecutive_close_reads = 0;
    }
  } else {
    // If they bounce between 2.1m and 1.9m, reset the counter
    if (consecutive_close_reads > 0) {
      Serial.printf("[DOOR] Distance bounced out of zone (%.2fm). Resetting counter.\n", last_filtered_distance_m);
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

double getLastFilteredDistance() {
  return last_filtered_distance_m;
}

double getLastResidual() {
  return last_residual_m;
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
  last_distance_m = 0.0;
  last_filtered_distance_m = 0.0;
  last_residual_m = 0.0;
  if (relay_active) {
    digitalWrite(RELAY_PIN, LOW);
    relay_active = false;
  }
}

// STAGE1_DISABLED: replaced by multi-anchor Stage 2 flow
#if 0
void handleRangingWithAI(double distanceM, float p_walk, float p_loiter, float p_attack) {
  // TÍNH TOÁN VẬN TỐC VẬT LÝ TỨC THỜI
  double current_velocity_m_per_frame = abs(distanceM - last_filtered_distance_m);
  last_filtered_distance_m = distanceM;

  // CHỐT CHẶN VẬT LÝ (KINEMATIC CHECK): Con người không thể đi nhanh hơn 3m/s (0.3m/frame 100ms)
  // Nếu tín hiệu giật lớn hơn 0.5m trong 1 frame -> 100% là sóng bị thao túng.
  if (current_velocity_m_per_frame > 0.5) {
    Serial.printf("[DOOR-SEC] LỖI VẬT LÝ! Tốc độ phi lý (%.2fm/frame). Ngăn chặn Relay Attack!\n", current_velocity_m_per_frame);
    consecutive_close_reads = 0;
    is_door_unlocked = false;
    if (relay_active) {
      digitalWrite(RELAY_PIN, LOW);
      relay_active = false;
    }
    return; // Dừng lập tức, không thèm quan tâm LSTM nói gì
  }

  // 1. HIGHEST PRIORITY: Attack Detection (security threshold at 0.70)
  if (p_attack > 0.70f) {
    Serial.printf("[DOOR-SEC] RELAY ATTACK DETECTED! p_attack=%.2f. System temporarily locked.\n", p_attack);
    consecutive_close_reads = 0;
    is_door_unlocked = false;
    if (relay_active) {
      digitalWrite(RELAY_PIN, LOW);
      relay_active = false;
      Serial.println("[DOOR-SEC] Relay immediately de-energized.");
    }
    return;
  }

  // 2. Check if user is walking away (reset state)
  if (last_filtered_distance_m > RESET_THRESHOLD_M) {
    if (is_door_unlocked) {
      Serial.printf("[DOOR] User left the zone (distance=%.2fm > reset=%.2fm). Resetting lock state.\n",
                    last_filtered_distance_m, RESET_THRESHOLD_M);
      is_door_unlocked = false;
    }
    consecutive_close_reads = 0;
    return;
  }

  // 3. If already unlocked, stay unlocked
  if (is_door_unlocked) {
    return;
  }

  // 4. NEW AI-ENHANCED LOGIC (CONSECUTIVE VOTING):
  // Door opens when BOTH: distance < 2m AND AI is confident (p_walk > 0.80)
  if (last_filtered_distance_m <= UNLOCK_THRESHOLD_M && p_walk > 0.80f) {
    consecutive_close_reads++;
    Serial.printf("[DOOR] In zone + AI confident! Hits: %d/%d (dist=%.2fm, p_walk=%.2f)\n",
                  consecutive_close_reads, REQUIRED_CONSECUTIVE_HITS,
                  last_filtered_distance_m, p_walk);

    if (consecutive_close_reads >= REQUIRED_CONSECUTIVE_HITS) {
      Serial.println("[DOOR] *** REACHED 3 CONSECUTIVE ATTEMPTS. UNLOCK DOOR ***");
      fireRelayPulse();
      is_door_unlocked = true;
      consecutive_close_reads = 0;
    }
  } else {
    // If either distance or AI confidence fails, reset the consecutive counter
    if (consecutive_close_reads > 0) {
      Serial.printf("[DOOR] AI confidence dropped (p_walk=%.2f) or user moved back. Resetting counter.\n",
                    p_walk);
    }
    consecutive_close_reads = 0;
  }
}
#endif  // STAGE1_DISABLED

}  // namespace UwbDoorUnlock
