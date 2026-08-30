#include "app/oob_parser.h"
#include <cstring>

namespace OobParser {

namespace {

uint16_t readLe16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}
uint32_t readLe32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8)
       | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

}  // namespace

bool parse(const uint8_t* data, size_t len, App::SessionConfig* out, const char** err) {
  if (err) *err = nullptr;

  // OOB payload V1: exactly 37 bytes
  if (len != 37) { if (err) *err = "bad_length"; return false; }
  if (data[0] != 1) { if (err) *err = "bad_version"; return false; }

  App::SessionConfig cfg;
  size_t o = 0;
  o++;  // skip version byte [0]
  cfg.controlee    = (data[o++] == 0);  // role byte [1]: 0=controlee, 1=controller
  cfg.session_id   = readLe32(data + o); o += 4;
  cfg.phone_mac    = readLe16(data + o); o += 2;
  cfg.car_mac      = readLe16(data + o); o += 2;
  cfg.channel      = data[o++];
  cfg.preamble_idx = data[o++];
  cfg.sfd_id       = data[o++];
  cfg.sts_config   = data[o++];
  cfg.hopping_mode = data[o++];
  cfg.rframe_config = data[o++];
  cfg.result_report_config = data[o++];
  cfg.aoa_result_req = data[o++];
  cfg.schedule_mode = data[o++];
  o++;  // skip multi_node_mode [19]
  o++;  // skip ranging_round_usage [20]
  o++;  // skip rssi_reporting [21]
  cfg.slot_duration = readLe16(data + o); o += 2;
  cfg.ranging_interval_ms = readLe32(data + o); o += 4;
  cfg.slots_per_rr = data[o++];
  cfg.vendor_id    = readLe16(data + o); o += 2;
  memcpy(cfg.static_sts_iv, data + o, 6);

  if (cfg.session_id == 0) { if (err) *err = "bad_session_id"; return false; }
  if (cfg.phone_mac == 0 || cfg.car_mac == 0) { if (err) *err = "bad_mac"; return false; }
  if (cfg.channel == 0 || cfg.channel > 15) { if (err) *err = "bad_channel"; return false; }

  *out = cfg;
  return true;
}

}  // namespace OobParser
