#pragma once

#include <stdint.h>
#include <Arduino.h>

/**
 * Access state machine (Stage 2): deterministic 2D access control.
 *
 * Fuses the trilateration/EKF state (x, y, vx, vy) with the Geofence zones to
 * drive a three-state lifecycle:
 *
 *   LOCKED        --(approach & nearly-stop near door)--> DOOR_UNLOCKED
 *   DOOR_UNLOCKED --(sit still at driver seat)--> OCCUPIED (lock + ignition)
 *   OCCUPIED      --(walk away)--> LOCKED
 *   DOOR_UNLOCKED --(walk away w/o sitting)--> LOCKED
 *
 * A deceleration (near-stop) gate rejects people who merely walk past the door.
 * The AI intent classifier (Stage 2, later) will gate these transitions; this
 * module is the deterministic core.
 */
namespace AccessController {

enum class State : uint8_t { LOCKED, DOOR_UNLOCKED, OCCUPIED };

// ---- Tunables ---------------------------------------------------------------
constexpr double STILL_SPEED_MPS = 0.4;        // below this = standing still
constexpr int REQUIRED_CONSECUTIVE_HITS = 3;   // unlock debounce
constexpr uint32_t SEAT_SETTLE_MS = 3000;      // sit still this long -> lock + ignition
constexpr int LEAVE_CONSECUTIVE_HITS = 3;      // leave debounce
constexpr bool ENABLE_APPROACH_GATE = true;    // reject moving-away readings
constexpr double APPROACH_SPEED_MIN_MPS = 0.10;

// NOTE: The board (m5stack_atoms3 / ESP32-S3 with embedded PSRAM) reserves
// GPIO26 as the PSRAM chip-select (CONFIG_SPIRAM_CS_IO=26) and GPIO27 as an
// octal-PSRAM data line. Driving those pins as GPIO outputs crashes the SoC.
// Use free GPIOs instead (G5 = IR out, G7 = free on the ATOM S3); adjust to
// your relay/ignition wiring if different.
constexpr int RELAY_PIN = 5;        // door lock relay (LOW=locked, HIGH pulse=unlock)
constexpr int IGNITION_PIN = 7;     // engine-start authorization (HIGH=authorized)
constexpr int RELAY_PULSE_MS = 500;

// ---- API --------------------------------------------------------------------
void begin();

// Process a fused 2D position reading (optionally with EKF velocity).
void handlePosition(double x, double y);
void handlePosition(double x, double y, double vx, double vy);

// Background state management (relay pulse timing).
void tick();

// ---- Telemetry / control ----------------------------------------------------
State state();
bool isIgnitionAuthorized();
bool isDoorUnlocked();  // legacy alias: true only in DOOR_UNLOCKED

int getConsecutiveReadCount();
double getLastDistance();   // distance to the driver door
double getLastX();
double getLastY();
double getLastRadialSpeed();  // >0 moving away, <0 approaching (m/s)
double getLastSpeed();        // EKF speed magnitude (m/s)

void manualUnlock();
void resetDoorState();

}  // namespace AccessController
