#pragma once
#include <stdint.h>

namespace UciSession {

struct Config {
  uint32_t session_id   = 42;
  bool     controlee     = true;   // car is controlee/responder
  uint16_t phone_mac    = 0x0001;
  uint16_t car_mac      = 0x0000;
  uint8_t  channel      = 9;
  uint8_t  preamble_idx = 9;
  uint8_t  sfd_id       = 2;
  uint8_t  sts_config   = 0;
  uint8_t  hopping_mode = 1;
  uint8_t  rframe_config = 3;
  uint8_t  result_report_config = 0x0B;
  uint8_t  aoa_result_req = 1;
  uint8_t  schedule_mode = 1;
  uint16_t slot_duration = 2400;
  uint32_t ranging_interval = 120;
  uint8_t  slots_per_rr  = 6;
  uint16_t vendor_id     = 0x0708;
  uint8_t  static_sts_iv[6] = {1, 2, 3, 4, 5, 6};
};

// Returns true if all 3 UCI commands succeeded and ranging is active.
// Blocking call: sends SESSION_INIT → SET_APP_CONFIG → RANGING_START
// with retries (2 retries each). Total max duration ~15s.
bool run(const Config& cfg);

}  // namespace UciSession
