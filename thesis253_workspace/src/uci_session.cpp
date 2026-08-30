#include "uci_session.h"
#include "uci_uart.h"
#include <Arduino.h>
#include <vector>

namespace UciSession {

namespace {

constexpr uint8_t kGidSession   = 0x01;
constexpr uint8_t kGidRanging   = 0x02;
constexpr uint8_t kOidInit      = 0x00;
constexpr uint8_t kOidSetCfg    = 0x03;
constexpr uint8_t kOidStart     = 0x00;
constexpr uint32_t kInitTimeoutMs  = 1500;
constexpr uint32_t kCfgTimeoutMs   = 2500;
constexpr uint32_t kStartTimeoutMs = 1500;

uint32_t g_sessionHandle = 0;

  // ── App Config TLV builder ──────────────────────────────────────
  // Must match exactly the 21 params from ESP32-S3 UciSessionManager.
  // Order matters: Qorvo SDK is known to be order-sensitive.
  std::vector<uint8_t> buildAppConfig(const Config& c) {
    std::vector<uint8_t> p;

    auto u8  = [&](uint8_t v) { p.push_back(v); };
    auto le16 = [&](uint16_t v) { u8(v & 0xFF); u8((v >> 8) & 0xFF); };
    auto le32 = [&](uint32_t v) { le16(v & 0xFFFF); le16((v >> 16) & 0xFFFF); };
    auto tlv1 = [&](uint8_t tag, uint8_t val)  { u8(tag); u8(1); u8(val); };
    auto tlv2 = [&](uint8_t tag, uint16_t val) { u8(tag); u8(2); le16(val); };
    auto tlv4 = [&](uint8_t tag, uint32_t val) { u8(tag); u8(4); le32(val); };
    auto tlvRaw = [&](uint8_t tag, const uint8_t* v, uint8_t n) {
      u8(tag); u8(n); for (uint8_t i = 0; i < n; ++i) u8(v[i]);
    };

    uint8_t devType = c.controlee ? 1 : 0;

    // session_handle (4 bytes LE)
    le32(g_sessionHandle);

    // num TLV entries — exactly 21, matching master's UciSessionManager
    u8(21);

    // 1.  0x00 DEVICE_TYPE  (0 = controlee/responder, 1 = controller)
    // 2.  0x11 DEVICE_ROLE  (0 = controlee/responder, 1 = initiator)
    // 3.  0x03 MULTI_NODE_MODE      = unicast (0)
    // 4.  0x01 RANGING_ROUND_USAGE  = DS-TWR deferred (2)
    // 5.  0x04 CHANNEL_NUMBER
    // 6.  0x22 SCHEDULE_MODE
    // 7.  0x06 DEVICE_MAC_ADDRESS   (2 bytes LE)
    // 8.  0x07 DST_MAC_ADDRESS      (2 bytes LE)
    // 9.  0x08 SLOT_DURATION        (2 bytes LE)
    // 10. 0x09 RANGING_INTERVAL     (4 bytes LE)
    // 11. 0x12 RFRAME_CONFIG        = SP3 (3)
    // 12. 0x13 RSSI_REPORTING       = 1
    // 13. 0x14 PREAMBLE_CODE_INDEX
    // 14. 0x15 SFD_ID
    // 15. 0x1B SLOTS_PER_RR
    // 16. 0x2C HOPPING_MODE
    // 17. 0x02 STS_CONFIG
    // 18. 0x0D AOA_RESULT_REQ
    // 19. 0x2E RESULT_REPORT_CONFIG
    // 20. 0x27 VENDOR_ID            (2 bytes LE)
    // 21. 0x28 STATIC_STS_IV        (6 raw bytes)

    tlv1(0x00, devType);
    tlv1(0x11, devType);
    tlv1(0x03, 0);
    tlv1(0x01, 2);
    tlv1(0x04, c.channel);
    tlv1(0x22, c.schedule_mode);
    tlv2(0x06, c.car_mac);
    tlv2(0x07, c.phone_mac);
    tlv2(0x08, c.slot_duration);
    tlv4(0x09, c.ranging_interval);
    tlv1(0x12, c.rframe_config);
    tlv1(0x13, 1);
    tlv1(0x14, c.preamble_idx);
    tlv1(0x15, c.sfd_id);
    tlv1(0x1B, c.slots_per_rr);
    tlv1(0x2C, c.hopping_mode);
    tlv1(0x02, c.sts_config);
    tlv1(0x0D, c.aoa_result_req);
    tlv1(0x2E, c.result_report_config);
    tlv2(0x27, c.vendor_id);
    tlvRaw(0x28, c.static_sts_iv, 6);

    return p;
  }

}  // namespace

bool run(const Config& cfg) {
  Serial.printf("[UCI] ==== Session start (sid=%lu dest=0x%04X) ====\n",
                static_cast<unsigned long>(cfg.session_id), cfg.phone_mac);

  // Drain stale UART data before starting
  UciUart::drain();

  // ── 1) SESSION_INIT (GID=0x01 OID=0x00) ───────────────────────
  {
    uint8_t payload[5];
    payload[0] = static_cast<uint8_t>(cfg.session_id & 0xFF);
    payload[1] = static_cast<uint8_t>((cfg.session_id >> 8) & 0xFF);
    payload[2] = static_cast<uint8_t>((cfg.session_id >> 16) & 0xFF);
    payload[3] = static_cast<uint8_t>((cfg.session_id >> 24) & 0xFF);
    payload[4] = 0x00;  // FiRa ranging session type

    bool ok = false;
    for (uint8_t attempt = 0; attempt < 3; ++attempt) {
      UciUart::sendCommand(kGidSession, kOidInit, payload, sizeof(payload));
      uint8_t rsp[32]; size_t rspLen = 0;
      uint8_t status = UciUart::waitResponse(kInitTimeoutMs, rsp, &rspLen);
      if (status == 0x00) {
        // Extract session_handle from response[1..4]
        if (rspLen >= 5) {
          g_sessionHandle = static_cast<uint32_t>(rsp[1])
                          | (static_cast<uint32_t>(rsp[2]) << 8)
                          | (static_cast<uint32_t>(rsp[3]) << 16)
                          | (static_cast<uint32_t>(rsp[4]) << 24);
        } else {
          g_sessionHandle = cfg.session_id;  // fallback
        }
        Serial.printf("[UCI] SESSION_INIT OK (handle=0x%08lX)\n",
                      static_cast<unsigned long>(g_sessionHandle));
        ok = true;
        break;
      }
      Serial.printf("[UCI] SESSION_INIT attempt %u: status=0x%02X\n",
                    attempt + 1, status);
      UciUart::drain();
    }
    if (!ok) {
      Serial.println("[UCI] SESSION_INIT FAILED");
      return false;
    }
  }

  // ── 2) SET_APP_CONFIG (GID=0x01 OID=0x03) ─────────────────────
  {
    std::vector<uint8_t> payload = buildAppConfig(cfg);

    bool ok = false;
    for (uint8_t attempt = 0; attempt < 3; ++attempt) {
      UciUart::sendCommand(kGidSession, kOidSetCfg, payload.data(), payload.size());
      uint8_t status = UciUart::waitResponse(kCfgTimeoutMs);
      if (status == 0x00) {
        Serial.println("[UCI] SET_APP_CONFIG OK");
        ok = true;
        break;
      }
      Serial.printf("[UCI] SET_APP_CONFIG attempt %u: status=0x%02X\n",
                    attempt + 1, status);
      UciUart::drain();
    }
    if (!ok) {
      Serial.println("[UCI] SET_APP_CONFIG FAILED");
      return false;
    }
  }

  // ── 3) RANGING_START (GID=0x02 OID=0x00) ──────────────────────
  {
    uint8_t payload[4];
    payload[0] = static_cast<uint8_t>(g_sessionHandle & 0xFF);
    payload[1] = static_cast<uint8_t>((g_sessionHandle >> 8) & 0xFF);
    payload[2] = static_cast<uint8_t>((g_sessionHandle >> 16) & 0xFF);
    payload[3] = static_cast<uint8_t>((g_sessionHandle >> 24) & 0xFF);

    bool ok = false;
    for (uint8_t attempt = 0; attempt < 3; ++attempt) {
      UciUart::sendCommand(kGidRanging, kOidStart, payload, sizeof(payload));
      uint8_t status = UciUart::waitResponse(kStartTimeoutMs);
      if (status == 0x00) {
        Serial.println("[UCI] RANGING_START OK — waiting for ranging data...");
        ok = true;
        break;
      }
      Serial.printf("[UCI] RANGING_START attempt %u: status=0x%02X\n",
                    attempt + 1, status);
      UciUart::drain();
    }
    if (!ok) {
      Serial.println("[UCI] RANGING_START FAILED");
      return false;
    }
  }

  return true;
}

}  // namespace UciSession
