#include "espnow_link.h"
#include <esp_now.h>
#include <WiFi.h>
#include <cstring>

namespace EspNowLink {

#pragma pack(push, 1)
struct StartMsg {
  uint8_t msg_type;        // 0x01
  uint32_t session_id;
  bool     controlee;      // true = car is controlee
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

struct RangingMsg {
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
uint8_t g_masterMac[6] = {};
bool g_peerAdded = false;
UciSession::Config g_pendingCfg;
uint8_t g_rawStartMsg[sizeof(StartMsg)];
volatile bool g_hasPending = false;
}  // namespace

static void onRecv(const uint8_t* mac, const uint8_t* data, int len) {
  (void)mac;
  if (len != sizeof(StartMsg)) return;
  // Minimal ISR — just copy raw bytes to static buffer, set flag
  memcpy(g_rawStartMsg, data, sizeof(StartMsg));
  g_hasPending = true;
}

static void onSent(const uint8_t* mac, esp_now_send_status_t status) {
  // silent
}

bool begin(const uint8_t* masterMac) {
  memcpy(g_masterMac, masterMac, 6);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] init fail");
    return false;
  }
  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, g_masterMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.printf("[ESPNOW] peer fail: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  masterMac[0], masterMac[1], masterMac[2],
                  masterMac[3], masterMac[4], masterMac[5]);
    return false;
  }
  g_peerAdded = true;

  Serial.printf("[ESPNOW] master peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                masterMac[0], masterMac[1], masterMac[2],
                masterMac[3], masterMac[4], masterMac[5]);
  Serial.printf("[ESPNOW] my MAC: %s\n", WiFi.macAddress().c_str());
  return true;
}

bool hasPendingStart() { return g_hasPending; }

UciSession::Config getPendingConfig() {
  g_hasPending = false;
  const StartMsg* sm = reinterpret_cast<const StartMsg*>(g_rawStartMsg);
  UciSession::Config cfg;
  cfg.session_id     = sm->session_id;
  cfg.controlee      = sm->controlee;
  cfg.phone_mac      = sm->phone_mac;
  cfg.car_mac        = sm->car_mac;
  cfg.channel        = sm->channel;
  cfg.preamble_idx   = sm->preamble_idx;
  cfg.sfd_id         = sm->sfd_id;
  cfg.sts_config     = sm->sts_config;
  cfg.hopping_mode   = sm->hopping_mode;
  cfg.rframe_config  = sm->rframe_config;
  cfg.result_report_config = sm->result_report_config;
  cfg.aoa_result_req = sm->aoa_result_req;
  cfg.schedule_mode  = sm->schedule_mode;
  cfg.slot_duration  = sm->slot_duration;
  cfg.ranging_interval = sm->ranging_interval;
  cfg.slots_per_rr   = sm->slots_per_rr;
  cfg.vendor_id      = sm->vendor_id;
  memcpy(cfg.static_sts_iv, sm->static_sts_iv, 6);

  Serial.printf("[ESPNOW] START session_id=%lu dest=0x%04X\n",
                static_cast<unsigned long>(cfg.session_id), cfg.phone_mac);
  return cfg;
}

void sendRanging(uint32_t seq, uint16_t distCm, uint8_t status,
                 uint8_t nlos, int8_t rssiDbm) {
  if (!g_peerAdded) return;

  RangingMsg rm = {};
  rm.msg_type = 0x10;
  rm.anchor_id = ANCHOR_ID;
  rm.seq_number = seq;
  rm.distance_cm = distCm;
  rm.status = status;
  rm.nlos = nlos;
  rm.rssi_neg_dbm = rssiDbm;

  esp_now_send(g_masterMac, reinterpret_cast<const uint8_t*>(&rm), sizeof(rm));
}

}  // namespace EspNowLink
