#include "uci_uart.h"
#include <vector>

namespace UciUart {

namespace {
HardwareSerial* g_uart = nullptr;
std::vector<uint8_t> g_buf;
}  // namespace

bool begin(int rxPin, int txPin, uint32_t baud) {
  g_uart = &Serial1;
  g_uart->begin(baud, SERIAL_8N1, rxPin, txPin);
  g_buf.reserve(1024);
  Serial.printf("[UART] anchor on RX=GPIO%d TX=GPIO%d baud=%lu\n",
                rxPin, txPin, static_cast<unsigned long>(baud));
  return true;
}

void drain() {
  while (g_uart->available()) g_uart->read();
  g_buf.clear();
  delay(150);
  while (g_uart->available()) g_uart->read();
  g_buf.clear();
}

bool sendCommand(uint8_t gid, uint8_t oid, const uint8_t* payload, size_t len) {
  if (!g_uart) return false;
  if (len > 255) return false;

  uint8_t frame[259];  // 4 header + max 255 payload
  frame[0] = static_cast<uint8_t>((1 << 5) | (gid & 0x0F));  // MT=Command, PBF=0
  frame[1] = oid;
  frame[2] = 0x00;  // RFU
  frame[3] = static_cast<uint8_t>(len);
  if (len > 0) memcpy(frame + 4, payload, len);

  Serial.printf("[UART→] gid=0x%02X oid=0x%02X len=%u:", gid, oid, static_cast<unsigned>(len));
  for (size_t i = 0; i < len && i < 8; ++i) Serial.printf(" %02X", payload[i]);
  if (len > 8) Serial.print("...");
  Serial.println();

  g_uart->write(frame, static_cast<size_t>(4 + len));
  g_uart->flush();
  return true;
}

uint8_t waitResponse(uint32_t timeoutMs, uint8_t* outPayload, size_t* outLen) {
  if (!g_uart) return 0xFF;

  uint32_t t0 = millis();
  while ((millis() - t0) < timeoutMs) {
    while (g_uart->available() > 0) {
      int b = g_uart->read();
      if (b >= 0) g_buf.push_back(static_cast<uint8_t>(b));
    }

    while (g_buf.size() >= 4) {
      const uint8_t mt_raw = (g_buf[0] >> 5) & 0x07;
      const uint8_t mt = mt_raw;  // guard: only respond to mt=2

      if (mt_raw > 3) { g_buf.erase(g_buf.begin()); continue; }

      const uint8_t gid = g_buf[0] & 0x0F;
      uint16_t plen = g_buf[3];
      if (mt_raw == 0)  // Data uses 2-byte length
        plen = static_cast<uint16_t>(g_buf[2]) | (static_cast<uint16_t>(g_buf[3]) << 8);

      if (plen > 512) { g_buf.erase(g_buf.begin()); continue; }
      const size_t full = 4 + plen;
      if (g_buf.size() < full) break;

      if (mt == 2) {  // Response
        uint8_t status = (plen >= 1) ? g_buf[4] : 0xFF;
        if (outPayload) memcpy(outPayload, g_buf.data() + 4, plen);
        if (outLen) *outLen = plen;
        g_buf.erase(g_buf.begin(), g_buf.begin() + full);
        Serial.printf("[UART←] RESP status=0x%02X gid=0x%02X len=%u\n",
                      status, gid, static_cast<unsigned>(plen));
        return status;
      }

      // Non-response: consume and discard (e.g., notifications during cmd/response flow)
      g_buf.erase(g_buf.begin(), g_buf.begin() + full);
    }

    delay(2);
  }

  Serial.printf("[UART←] timeout after %lu ms\n", static_cast<unsigned long>(timeoutMs));
  return 0xFF;
}

void poll(RangingCallback cb) {
  if (!g_uart || !cb) return;

  while (g_uart->available() > 0) {
    int b = g_uart->read();
    if (b >= 0) g_buf.push_back(static_cast<uint8_t>(b));
  }

  while (g_buf.size() >= 4) {
    const uint8_t mt_raw = (g_buf[0] >> 5) & 0x07;
    if (mt_raw > 3) { g_buf.erase(g_buf.begin()); continue; }

    const uint8_t mt = mt_raw;
    const uint8_t gid = g_buf[0] & 0x0F;
    const uint8_t oid = g_buf[1] & 0x3F;
    uint16_t plen = g_buf[3];
    if (mt_raw == 0) plen = static_cast<uint16_t>(g_buf[2]) | (static_cast<uint16_t>(g_buf[3]) << 8);
    if (plen > 512) { g_buf.erase(g_buf.begin()); continue; }
    const size_t full = 4 + plen;
    if (g_buf.size() < full) break;

    // RANGE_DATA_NTF: mt=3, gid=0x02, oid=0x00
    if (mt == 3 && gid == 0x02 && oid == 0x00 && plen >= 31) {
      const uint8_t* p = g_buf.data() + 4;
      if (p[24] >= 1 && p[24] <= 8) {
        uint32_t seq = static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8)
                     | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
        uint8_t st = p[27];
        // Only forward valid measurements (status 0x00 = OK, 0x1B = near-field)
        if (st != 0x00 && st != 0x1B) {
          // Log skipped measurements for debugging
          static uint8_t lastSkippedStatus = 0xFF;
          if (st != lastSkippedStatus) {
            Serial.printf("[UART] skipping RANGE_DATA_NTF status=0x%02X (seq=%u)\n", st, static_cast<unsigned>(seq));
            lastSkippedStatus = st;
          }
        } else {
          uint8_t nl = (plen >= 29) ? p[28] : 0;
          uint16_t dist = static_cast<uint16_t>(p[29]) | (static_cast<uint16_t>(p[30]) << 8);
          int8_t rssi = (plen >= 45) ? -static_cast<int8_t>(p[44]) : 0;
          cb(seq, dist, st, nl, rssi);
        }
      }
    }

    g_buf.erase(g_buf.begin(), g_buf.begin() + full);
  }
}

}  // namespace UciUart
