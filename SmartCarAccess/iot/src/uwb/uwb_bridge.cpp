#include "uwb/uwb_bridge.h"
#include "uwb/ranging_frame.h"
#include "uwb/uwb_geometry.h"
#include "uwb/trilateration.h"
#include "uwb/ekf_stub.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstdio>
#include <cstring>

namespace UwbBridge {
namespace {

QueueHandle_t g_frameQueue = nullptr;
bool g_ranging = false;

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
    Ekf::update(r.x, r.y);
  }
}

void sendStart() {
  if (g_ranging) return;
  g_ranging = true;
  Serial.println("CMD:START_RANGING");
}

void sendStop() {
  if (!g_ranging) return;
  g_ranging = false;
  Serial.println("CMD:STOP_RANGING");
}

}  // namespace UwbBridge