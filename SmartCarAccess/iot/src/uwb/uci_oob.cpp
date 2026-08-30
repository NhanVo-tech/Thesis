#include "uwb/uci_oob.h"

#include <string.h>

namespace UwbUci {

namespace {

uint16_t readLe16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

uint32_t readLe32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) |
         (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

}  // namespace

bool parseOobPayloadV1(const uint8_t* raw, size_t len, UciOobPayloadV1* out, const char** err) {
  if (err) *err = nullptr;
  if (!raw || !out) {
    if (err) *err = "null_input";
    return false;
  }
  if (len != UciOobPayloadV1::kSize) {
    if (err) *err = "bad_length";
    return false;
  }

  UciOobPayloadV1 p;
  size_t o = 0;
  p.version = raw[o++];
  p.role = raw[o++];
  p.sessionId = readLe32(raw + o); o += 4;
  p.phoneMac = readLe16(raw + o); o += 2;
  p.carMac = readLe16(raw + o); o += 2;
  p.channel = raw[o++];
  p.preambleIdx = raw[o++];
  p.sfdId = raw[o++];
  p.stsConfig = raw[o++];
  p.hoppingMode = raw[o++];
  p.rframeConfig = raw[o++];
  p.resultReportConfig = raw[o++];
  p.aoaResultReq = raw[o++];
  p.scheduleMode = raw[o++];
  p.multiNodeMode = raw[o++];
  p.rangingRoundUsage = raw[o++];
  p.rssiReporting = raw[o++];
  p.slotDuration = readLe16(raw + o); o += 2;
  p.rangingInterval = readLe32(raw + o); o += 4;
  p.slotsPerRr = raw[o++];
  p.vendorId = readLe16(raw + o); o += 2;
  memcpy(p.staticStsIv, raw + o, sizeof(p.staticStsIv));

  const char* vErr = nullptr;
  if (!validateOobPayloadV1(p, &vErr)) {
    if (err) *err = vErr;
    return false;
  }

  *out = p;
  return true;
}

bool validateOobPayloadV1(const UciOobPayloadV1& p, const char** err) {
  if (err) *err = nullptr;

  if (p.version != UciOobPayloadV1::kVersion) {
    if (err) *err = "bad_version";
    return false;
  }
  if (p.role > 1) {
    if (err) *err = "bad_role";
    return false;
  }
  if (p.sessionId == 0) {
    if (err) *err = "bad_session_id";
    return false;
  }
  if (p.phoneMac == 0 || p.carMac == 0 || p.phoneMac == p.carMac) {
    if (err) *err = "bad_mac";
    return false;
  }
  if (p.channel == 0 || p.channel > 15) {
    if (err) *err = "bad_channel";
    return false;
  }
  if (p.slotsPerRr == 0) {
    if (err) *err = "bad_slots_per_rr";
    return false;
  }
  if (p.slotDuration == 0 || p.rangingInterval == 0) {
    if (err) *err = "bad_timing";
    return false;
  }
  if (p.scheduleMode > 1) {
    if (err) *err = "bad_schedule_mode";
    return false;
  }
  if (p.multiNodeMode > 2) {
    if (err) *err = "bad_multi_node_mode";
    return false;
  }
  if (p.rangingRoundUsage == 0 || p.rangingRoundUsage > 8) {
    if (err) *err = "bad_round_usage";
    return false;
  }

  return true;
}

void mapOobToRunConfig(const UciOobPayloadV1& p, UciRunConfig* cfg) {
  if (!cfg) {
    return;
  }

  cfg->sessionId = p.sessionId;
  cfg->controlee = (p.role == 0);
  cfg->localMac = p.carMac;
  cfg->destMac = p.phoneMac;
  cfg->channel = p.channel;
  cfg->scheduleMode = p.scheduleMode;
  cfg->preambleIdx = p.preambleIdx;
  cfg->sfd = p.sfdId;
  cfg->slotDuration = p.slotDuration;
  cfg->rangingDuration = p.rangingInterval;
  cfg->slotsPerRr = p.slotsPerRr;
  cfg->hoppingMode = p.hoppingMode;
  cfg->stsConfig = p.stsConfig;
  cfg->aoaReport = p.aoaResultReq;
  cfg->vendorId = p.vendorId;
  memcpy(cfg->staticStsIv, p.staticStsIv, sizeof(cfg->staticStsIv));
  cfg->resultReportConfig = p.resultReportConfig;
  cfg->rframeConfig = p.rframeConfig;
}

}  // namespace UwbUci
