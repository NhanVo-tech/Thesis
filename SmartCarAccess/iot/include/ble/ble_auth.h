#pragma once

#include <NimBLEDevice.h>

// Forward declaration for RNG context
struct mbedtls_ctr_drbg_context;

namespace BLEAuth {
  // Register the AUTH service and start the crypto worker.
  // Uses the provided DRBG for key generation and nonces.
  void registerService(NimBLEServer* server, mbedtls_ctr_drbg_context* drbg);

  // Session status and key accessors
  bool isSessionReady();
  const uint8_t* sessionEncKey();
  size_t sessionEncKeyLen();
  const uint8_t* sessionMacKey();
  size_t sessionMacKeyLen();

  // Reset the current session state (keys, buffers) on disconnect.
  void resetSession();
  
  // GPS location data callback
  // Will be called when phone sends encrypted GPS data
  typedef void (*GpsDataCallback)(const uint8_t* data, size_t len, bool verified);
  void setGpsDataCallback(GpsDataCallback callback);
  
  // Debug and statistics
  void printStats();
}
