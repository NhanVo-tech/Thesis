#pragma once
#include <stddef.h>
#include <stdint.h>
#include "app/session_config.h"

// ── Anchor Bridge (application layer) ───────────────────────────────
// Receives OOB from BLE, broadcasts SessionConfig to aux nodes via ESP-NOW,
// collects RangingReports from aux nodes, feeds distances to door unlock.
//
// ZERO UCI knowledge — no GID/OID, no TLV, no UciSessionManager.

namespace AnchorBridge {

bool begin(const uint8_t auxMacs[][6], uint8_t count);
void tick();

bool submitBleOob(const uint8_t* payload, size_t len, const char** err);
bool submitConfig(const App::SessionConfig& cfg, const char** err);
bool requestStart(const char** err);
bool requestStop(const char** err);
bool hasCachedConfig();
bool isBusy();

}  // namespace AnchorBridge
