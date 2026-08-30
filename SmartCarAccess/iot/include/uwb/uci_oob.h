#pragma once

#include <stddef.h>
#include <stdint.h>

#include "uwb/uci_session_manager.h"

namespace UwbUci {

struct UciOobPayloadV1 {
  static constexpr uint8_t kVersion = 1;
  static constexpr size_t kSize = 37;

  uint8_t version = kVersion;
  uint8_t role = 1;  // 0=controlee, 1=controller
  uint32_t sessionId = 42;
  uint16_t phoneMac = 0x0001;
  uint16_t carMac = 0x0000;
  uint8_t channel = 9;
  uint8_t preambleIdx = 9;
  uint8_t sfdId = 2;
  uint8_t stsConfig = 0;
  uint8_t hoppingMode = 1;
  uint8_t rframeConfig = 3;
  uint8_t resultReportConfig = 0x0B;
  uint8_t aoaResultReq = 1;
  uint8_t scheduleMode = 1;
  uint8_t multiNodeMode = 0;
  uint8_t rangingRoundUsage = 2;
  uint8_t rssiReporting = 1;
  uint16_t slotDuration = 2400;
  uint32_t rangingInterval = 120;
  uint8_t slotsPerRr = 6;
  uint16_t vendorId = 0x0708;
  uint8_t staticStsIv[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
};

bool parseOobPayloadV1(const uint8_t* raw, size_t len, UciOobPayloadV1* out, const char** err);
bool validateOobPayloadV1(const UciOobPayloadV1& p, const char** err);
void mapOobToRunConfig(const UciOobPayloadV1& p, UciRunConfig* cfg);

}  // namespace UwbUci
