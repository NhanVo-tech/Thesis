#include "uwb/uci_host_bridge.h"
#include "uwb/uci_door_unlock.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_coexist.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstring>

namespace UwbUciHost {

#pragma pack(push, 1)
struct StartSession {
  uint8_t msg_type;
  uint32_t session_id;
  bool     controlee;
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
  uint32_t ranging_interval;
  uint8_t slots_per_rr;
  uint16_t vendor_id;
  uint8_t static_sts_iv[6];
};

struct RangingReport {
  uint8_t msg_type;
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

UwbUci::UciRunConfig g_pendingCfg;
bool g_hasCachedConfig = false;
bool g_pendingStart = false;
bool g_pendingStop = false;
bool g_busy = false;

volatile uint32_t g_recvCnt = 0, g_recvBadLen = 0, g_recvBadType = 0;

void onRecv(const uint8_t* mac, const uint8_t* data, int len) {
  (void)mac;
  g_recvCnt++;
  if (len != sizeof(RangingReport)) { g_recvBadLen++; return; }
  const RangingReport* rr = reinterpret_cast<const RangingReport*>(data);
  if (rr->msg_type != 0x10) { g_recvBadType++; return; }
  if (!g_rangingQueue) return;
  BaseType_t woken = pdFALSE;
  xQueueSendFromISR(g_rangingQueue, rr, &woken);
  if (woken == pdTRUE) portYIELD_FROM_ISR();
}

void onSent(const uint8_t* mac, esp_now_send_status_t status) {}

void sendStartToAux(uint8_t aid, const UwbUci::UciRunConfig& cfg) {
  if (!g_espnowInit || aid >= g_auxCount) return;

  StartSession ss = {};
  ss.msg_type = 0x01;
  ss.session_id = cfg.sessionId;
  ss.controlee = cfg.controlee;
  ss.phone_mac = cfg.destMac;
  ss.car_mac = cfg.localMac;
  ss.channel = cfg.channel;
  ss.preamble_idx = cfg.preambleIdx;
  ss.sfd_id = cfg.sfd;
  ss.sts_config = cfg.stsConfig;
  ss.hopping_mode = cfg.hoppingMode;
  ss.rframe_config = cfg.rframeConfig;
  ss.result_report_config = cfg.resultReportConfig;
  ss.aoa_result_req = cfg.aoaReport;
  ss.schedule_mode = cfg.scheduleMode;
  ss.slot_duration = cfg.slotDuration;
  ss.ranging_interval = cfg.rangingDuration;
  ss.slots_per_rr = cfg.slotsPerRr;
  ss.vendor_id = cfg.vendorId;
  memcpy(ss.static_sts_iv, cfg.staticStsIv, 6);

  esp_err_t err = esp_now_send(g_auxMacs[aid],
                               reinterpret_cast<const uint8_t*>(&ss),
                               sizeof(ss));
  Serial.printf("[ESPNOW] START→anchor-%u: %s\n", aid,
                err == ESP_OK ? "OK" : "FAIL");
}

}  // namespace

bool begin(const uint8_t auxMacs[][6], uint8_t count) {
  if (count > kMaxAnchors) count = kMaxAnchors;
  if (count == 0) return false;

  memcpy(g_auxMacs, auxMacs, count * 6);
  g_auxCount = count;

  g_rangingQueue = xQueueCreate(32, sizeof(RangingReport));
  if (!g_rangingQueue) {
    Serial.println("[ESPNOW-M] queue fail");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_start();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_ps(WIFI_PS_NONE);
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW-M] init fail");
    return false;
  }
  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSent);

  esp_coex_preference_set(ESP_COEX_PREFER_BALANCE);

  for (uint8_t i = 0; i < g_auxCount; ++i) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, g_auxMacs[i], 6);
    peer.channel = 0;
    peer.encrypt = false;
    peer.ifidx = WIFI_IF_STA;
    if (esp_now_add_peer(&peer) != ESP_OK) {
      Serial.printf("[ESPNOW-M] peer anchor-%u fail\n", i);
      return false;
    }
  }

  g_espnowInit = true;
  Serial.printf("[ESPNOW-M] %u anchors ready, master MAC: %s\n",
                g_auxCount, WiFi.macAddress().c_str());
  return true;
}

bool submitBleOob(const uint8_t* payload, size_t len, const char** err) {
  if (err) *err = nullptr;
  if (g_busy) { if (err) *err = "busy"; return false; }

  UwbUci::UciOobPayloadV1 oob;
  const char* pe = nullptr;
  if (!UwbUci::parseOobPayloadV1(payload, len, &oob, &pe)) {
    if (err) *err = pe; return false;
  }
  UwbUci::mapOobToRunConfig(oob, &g_pendingCfg);
  g_hasCachedConfig = true;
  g_pendingStart = false;

  Serial.printf("[OOB] cached sid=%lu dest=0x%04X ch=%u\n",
                static_cast<unsigned long>(g_pendingCfg.sessionId),
                g_pendingCfg.destMac,
                g_pendingCfg.channel);
  return true;
}

bool submitConfig(const UwbUci::UciRunConfig& cfg, const char** err) {
  if (err) *err = nullptr;
  if (g_busy) { if (err) *err = "busy"; return false; }
  g_pendingCfg = cfg;
  g_hasCachedConfig = true;
  g_pendingStart = false;
  return true;
}

bool requestStart(const char** err) {
  if (err) *err = nullptr;
  if (g_busy) { if (err) *err = "busy"; return false; }
  if (!g_hasCachedConfig) { if (err) *err = "no_oob"; return false; }
  if (g_pendingStart) { if (err) *err = "pending"; return false; }
  g_pendingStart = true;
  return true;
}

bool requestStop(const char** err) {
  if (err) *err = nullptr;
  g_pendingStop = true;
  return true;
}

bool hasCachedConfig() { return g_hasCachedConfig; }
bool isBusy() { return g_busy; }
bool hasPending() { return g_pendingStart || g_pendingStop; }

void tick() {
  if (g_busy) return;

  if (g_pendingStop) {
    g_pendingStop = false;
    // Stop not implemented for now — anchors stop automatically when session expires
  }

  if (g_pendingStart) {
    g_pendingStart = false;
    g_busy = true;

    for (uint8_t i = 0; i < g_auxCount; ++i) {
      Serial.printf("[OOB] anchor-%u starting...\n", i);
      sendStartToAux(i, g_pendingCfg);
    }
    Serial.println("[OOB] all anchors triggered");

    g_busy = false;
  }

  // Drain ranging data from queue
  static uint32_t s_last = 0;
  if (g_recvCnt != s_last) {
    Serial.printf("[ESPNOW-M] recv: cnt=%lu badLen=%lu badType=%lu\n",
                  static_cast<unsigned long>(g_recvCnt),
                  static_cast<unsigned long>(g_recvBadLen),
                  static_cast<unsigned long>(g_recvBadType));
    s_last = g_recvCnt;
  }
  if (g_rangingQueue) {
    RangingReport rr;
    while (xQueueReceive(g_rangingQueue, &rr, 0) == pdTRUE) {
      double distM = static_cast<double>(static_cast<int16_t>(rr.distance_cm)) / 100.0;
      Serial.printf("[ESPNOW] ranging a%u seq=%u dist=%.2fm\n",
                    rr.anchor_id,
                    static_cast<unsigned>(rr.seq_number),
                    distM);
      // Feed to door unlock / ranging pipeline
      UwbDoorUnlock::handleRangingDistance(distM);
    }
  }
}

}  // namespace UwbUciHost
