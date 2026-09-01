#pragma once

#include <stdint.h>
#include <Arduino.h>

/**
 * Access Controller: hysteresis-based 2D unlock logic.
 *
 * Replaces the Stage 1 single-distance (1D) door unlock. Takes the fused
 * (x, y) position from trilateration/EKF, computes the Euclidean distance to
 * the unlock point (driver door), and applies hysteresis + consecutive-hit
 * debouncing before firing the relay.
 */
namespace AccessController {

// ===== Configuration =====
// Driver-door unlock point in car-frame metres (near anchor A1 / left side).
constexpr double kUnlockPointX = -0.85;
constexpr double kUnlockPointY = 0.0;
constexpr double UNLOCK_RADIUS_M = 2.0;   // fire relay inside this radius
constexpr double RESET_RADIUS_M = 3.0;    // re-arm after leaving this radius
constexpr int REQUIRED_CONSECUTIVE_HITS = 3;
// Approach gate: only accumulate unlock hits while the user is moving toward
// the door. A radial velocity above this (moving away) blocks/resets counting.
constexpr bool ENABLE_APPROACH_GATE = true;
constexpr double APPROACH_SPEED_MIN_MPS = 0.10;
constexpr int RELAY_PIN = 26;  // GPIO pin for door relay (adjustable)
constexpr int RELAY_PULSE_MS = 500;  // How long to energize relay

// ===== Initialization & Control =====

/**
 * Initialize the access controller and GPIO pins.
 */
void begin();

/**
 * Process a 2D position reading (x, y in metres, car frame).
 * Implements hysteresis logic and fires the relay when inside the unlock zone.
 * @param x X coordinate in metres
 * @param y Y coordinate in metres
 */
void handlePosition(double x, double y);

/**
 * Process a 2D position reading together with the EKF velocity estimate.
 * Adds an approach gate: unlock hits only accumulate while the user moves
 * toward the door (radial velocity), which rejects people walking past/away.
 * @param x  X coordinate in metres
 * @param y  Y coordinate in metres
 * @param vx X velocity in metres/second (car frame)
 * @param vy Y velocity in metres/second (car frame)
 */
void handlePosition(double x, double y, double vx, double vy);

/**
 * Tick function for background state management (relay pulse timing).
 */
void tick();

// ===== Telemetry / debugging =====
bool isDoorUnlocked();
int getConsecutiveReadCount();
double getLastDistance();  // distance to the unlock point
double getLastX();
double getLastY();
double getLastRadialSpeed();  // >0 moving away, <0 approaching (m/s)

/**
 * Manually trigger relay (for testing/admin commands).
 */
void manualUnlock();

/**
 * Reset door state (for debugging/admin commands).
 */
void resetDoorState();

}  // namespace AccessController
