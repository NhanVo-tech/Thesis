#pragma once

// PC USB-CDC bridge (shares Arduino Serial with the console).
// Receives "RANGE:d0=,d1=,d2=,valid=" frames from the PC bridge script,
// runs trilateration in the UWB task, and emits "CMD:START/STOP_RANGING".
namespace UwbBridge {

void begin();

// Feed one complete, trimmed line read from Serial.
// Recognizes "RANGE:" (enqueues a frame) and "ACK:" (command confirmation).
void feedLine(const char* line);

// Drain queued frames: log [RANGE3], trilaterate, log [POS2D], push to EKF stub.
void tick();

// Control the PC-side ranging session (idempotent).
void sendStart();
void sendStop();

}  // namespace UwbBridge