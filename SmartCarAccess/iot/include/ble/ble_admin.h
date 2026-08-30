#pragma once

#include <NimBLEDevice.h>
#include "ble.h" // For BLEMod::AdminMode

namespace BLEAdmin {
  // Register Admin service (mode control, info, commands, phone key upload)
  void registerService(NimBLEServer* server);

  // Admin mode state (backed by characteristic)
  BLEMod::AdminMode getAdminMode();
  void setAdminMode(BLEMod::AdminMode mode);

  // Push a short info message and notify (safe to call from app code)
  void notify(const char* msg);
}
