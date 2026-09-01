#include "uwb/uwb_bridge.h"
#include "uwb/ranging_frame.h"
#include "uwb/uwb_geometry.h"
#include "uwb/trilateration.h"
#include "uwb/ekf_stub.h"
#include "uwb/access_controller.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstdio>
#include <cstring>

namespace UwbBridge {
namespace {

QueueHandle_t g_frameQueue = nullptr;
bool g_ranging = false;
uint32_t g_lastDriveMs = 0;  // last time the door logic was driven

// Cadence at which the EKF estimate drives the access controller. Decoupling
// this from the raw RANGE rate keeps the unlock debounce timing consistent and
// lets prediction bridge dropped frames.
constexpr uint32_t kDrivePeriodMs = 100;

bool parseRange(const char* line, RangingFrame* out) {
  const char* p = line + 6;  // skip "RANGE:"
  double d0 = 0.0, d1 = 0.0, d2 = 0.0;
  int valid = 0;
  if (sscanf(p, "d0=%lf,d1=%lf,d2=%lf,valid=%d", &d0, &d1, &d2, &valid) != 4) {
    return false;
  }
  out->t_ms = millis();
  out->d[0] = d0;
  out->d[1] = d1;
  out->d[2] = d2;
  out->valid_mask = valid ? 0x07 : 0x00;
  return true;
}

}  // namespace

void begin() {
  if (!g_frameQueue) {
    g_frameQueue = xQueueCreate(8, sizeof(RangingFrame));
  }
  g_ranging = false;
  Serial.println("[BRIDGE] UWB PC bridge ready (RANGE/CMD over USB-CDC)");
}

void feedLine(const char* line) {
  if (!line) return;
  if (strncmp(line, "RANGE:", 6) == 0) {
    RangingFrame f;
    if (parseRange(line, &f) && g_frameQueue) {
      xQueueSend(g_frameQueue, &f, 0);
    }
    return;
  }
  if (strncmp(line, "ACK:", 4) == 0) {
    Serial.printf("[BRIDGE] %s\n", line);
    return;
  }
}

void tick() {
  if (!g_frameQueue) return;

  // 1. Drain RANGE frames: each one corrects the EKF (no direct door drive).
  RangingFrame f;
  while (xQueueReceive(g_frameQueue, &f, 0) == pdTRUE) {
    Serial.printf("[RANGE3] t=%lu d0=%.2f d1=%.2f d2=%.2f valid=%u\n",
                  (unsigned long)f.t_ms, f.d[0], f.d[1], f.d[2],
                  (unsigned)f.valid_mask);
    if (f.valid_mask == 0) continue;

    Trilateration::Result r = Trilateration::solve(
        UwbGeo::kAnchorX, UwbGeo::kAnchorY, f.d, f.valid_mask);
    if (!r.valid) continue;

    Serial.printf("[POS2D] x=%.2f y=%.2f rms=%.3f\n", r.x, r.y, r.rms);
    // RMS weights how much the raw fix is trusted.
    Ekf::update(r.x, r.y, f.t_ms, r.rms);
  }

  // 2. Fixed-rate drive: predict the EKF forward (bridges dropped frames) and
  //    feed the smoothed position + velocity to the door logic.
  const uint32_t now = millis();
  if (now - g_lastDriveMs >= kDrivePeriodMs) {
    g_lastDriveMs = now;
    if (Ekf::predictTo(now)) {
      const double fx = Ekf::x();
      const double fy = Ekf::y();
      Serial.printf("[EKF] x=%.2f y=%.2f vx=%.2f vy=%.2f v=%.2f\n",
                    fx, fy, Ekf::vx(), Ekf::vy(), Ekf::speed());
      AccessController::handlePosition(fx, fy, Ekf::vx(), Ekf::vy());
    }
  }
}

void sendStart() {
  if (g_ranging) return;
  g_ranging = true;
  Ekf::reset();
  g_lastDriveMs = 0;
  Serial.println("CMD:START_RANGING");
}

void sendStop() {
  if (!g_ranging) return;
  g_ranging = false;
  Serial.println("CMD:STOP_RANGING");
}

}  // namespace UwbBridge