#pragma once

#include <stddef.h>
#include <stdint.h>
#include "uwb/uci_oob.h"

namespace UwbUciHost {

bool begin(const uint8_t auxMacs[][6], uint8_t count);
void tick();

bool submitBleOob(const uint8_t* payload, size_t len, const char** err);
bool requestStart(const char** err);
bool requestStop(const char** err);
bool hasCachedConfig();
bool isBusy();
bool hasPending();

// Direct config for debug
bool submitConfig(const UwbUci::UciRunConfig& cfg, const char** err);

}  // namespace UwbUciHost
