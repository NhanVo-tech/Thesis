#pragma once
#include <Arduino.h>

namespace UciUart {

bool begin(int rxPin, int txPin, uint32_t baud);
void drain();

// MT=Command, 4-byte header, returns false if UART not ready
bool sendCommand(uint8_t gid, uint8_t oid, const uint8_t* payload, size_t len);

// Polls UART for 1 complete UCI Response. Returns status byte (0x00=OK).
// Blocks up to timeoutMs. Stores payload in outPayload if non-null.
// Returns 0xFF on timeout or transport error.
uint8_t waitResponse(uint32_t timeoutMs, uint8_t* outPayload = nullptr, size_t* outLen = nullptr);

// Non-blocking: process all available UART bytes. For each complete
// RANGE_DATA_NTF (mt=3, gid=0x02, oid=0x00), calls callback(seq, dist_cm, status, nlos, rssi).
using RangingCallback = void (*)(uint32_t seq, uint16_t distCm, uint8_t status,
                                  uint8_t nlos, int8_t rssiDbm);
void poll(RangingCallback cb);

}  // namespace UciUart
