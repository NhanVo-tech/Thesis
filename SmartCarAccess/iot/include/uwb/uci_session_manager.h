#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <vector>

#include "uwb/uci_uart_link.h"
#include "Kalman.h"
#include "uwb/lstm_inference.h"

namespace UwbUci {

struct UciRunConfig {
  uint32_t sessionId = 42;
  bool controlee = false;
  uint16_t localMac = 0x0000;
  uint16_t destMac = 0x0001;
  uint8_t channel = 9;
  uint8_t scheduleMode = 1;
  uint8_t preambleIdx = 9;
  uint8_t sfd = 2;
  uint16_t slotDuration = 2400;
  uint32_t rangingDuration = 120;
  uint8_t slotsPerRr = 6;
  uint8_t hoppingMode = 1;
  uint8_t stsConfig = 0;
  uint8_t aoaReport = 1;
  uint16_t vendorId = 0x0708;
  uint8_t staticStsIv[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  uint8_t resultReportConfig = 0x0B;
  uint8_t rframeConfig = 0x03;
};

class UciSessionManager {
 public:
  explicit UciSessionManager(IUciLink& link);

  void poll();
  bool runOnce(const UciRunConfig& cfg);
  bool stopActiveSession();
  bool isSessionActive() const;

  uint32_t rangingNotificationCount() const;

 private:
  void onPacket(const UciPacket& packet);

  bool sendCommandWithRetry(
      uint8_t gid,
      uint8_t oid,
      const std::vector<uint8_t>& payload,
      uint32_t timeoutMs,
      uint8_t retries,
      std::vector<uint8_t>* outPayload,
      uint8_t* outStatus);

  bool commandSessionInit(const UciRunConfig& cfg);
  bool commandSessionSetAppConfig(const UciRunConfig& cfg);
  bool commandRangingStart(const UciRunConfig& cfg);
  bool commandRangingStop(const UciRunConfig& cfg);
  bool commandSessionDeinit(const UciRunConfig& cfg);

  uint32_t effectiveSessionId() const;

  bool waitForAtLeastOneRangingNotification(uint32_t timeoutMs);

  static void appendLe(std::vector<uint8_t>& dst, uint64_t v, uint8_t len);
  static void appendTlv(std::vector<uint8_t>& dst, uint8_t tag, const uint8_t* value, uint8_t len);

  IUciLink& link_;

  bool waitingResponse_;
  uint8_t expectedGid_;
  uint8_t expectedOid_;
  bool responseReady_;
  uint8_t responseStatus_;
  std::vector<uint8_t> responsePayload_;

  uint32_t activeSessionId_;
  bool activeSessionIdValid_;
  bool sessionActive_;
  UciRunConfig activeCfg_;

  uint32_t rangingNotifCount_;
  // Kalman filter for UWB distance smoothing.
  Kalman uwbFilter_{0.05, 0.2, 1.0, 0.0};
  bool first_reading_ = true;
  // LSTM inference engine for relay attack detection
  LstmInference lstm_ai_;
};

}  // namespace UwbUci
