#pragma once

#include <NimBLEDevice.h>

// Forward declaration for RNG context
struct mbedtls_ctr_drbg_context;

namespace BLEEcho {
  // Register the Secure Echo (AES-GCM) service.
  // Requires DRBG for nonce generation. Uses BLEAuth's session key.
  void registerService(NimBLEServer* server, mbedtls_ctr_drbg_context* drbg);
}
