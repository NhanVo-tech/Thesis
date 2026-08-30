#include "app/anchor_bridge.h"
#include "app/oob_parser.h"
#include "uwb/uci_door_unlock.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstring>

namespace AnchorBridge {

#pragma pack(push, 1)
struct EspNowStart {
  uint8_t msg_type;        // 0x01
  uint32_t session_id;
  bool     controlee;      // true = car is controlee (responder)
  uint16_t phone_mac;
  uint16_t car_mac;
  uint8_t channel;
  uint8_t preamble_idx;
  uint8_t sfd_id;
  uint8_t sts_config;
  uint8_t hopping_mode;
  uint8_t rframe_config;
  uint8_t result_report_config;
  uint8_t aoa_result_req;
  uint8_t schedule_mode;
  uint16_t slot_duration;
  uint32_t ranging_interval_ms;
  uint8_t slots_per_rr;
  uint16_t vendor_id;
  uint8_t static_sts_iv[6];
};

struct EspNowReport {
  uint8_t msg_type;        // 0x10
  uint8_t anchor_id;
  uint32_t seq_number;
  uint16_t distance_cm;
  uint8_t status;
  uint8_t nlos;
  int8_t rssi_neg_dbm;
};
#pragma pack(pop)

namespace {

constexpr uint8_t kMaxAnchors = 3;
uint8_t g_auxMacs[kMaxAnchors][6] = {};
uint8_t g_auxCount = 0;
bool g_espnowInit = false;
QueueHandle_t g_rangingQueue = nullptr;

App::SessionConfig g_pendingCfg;
bool g_hasCachedCfg = false;
bool g_pendingStart = false;
bool g_busy = false;

void onRecv(const uint8_t* mac, const uint8_t* data, int len) {
  (void)mac;
  if (len != sizeof(EspNowReport)) return;
  const EspNowReport* r = reinterpret_cast<const EspNowReport*>(data);
  if (r->msg_type != 0x10) return;
  if (!g_rangingQueue) return;
  BaseType_t woken = pdFALSE;
  xQueueSendFromISR(g_rangingQueue, r, &woken);
  if (woken == pdTRUE) portYIELD_FROM_ISR();
}

void onSent(const uint8_t* mac, esp_now_send_status_t status) {}

}  // namespace

bool begin(const uint8_t auxMacs[][6], uint8_t count) {
  if (count > kMaxAnchors) count = kMaxAnchors;
  memcpy(g_auxMacs, auxMacs, count * 6);
  g_auxCount = count;

  g_rangingQueue = xQueueCreate(32, sizeof(EspNowReport));
  if (!g_rangingQueue) return false;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSent);

  for (uint8_t i = 0; i < g_auxCount; ++i) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, g_auxMacs[i], 6);
    peer.channel = 0;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) {
      Serial.printf("[BRIDGE] peer anchor-%u fail\n", i);
      return false;
    }
  }

  g_espnowInit = true;
  Serial.printf("[BRIDGE] %u anchors, master MAC: %s\n",
                g_auxCount, WiFi.macAddress().c_str());
  return true;
}

bool submitBleOob(const uint8_t* payload, size_t len, const char** err) {
  if (err) *err = nullptr;
  if (g_busy) { if (err) *err = "busy"; return false; }

  App::SessionConfig cfg;
  const char* pe = nullptr;
  if (!OobParser::parse(payload, len, &cfg, &pe)) {
    if (err) *err = pe; return false;
  }

  g_pendingCfg = cfg;
  g_hasCachedCfg = true;
  g_pendingStart = false;

  Serial.printf("[BRIDGE] OOB cached sid=%lu phone=0x%04X car=0x%04X ch=%u\n",
                static_cast<unsigned long>(cfg.session_id),
                cfg.phone_mac, cfg.car_mac, cfg.channel);
  return true;
}

bool submitConfig(const App::SessionConfig& cfg, const char** err) {
  if (err) *err = nullptr;
  if (g_busy) { if (err) *err = "busy"; return false; }
  g_pendingCfg = cfg;
  g_hasCachedCfg = true;
  g_pendingStart = false;
  return true;
}

bool requestStart(const char** err) {
  if (err) *err = nullptr;
  if (!g_hasCachedCfg) { if (err) *err = "no_config"; return false; }
  if (g_pendingStart) { if (err) *err = "pending"; return false; }
  g_pendingStart = true;
  return true;
}

bool requestStop(const char** err) {
  if (err) *err = nullptr;
  // Stop not implemented — sessions expire naturally
  return true;
}

bool hasCachedConfig() { return g_hasCachedCfg; }
bool isBusy() { return g_busy; }

void tick() {
  if (g_busy) return;

  // Flush ranging data to door unlock
  if (g_rangingQueue) {
    EspNowReport r;
    while (xQueueReceive(g_rangingQueue, &r, 0) == pdTRUE) {
      double distM = static_cast<double>(static_cast<int16_t>(r.distance_cm)) / 100.0;
      Serial.printf("[BRIDGE] ranging a%u seq=%u dist=%.2fm\n",
                    r.anchor_id,
                    static_cast<unsigned>(r.seq_number),
                    distM);
      UwbDoorUnlock::handleRangingDistance(distM);
    }
  }

  // Start session on all anchors
  if (!g_pendingStart) return;
  g_pendingStart = false;
  g_busy = true;

  for (uint8_t i = 0; i < g_auxCount; ++i) {
    EspNowStart ss = {};
    ss.msg_type      = 0x01;
    ss.session_id    = g_pendingCfg.session_id;
    ss.controlee     = g_pendingCfg.controlee;
    ss.phone_mac     = g_pendingCfg.phone_mac;
    ss.car_mac       = g_pendingCfg.car_mac;
    ss.channel       = g_pendingCfg.channel;
    ss.preamble_idx  = g_pendingCfg.preamble_idx;
    ss.sfd_id        = g_pendingCfg.sfd_id;
    ss.sts_config    = g_pendingCfg.sts_config;
    ss.hopping_mode  = g_pendingCfg.hopping_mode;
    ss.rframe_config = g_pendingCfg.rframe_config;
    ss.result_report_config = g_pendingCfg.result_report_config;
    ss.aoa_result_req = g_pendingCfg.aoa_result_req;
    ss.schedule_mode = g_pendingCfg.schedule_mode;
    ss.slot_duration = g_pendingCfg.slot_duration;
    ss.ranging_interval_ms = g_pendingCfg.ranging_interval_ms;
    ss.slots_per_rr  = g_pendingCfg.slots_per_rr;
    ss.vendor_id     = g_pendingCfg.vendor_id;
    memcpy(ss.static_sts_iv, g_pendingCfg.static_sts_iv, 6);

    esp_err_t err = esp_now_send(g_auxMacs[i],
                                 reinterpret_cast<const uint8_t*>(&ss),
                                 sizeof(ss));
    Serial.printf("[BRIDGE] START→anchor-%u: %s\n", i,
                  err == ESP_OK ? "OK" : "FAIL");
  }

  g_busy = false;
}

}  // namespace AnchorBridge
