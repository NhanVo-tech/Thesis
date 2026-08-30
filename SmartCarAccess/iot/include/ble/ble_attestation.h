#pragma once

#include <NimBLEDevice.h>

namespace BLEAttestation {
  // Register the Digital Key attestation service (Auth_RX/Auth_TX).
  void registerService(NimBLEServer* server);
}
