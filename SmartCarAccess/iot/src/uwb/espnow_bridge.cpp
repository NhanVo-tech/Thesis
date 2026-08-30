#include "uwb/espnow_bridge.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstring>

namespace EspNowBridge {

namespace {

constexpr uint8_t kMaxAnchors = 3;

RangingCallback g_callback = nullptr;
uint8_t g_auxMacs[kMaxAnchors][6] = {};
uint8_t g_auxCount = 0;
bool g_initialized = false;

QueueHandle_t g_rangingQueue = nullptr;

void onRecv(const uint8_t* mac, const uint8_t* data, int len) {
  if (len != sizeof(RangingData)) return;
  const RangingData* rd = reinterpret_cast<const RangingData*>(data);
  if (rd->msg_type != 0x10) return;
  if (rd->anchor_id >= g_auxCount) return;

  // Copy to queue for thread-safe handling
  RangingData copy = *rd;
  xQueueSendFromISR(g_rangingQueue, &copy, nullptr);
  portYIELD_FROM_ISR();
}

void onSent(const uint8_t* mac, esp_now_send_status_t status) {
  // optional debug
}

}  // namespace

bool begin(const uint8_t auxMacs[][6], uint8_t auxCount) {
  if (auxCount > kMaxAnchors) auxCount = kMaxAnchors;
  if (auxCount == 0) return false;

  // Create queue for ranging data (max 16 pending)
  g_rangingQueue = xQueueCreate(16, sizeof(RangingData));
  if (!g_rangingQueue) {
    Serial.println("[ESPNOW-M] queue create failed");
    return false;
  }

  memcpy(g_auxMacs, auxMacs, auxCount * 6);
  g_auxCount = auxCount;

  // WiFi init — ESP32-S3 must coexist with NimBLE
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW-M] init failed");
    return false;
  }

  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSent);

  // Register each aux as peer
  for (uint8_t i = 0; i < g_auxCount; ++i) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, g_auxMacs[i], 6);
    peer.channel = 0;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) {
      Serial.printf("[ESPNOW-M] add peer anchor-%u failed\n", i);
      return false;
    }
  }

  g_initialized = true;

  Serial.printf("[ESPNOW-M] ready with %u anchors\n", g_auxCount);
  for (uint8_t i = 0; i < g_auxCount; ++i) {
    Serial.printf("[ESPNOW-M]   anchor-%u: %s\n", i, getAuxMacStr(i));
  }
  Serial.printf("[ESPNOW-M] master MAC: %s\n", WiFi.macAddress().c_str());

  return true;
}

bool sendUciToAux(uint8_t anchorId, const uint8_t* uciPayload, uint16_t len) {
  if (!g_initialized || anchorId >= g_auxCount) return false;
  if (len > sizeof(CmdToAux::payload)) return false;

  static uint8_t reqId = 0;
  CmdToAux cmd = {};
  cmd.msg_type = 0x01;
  cmd.request_id = ++reqId;
  cmd.payload_len = len;
  memcpy(cmd.payload, uciPayload, len);

  esp_err_t err = esp_now_send(g_auxMacs[anchorId],
                               reinterpret_cast<const uint8_t*>(&cmd),
                               sizeof(CmdToAux));
  if (err != ESP_OK) {
    Serial.printf("[ESPNOW-M] send to anchor-%u failed: %d\n", anchorId, err);
    return false;
  }
  return true;
}

void setRangingCallback(RangingCallback cb) {
  g_callback = cb;
}

const char* getAuxMacStr(uint8_t anchorId) {
  static char buf[18];
  if (anchorId >= g_auxCount) return "??:??:??:??:??:??";
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           g_auxMacs[anchorId][0], g_auxMacs[anchorId][1],
           g_auxMacs[anchorId][2], g_auxMacs[anchorId][3],
           g_auxMacs[anchorId][4], g_auxMacs[anchorId][5]);
  return buf;
}

void poll() {
  if (!g_rangingQueue) return;

  RangingData rd;
  while (xQueueReceive(g_rangingQueue, &rd, 0) == pdTRUE) {
    Serial.printf("[ESPNOW-M] ranging anchor-%u seq=%u dist=%d cm status=0x%02X nlos=%u rssi=%d\n",
                  rd.anchor_id,
                  static_cast<unsigned>(rd.seq_number),
                  static_cast<int>(static_cast<int16_t>(rd.distance_cm)),
                  rd.status, rd.nlos,
                  static_cast<int>(rd.rssi_neg_dbm));
    if (g_callback) {
      g_callback(rd);
    }
  }
}

}  // namespace EspNowBridge
