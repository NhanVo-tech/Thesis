#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <functional>

namespace EspNowBridge {

#pragma pack(push, 1)
struct RangingData {
  uint8_t msg_type;     // 0x10
  uint8_t anchor_id;    // 0-2
  uint32_t seq_number;
  uint16_t distance_cm;
  uint8_t status;
  uint8_t nlos;
  int8_t rssi_neg_dbm;
};

struct CmdToAux {
  uint8_t msg_type;     // 0x01 = UCI command
  uint8_t request_id;
  uint16_t payload_len;
  uint8_t payload[245];
};
#pragma pack(pop)

using RangingCallback = std::function<void(const RangingData&)>;

bool begin(const uint8_t auxMacs[][6], uint8_t auxCount);
bool sendUciToAux(uint8_t anchorId, const uint8_t* uciPayload, uint16_t len);
void setRangingCallback(RangingCallback cb);
const char* getAuxMacStr(uint8_t anchorId);

// Called from loop/task — processes pending incoming data
void poll();

}  // namespace EspNowBridge
