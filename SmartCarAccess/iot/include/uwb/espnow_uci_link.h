#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <functional>
#include <vector>

#include "uwb/uci_uart_link.h"

namespace UwbUci {

#pragma pack(push, 1)
struct EspNowMsg {
  uint8_t anchor_id;
  uint16_t payload_len;
  uint8_t payload[247];
};
#pragma pack(pop)

class EspNowUciLink : public IUciLink {
 public:
  EspNowUciLink();

  bool begin(uint8_t anchorId, const uint8_t* mac);
  void poll() override;

  bool sendPacket(Mt mt, uint8_t gid, uint8_t oid,
                  const std::vector<uint8_t>& payload,
                  uint8_t pbf = 0) override;

  void setPacketCallback(PacketCallback cb) override;
  bool isReady() const override;

 private:
  bool tryParseOnePacket();
  void feedBytes(const uint8_t* data, size_t len);

  uint8_t anchorId_;
  uint8_t peerMac_[6];
  PacketCallback callback_;
  std::vector<uint8_t> rxBuffer_;
  bool ready_;

  static void globalRecvCb(const uint8_t* mac, const uint8_t* data, int len);
  static void globalSendCb(const uint8_t* mac, esp_now_send_status_t status);
  static bool s_wifiInit;
  static EspNowUciLink* s_instances[3];
};

}  // namespace UwbUci
