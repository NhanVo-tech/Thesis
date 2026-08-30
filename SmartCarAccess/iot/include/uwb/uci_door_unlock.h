#pragma once

#include <stdint.h>
#include <Arduino.h>

/**
 * Door Unlock Module: Implements hysteresis-based door unlock logic.
 * 
 * Prevents relay chattering and false triggers by:
 * 1. Requiring 3 consecutive distance readings below 2.0m to unlock
 * 2. Entering cooldown after unlock; requires distance > 3.0m to reset
 */
namespace UwbDoorUnlock {

// ===== Configuration =====
constexpr double UNLOCK_THRESHOLD_M = 2.0;
constexpr double RESET_THRESHOLD_M = 3.0;
constexpr int REQUIRED_CONSECUTIVE_HITS = 3;
constexpr int RELAY_PIN = 26;  // GPIO pin for door relay (adjustable)
constexpr int RELAY_PULSE_MS = 500;  // How long to energize relay

// ===== Initialization & Control =====

/**
 * Initialize the door unlock module and GPIO pins
 */
void begin();

/**
 * Process a UWB ranging distance reading.
 * Implements hysteresis logic and fires relay when threshold is met.
 * @param distanceM Distance in meters from UWB ranging
 */
void handleRangingDistance(double distanceM);

/**
 * Process UWB distance with AI relay-attack detection.
 * The door only opens when distance < 2m AND AI confirms normal approach (p_walk > 0.80).
 * If p_attack > 0.70, the relay is temporarily disabled (attack protection).
 * @param distanceM Kalman-filtered distance in meters
 * @param p_walk Probability of normal walking approach [0-1]
 * @param p_loiter Probability of hovering / loitering [0-1]
 * @param p_attack Probability of relay attack [0-1]
 */
void handleRangingWithAI(double distanceM, float p_walk, float p_loiter, float p_attack);

/**
 * Tick function for background state management
 */
void tick();

/**
 * Get current door unlock state (for debugging/telemetry)
 */
bool isDoorUnlocked();
int getConsecutiveReadCount();
double getLastDistance();
double getLastFilteredDistance();
double getLastResidual();

/**
 * Manually trigger relay (for testing/admin commands)
 */
void manualUnlock();

/**
 * Reset door state (for debugging/admin commands)
 */
void resetDoorState();

}  // namespace UwbDoorUnlock
