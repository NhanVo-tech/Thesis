#include "uwb/espnow_uci_link.h"
#include "uwb/uci_uart_link.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace UwbUci {

bool EspNowUciLink::s_wifiInit = false;
EspNowUciLink* EspNowUciLink::s_instances[3] = {nullptr, nullptr, nullptr};

namespace {

QueueHandle_t g_rxQueue = nullptr;
volatile uint32_t g_rxCount = 0;  // debug: count received messages

void ensureQueue() {
  if (!g_rxQueue) {
    g_rxQueue = xQueueCreate(32, sizeof(EspNowMsg));
  }
}

}  // namespace

EspNowUciLink::EspNowUciLink()
    : anchorId_(0), callback_(), rxBuffer_(), ready_(false) {
  memset(peerMac_, 0, sizeof(peerMac_));
}

void EspNowUciLink::globalRecvCb(const uint8_t* mac, const uint8_t* data, int len) {
  (void)mac;
  if (len != sizeof(EspNowMsg)) return;
  g_rxCount++;
  const EspNowMsg* msg = reinterpret_cast<const EspNowMsg*>(data);
  if (msg->anchor_id > 2) return;

  ensureQueue();
  BaseType_t woken = pdFALSE;
  xQueueSendFromISR(g_rxQueue, msg, &woken);
  if (woken == pdTRUE) portYIELD_FROM_ISR();
}

void EspNowUciLink::globalSendCb(const uint8_t* mac, esp_now_send_status_t status) {
  // silent
}

bool EspNowUciLink::begin(uint8_t anchorId, const uint8_t* mac) {
  if (anchorId > 2) return false;
  anchorId_ = anchorId;
  memcpy(peerMac_, mac, 6);

  if (!s_wifiInit) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_now_init() != ESP_OK) {
      Serial.println("[ESPNOW-L] init failed");
      return false;
    }
    esp_now_register_recv_cb(globalRecvCb);
    esp_now_register_send_cb(globalSendCb);
    ensureQueue();
    s_wifiInit = true;
    Serial.printf("[ESPNOW-L] WiFi+ESP-NOW ready, master MAC: %s\n",
                  WiFi.macAddress().c_str());
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, peerMac_, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.printf("[ESPNOW-L] add peer anchor-%u failed\n", anchorId_);
    return false;
  }

  s_instances[anchorId_] = this;
  ready_ = true;

  Serial.printf("[ESPNOW-L] anchor-%u linked, MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                anchorId_, peerMac_[0], peerMac_[1], peerMac_[2],
                peerMac_[3], peerMac_[4], peerMac_[5]);
  return true;
}

void EspNowUciLink::setPacketCallback(PacketCallback cb) {
  callback_ = cb;
}

bool EspNowUciLink::isReady() const {
  return ready_;
}

void EspNowUciLink::poll() {
  if (!g_rxQueue) return;

  static uint32_t lastRxCount = 0;
  if (g_rxCount != lastRxCount) {
    Serial.printf("[ESPNOW-L] total rx messages: %lu\n",
                  static_cast<unsigned long>(g_rxCount));
    lastRxCount = g_rxCount;
  }

  EspNowMsg msg;
  while (xQueueReceive(g_rxQueue, &msg, 0) == pdTRUE) {
    if (msg.anchor_id != anchorId_) continue;
    feedBytes(msg.payload, msg.payload_len);
  }

  while (tryParseOnePacket()) {
    // parsed
  }
}

void EspNowUciLink::feedBytes(const uint8_t* data, size_t len) {
  rxBuffer_.insert(rxBuffer_.end(), data, data + len);
  if (rxBuffer_.size() > 1024) {
    rxBuffer_.erase(rxBuffer_.begin(),
                    rxBuffer_.begin() + rxBuffer_.size() - 512);
  }
}

bool EspNowUciLink::sendPacket(Mt mt, uint8_t gid, uint8_t oid,
                                const std::vector<uint8_t>& payload,
                                uint8_t pbf) {
  if (!ready_) return false;

  std::vector<uint8_t> frame;
  frame.reserve(4 + payload.size());

  frame.push_back((static_cast<uint8_t>(mt) << 5) | ((pbf & 0x01) << 4) | (gid & 0x0F));
  frame.push_back(oid);

  if (mt == Mt::Data) {
    const uint16_t len = static_cast<uint16_t>(payload.size());
    frame.push_back(static_cast<uint8_t>(len & 0xFF));
    frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
  } else {
    frame.push_back(0x00);
    frame.push_back(static_cast<uint8_t>(payload.size() & 0xFF));
  }

  frame.insert(frame.end(), payload.begin(), payload.end());

  if (frame.size() > sizeof(EspNowMsg::payload)) {
    Serial.printf("[ESPNOW-L] frame too large: %u bytes\n",
                  static_cast<unsigned>(frame.size()));
    return false;
  }

  EspNowMsg msg = {};
  msg.anchor_id = anchorId_;
  msg.payload_len = static_cast<uint16_t>(frame.size());
  memcpy(msg.payload, frame.data(), frame.size());

  esp_err_t err = esp_now_send(peerMac_,
                               reinterpret_cast<const uint8_t*>(&msg),
                               sizeof(EspNowMsg));
  if (err != ESP_OK) {
    Serial.printf("[ESPNOW-L] send anchor-%u failed: %d\n", anchorId_, err);
    return false;
  }
  return true;
}

bool EspNowUciLink::tryParseOnePacket() {
  if (rxBuffer_.size() < 4) return false;

  const uint8_t h0 = rxBuffer_[0];
  const uint8_t h1 = rxBuffer_[1];
  const uint8_t h2 = rxBuffer_[2];
  const uint8_t h3 = rxBuffer_[3];

  const uint8_t mtRaw = static_cast<uint8_t>((h0 & 0xE0) >> 5);
  if (mtRaw > static_cast<uint8_t>(Mt::Notification)) {
    rxBuffer_.erase(rxBuffer_.begin());
    return true;
  }

  const Mt mt = static_cast<Mt>(mtRaw);
  const uint16_t payloadLen = (mt == Mt::Data)
      ? static_cast<uint16_t>(h2 | (static_cast<uint16_t>(h3) << 8))
      : static_cast<uint16_t>(h3);
  const size_t fullLen = static_cast<size_t>(4 + payloadLen);
  if (rxBuffer_.size() < fullLen) return false;

  UciPacket packet;
  packet.mt = mt;
  packet.gid = static_cast<uint8_t>(h0 & 0x0F);
  packet.oid = h1;
  packet.pbf = static_cast<uint8_t>((h0 & 0x10) >> 4);
  packet.payload.assign(rxBuffer_.begin() + 4, rxBuffer_.begin() + fullLen);

  rxBuffer_.erase(rxBuffer_.begin(), rxBuffer_.begin() + fullLen);

  if (callback_) {
    callback_(packet);
  }
  return true;
}

}  // namespace UwbUci
