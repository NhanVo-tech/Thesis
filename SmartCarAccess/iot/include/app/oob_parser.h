#pragma once
#include <stddef.h>
#include <stdint.h>
#include "app/session_config.h"

// ── OOB Parser (BLE 37-byte payload) ────────────────────────────────
// Extracts App::SessionConfig from raw OOB bytes.
// Zero UCI dependency — decodes the wire format directly.

namespace OobParser {

bool parse(const uint8_t* data, size_t len, App::SessionConfig* out, const char** err);

}  // namespace OobParser
