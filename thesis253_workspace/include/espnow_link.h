#pragma once
#include <stdint.h>
#include "uci_session.h"

namespace EspNowLink {

bool begin(const uint8_t* masterMac);

bool hasPendingStart();
UciSession::Config getPendingConfig();

void sendRanging(uint32_t seq, uint16_t distCm, uint8_t status,
                 uint8_t nlos, int8_t rssiDbm);

}  // namespace EspNowLink
