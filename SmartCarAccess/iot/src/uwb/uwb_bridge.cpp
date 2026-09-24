#include "uwb/uwb_bridge.h"
#include "uwb/ranging_frame.h"
#include "uwb/uwb_geometry.h"
#include "uwb/trilateration.h"
#include "uwb/ekf_stub.h"
#include "uwb/access_controller.h"
#include "uwb/traj_inference.h"
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
uint32_t g_lastFixMs = 0;    // last time a trilateration fix corrected the EKF
TrajInference g_trajInference;

// Cadence at which the EKF estimate drives the access controller. Decoupling
// this from the raw RANGE rate keeps the unlock debounce timing consistent and
// lets prediction bridge dropped frames.
constexpr uint32_t kDrivePeriodMs = 100;
// Stop predicting/coasting once no fresh fix has arrived for this long, so the
// EKF stops fabricating a straight-line trajectory while the user is out of
// range (the UI freezes instead of later snapping back).
constexpr uint32_t kMaxCoastMs = 500;

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
  // Per-anchor mask: the bridge zeroes stale/out-of-bounds distances, so any
  // positive distance is a fresh in-bounds measurement. This lets solve() use
  // the 2-anchor path when one anchor drops out, or return no fix below two.
  (void)valid;  // kept for protocol/log compatibility
  out->valid_mask = 0;
  if (d0 > 0.0) out->valid_mask |= 1u << 0;
  if (d1 > 0.0) out->valid_mask |= 1u << 1;
  if (d2 > 0.0) out->valid_mask |= 1u << 2;
  return true;
}

}  // namespace

void begin() {
  if (!g_frameQueue) {
    g_frameQueue = xQueueCreate(8, sizeof(RangingFrame));
  }
  g_ranging = false;
  g_trajInference.begin();
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
    // valid_mask bits are d2 d1 d0 (MSB..LSB); n = number of fresh anchors.
    const unsigned n = (f.valid_mask & 1u) + ((f.valid_mask >> 1) & 1u) +
                       ((f.valid_mask >> 2) & 1u);
    Serial.printf("[RANGE3] t=%lu d0=%.2f d1=%.2f d2=%.2f n=%u mask=%c%c%c\n",
                  (unsigned long)f.t_ms, f.d[0], f.d[1], f.d[2], n,
                  (f.valid_mask & 4u) ? '1' : '0',
                  (f.valid_mask & 2u) ? '1' : '0',
                  (f.valid_mask & 1u) ? '1' : '0');
    if (f.valid_mask == 0) continue;

    Trilateration::Result r = Trilateration::solve(
        UwbGeo::kAnchorX, UwbGeo::kAnchorY, f.d, f.valid_mask);
    if (!r.valid) continue;

    Serial.printf("[POS2D] t=%lu x=%.2f y=%.2f rms=%.3f\n",
                  (unsigned long)f.t_ms, r.x, r.y, r.rms);
    // RMS weights how much the raw fix is trusted.
    Ekf::update(r.x, r.y, f.t_ms, r.rms);
    g_lastFixMs = f.t_ms;
  }

  // 2. Fixed-rate drive: emit the fused position + velocity to the door logic.
  //    There is NO prediction/coasting between fixes — the position is frozen
  //    at the last corrected value, so when ranging drops out the UI stops
  //    moving instead of drawing a fake straight-line trajectory. The velocity
  //    is reported as-is (it is the EKF's last corrected estimate) because the
  //    access controller's speed/deceleration gates depend on an accurate value;
  //    zeroing it here made "pass-by" reads look stationary and unlock the door.
  const uint32_t now = millis();
  if (now - g_lastDriveMs >= kDrivePeriodMs) {
    g_lastDriveMs = now;
    if (now - g_lastFixMs <= kMaxCoastMs && Ekf::initialized()) {
      const double fx = Ekf::x();
      const double fy = Ekf::y();
      Serial.printf("[EKF] t=%lu x=%.2f y=%.2f vx=%.2f vy=%.2f v=%.2f\n",
                    (unsigned long)now, fx, fy, Ekf::vx(), Ekf::vy(),
                    Ekf::speed());
      float intent[TrajInference::NUM_CLASSES] = {0.0f, 0.0f, 0.0f, 0.0f};
      const float* intent_ptr = nullptr;
      if (g_trajInference.predict(static_cast<float>(fx),
                                  static_cast<float>(fy),
                                  static_cast<float>(Ekf::vx()),
                                  static_cast<float>(Ekf::vy()),
                                  intent)) {
        Serial.printf("[INTENT] p_approach=%.3f p_seated=%.3f "
                      "p_leave=%.3f p_passing=%.3f\n",
                      intent[0], intent[1], intent[2], intent[3]);
        intent_ptr = intent;
      }
      AccessController::handlePosition(fx, fy, Ekf::vx(), Ekf::vy(),
                                       intent_ptr);
    }
  }
}

void sendStart() {
  if (g_ranging) return;
  g_ranging = true;
  Ekf::reset();
  g_lastDriveMs = 0;
  g_lastFixMs = 0;
  Serial.println("CMD:START_RANGING");
}

void sendStop() {
  if (!g_ranging) return;
  g_ranging = false;
  Serial.println("CMD:STOP_RANGING");
}

}  // namespace UwbBridge
