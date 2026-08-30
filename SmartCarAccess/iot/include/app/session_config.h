#pragma once
#include <stdint.h>

// ── Application-layer data model ────────────────────────────────────
// These structs are the boundary between Master (app) and Aux (UCI gateway).
// NO UCI knowledge here — no GID/OID, no TLV tags, no UCI frame format.

namespace App {

struct SessionConfig {
  uint32_t session_id    = 42;
  bool     controlee     = true;      // car is controlee/responder, phone is initiator
  uint16_t phone_mac     = 0x0001;   // phone short MAC
  uint16_t car_mac       = 0x0000;   // this car's short MAC
  uint8_t  channel       = 9;
  uint8_t  preamble_idx  = 9;
  uint8_t  sfd_id        = 2;
  uint8_t  sts_config    = 0;
  uint8_t  hopping_mode  = 1;
  uint8_t  rframe_config = 3;
  uint8_t  result_report_config = 0x0B;
  uint8_t  aoa_result_req = 1;
  uint8_t  schedule_mode = 1;
  uint16_t slot_duration = 2400;
  uint32_t ranging_interval_ms = 120;
  uint8_t  slots_per_rr  = 6;
  uint16_t vendor_id     = 0x0708;
  uint8_t  static_sts_iv[6] = {1, 2, 3, 4, 5, 6};
};

struct RangingReport {
  uint8_t  anchor_id;
  uint32_t seq_number;
  uint16_t distance_cm;     // raw, no antenna offset
  uint8_t  status;          // 0x00 = OK
  uint8_t  nlos;            // 0 = LoS, 1 = NLoS, 0xFF = unknown
  int8_t   rssi_neg_dbm;   // negate for dBm
};

}  // namespace App
