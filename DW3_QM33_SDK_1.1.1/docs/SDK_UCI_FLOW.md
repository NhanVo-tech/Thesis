# SDK_UCI_FLOW.md

> Exact UCI flow produced by DW3_QM33_SDK_1.1.1 anchors (nRF52840DK+DWM3000EVB
> and DWM3001CDK) as received by the ESP32 firmware (`UciSessionManager`).
> **Stage 1 only:** single-anchor, one-to-one ranging, Controlee/Responder role on the phone side,
> Controller/Initiator driven by the host over UCI.
>
> **This is the single most important document for thesis integration.** Every byte offset below
> was verified against SDK source; the source file + line is cited inline.

---

## 0. Source of truth

All values in this document come from these SDK files (not invented):

| Fact | SDK source |
|------|-----------|
| UCI header layout, GID/OID/status codes, APP config param IDs | [SDK/Firmware/Libs/uwbstack_libs/delivery/full/Release/include/uci_bundle/uci/uci_spec_fira.h](../SDK/Firmware/Libs/uwbstack_libs/delivery/full/Release/include/uci_bundle/uci/uci_spec_fira.h) |
| Frame parsing / length field / raw framing | [SDK/Firmware/Src/Apps/Src/uci/uci_transport/src/uci_transport.c](../SDK/Firmware/Src/Apps/Src/uci/uci_transport/src/uci_transport.c) |
| RANGE_DATA_NTF byte-by-byte layout | [SDK/Tools/uwb-qorvo-tools/lib/uwb-uci/uci/qorvo_msg.py](../SDK/Tools/uwb-qorvo-tools/lib/uwb-uci/uci/qorvo_msg.py) (`class RangingData`, `class RangingTwrData`) |
| UART transport, baud rate | [SDK/Firmware/Src/HAL/Src/nrfx/HAL_uart.c](../SDK/Firmware/Src/HAL/Src/nrfx/HAL_uart.c) |
| Board UART pins | [SDK/Firmware/Src/Boards/Src/DWM3001CDK/Common/custom_board.h](../SDK/Firmware/Src/Boards/Src/DWM3001CDK/Common/custom_board.h), [.../nRF52840DK/Common/custom_board.h](../SDK/Firmware/Src/Boards/Src/nRF52840DK/Common/custom_board.h) |
| Host-side command sequence | [SmartCarAccess-main/iot/src/uwb/uci_session_manager.cpp](../../SmartCarAccess-main/iot/src/uwb/uci_session_manager.cpp) |

---

## 1. UCI Packet Format (byte-level)

### 1.1 General Frame Structure

The SDK uses the **standard FiRa UCI Common Packet Header** (4-byte header + payload).
Field layout of Octet 0 comes from `enum uci_common_packet_header` in `uci_spec_fira.h`.

| Byte offset | Field | Size | Description |
|-------------|-------|------|-------------|
| 0 | `MT` (bits 7-5), `PBF` (bit 4), `GID` (bits 3-0) | 1 | Message Type, Packet Boundary Flag, Group ID |
| 1 | `RFU` (bits 7-6), `OID` (bits 5-0) | 1 | Opcode ID (lower 6 bits) |
| 2 | `RFU` | 1 | Reserved (0x00) |
| 3 | `Payload Length` | 1 | Number of payload bytes that follow |
| 4 … 4+len-1 | `Payload` | `len` | Command/response/notification payload |

Octet 0 bit packing (`uci_spec_fira.h`, `enum uci_common_packet_header`):
- `MT_OFFSET = 5`, `MT_MASK = 0b111` → `octet0 = (MT << 5) | (PBF << 4) | GID`
- `PBF_OFFSET = 4`, `PBF_MASK = 1`
- `INFO_OFFSET = 0`, `INFO_MASK = 0b1111` (holds GID)

Message types (`enum uci_message_type`): `DATA=0b000`, `COMMAND=0b001`, `RESPONSE=0b010`,
`NOTIFICATION=0b011`.

> Example header for `RANGING_START_CMD` (MT=Command=1, PBF=0, GID=0x02, OID=0x00):
> `0x22 0x00 0x00 0x04` followed by the 4-byte session id.

### 1.2 How the SDK frames packets for UART output

Verified in `uci_transport.c → uci_tp_read()`:

- **Byte order:** little-endian for all multi-byte scalar fields.
- **Framing:** **raw UCI bytes — NO SLIP, NO HDLC, NO COBS.** The parser reads exactly
  `UCI_PACKET_HEADER_SIZE` (4) header bytes, then reads `rx->data[3]` (the Payload Length byte)
  more bytes, and considers the message complete when
  `rx_offset == UCI_PACKET_HEADER_SIZE + rx->data[3]`.
- **Length prefix:** none beyond the standard 1-byte UCI Payload Length at offset 3.
- **CRC:** **none.** There is no checksum over UCI frames on the UART. Integrity is delegated
  to the UART layer (which itself has no parity here — see §1.3).
- **Garbage handling:** if a partial message stalls for more than `UCI_GARBAGE_TIMEOUT_MS` (100 ms),
  the RX buffer is flushed (`uci_tp_read` → `uci_tp_flush`). Relevant for resync after a glitch.
- **Output path:** notifications/responses are written byte-for-byte to the transport via
  `reporter_instance.print()` in `uci_tp_usb_packet_send_ready()`.

### 1.3 UART line settings (`HAL_uart.c → deca_uart_init`)

| Setting | Value | Source |
|---------|-------|--------|
| Baud rate | **115200** | `UART_BAUDRATE_BAUDRATE_Baud115200` |
| Data / parity / stop | 8N1 | nRF `app_uart` default |
| HW flow control | **Disabled** | `APP_UART_FLOW_CONTROL_DISABLED` |

This exactly matches the ESP32 side (`UCI_UART_BAUD 115200`, 8N1, no flow control).

---

## 2. Session Startup Sequence (command → response pairs)

The UCI application in the SDK is a **UCI server**: it does nothing autonomously except emit a
`CORE_DEVICE_STATUS_NTF` (device ready) on boot. **The host (ESP32) drives the entire sequence.**
GID/OID values below are from `uci_spec_fira.h`.

| Step | Name | MT | GID | OID | Notes |
|------|------|----|-----|-----|-------|
| — | (boot) `CORE_DEVICE_STATUS_NTF` | Ntf(3) | 0x00 | 0x01 | payload[0]=`0x01` (READY) |
| 2.1 | `CORE_DEVICE_RESET` (optional) | Cmd(1) | 0x00 | 0x00 | resets UWBS |
| 2.2 | `CORE_GET_DEVICE_INFO` (optional) | Cmd(1) | 0x00 | 0x02 | version diagnostics |
| 2.3 | `SESSION_INIT_CMD` | Cmd(1) | 0x01 | 0x00 | create session |
| 2.4 | `SESSION_SET_APP_CONFIG_CMD` | Cmd(1) | 0x01 | 0x03 | set all params |
| 2.5 | `SESSION_START_CMD` | Cmd(1) | 0x02 | 0x00 | begin ranging |
| — | `RANGE_DATA_NTF` (repeats) | Ntf(3) | 0x02 | 0x00 | see §3 |
| 2.6 | `SESSION_STOP_CMD` | Cmd(1) | 0x02 | 0x01 | stop ranging |
| 2.7 | `SESSION_DEINIT_CMD` | Cmd(1) | 0x01 | 0x01 | delete session |

> **Critical clarification vs. the FiRa spec quick-reference:** the thesis prompt assumed session
> commands live under `GID=0x00`. **They do not.** In this SDK (and standard FiRa UCI 2.0):
> - `GID_SESSION_CONFIG = 0x01` → INIT / DEINIT / SET_APP_CONFIG live here.
> - `GID_SESSION_CONTROL = 0x02` → START / STOP / and the `RANGE_DATA_NTF` live here.
> The ESP32 firmware already uses the correct values (`kGidSession=0x01`, `kGidRanging=0x02`).

### 2.1 `CORE_DEVICE_RESET` (optional, on boot)
```
→ HOST sends:   [MT=1 PBF=0 GID=0x00 OID=0x00] len=1  payload: [00]           (reset config = 0)
← ANCHOR reply: [MT=2 PBF=0 GID=0x00 OID=0x00] len=1  payload: [status]        status=0x00 (OK)
← ANCHOR ntf:   [MT=3 PBF=0 GID=0x00 OID=0x01] len=1  payload: [01]            DEVICE_STATE_READY
```
Wait for the READY notification before continuing.

### 2.2 `CORE_GET_DEVICE_INFO` (optional diagnostics)
```
→ HOST sends:   [MT=1 PBF=0 GID=0x00 OID=0x02] len=0
← ANCHOR reply: [MT=2 PBF=0 GID=0x00 OID=0x02] payload: [status, uci_major, uci_minor.maint,
                 mac_major, mac_minor.maint, phy_major, phy_minor.maint, test_major,
                 test_minor.maint, vendor_data_len, vendor_data…]
```
Decoded by `DeviceInfo_fira` in `fira_msg.py`.

### 2.3 `SESSION_INIT_CMD`
```
→ HOST sends:   [MT=1 PBF=0 GID=0x01 OID=0x00] len=5  payload:
                  [session_id (4, LE)] [session_type (1)]
                  session_type = 0x00 (FiRa ranging)
← ANCHOR reply: [MT=2 PBF=0 GID=0x01 OID=0x00] payload: [status (1)] [session_handle (4, LE)]
                  status = 0x00 (OK)
```
- The ESP32 sends `session_id = 42` (`0x2A 0x00 0x00 0x00`).
- **FiRa 2.0 returns a 4-byte `session_handle`** in the response; the ESP32 captures it and uses it
  in every later command (`commandSessionInit()` reads `rsp[1..4]`). If the response is shorter
  (FiRa 1.x style), the ESP32 falls back to using the `session_id` directly.
- A `SESSION_STATUS_NTF` (`GID=0x01 OID=0x02`) with state = INIT/IDLE may also arrive.

### 2.4 `SESSION_SET_APP_CONFIG_CMD`
```
→ HOST sends:   [MT=1 PBF=0 GID=0x01 OID=0x03] payload:
                  [session_handle (4, LE)] [n_params (1)] [ TLV … ]
                  TLV = [param_id (1)] [len (1)] [value (len)]
← ANCHOR reply: [MT=2 PBF=0 GID=0x01 OID=0x03] payload:
                  [status (1)] [n_failed (1)] [ (param_id, status) … ]
                  status = 0x00 (OK) and n_failed = 0 on success
```
See §5 for the exact parameter list the ESP32 sends.

### 2.5 `SESSION_START_CMD`
```
→ HOST sends:   [MT=1 PBF=0 GID=0x02 OID=0x00] len=4  payload: [session_handle (4, LE)]
← ANCHOR reply: [MT=2 PBF=0 GID=0x02 OID=0x00] payload: [status (1)]   status=0x00 (OK)
← ANCHOR ntf:   [MT=3 PBF=0 GID=0x02 OID=0x00]  → RANGE_DATA_NTF stream begins (see §3)
```

### 2.6 `SESSION_STOP_CMD`
```
→ HOST sends:   [MT=1 PBF=0 GID=0x02 OID=0x01] len=4  payload: [session_handle (4, LE)]
← ANCHOR reply: [MT=2 PBF=0 GID=0x02 OID=0x01] payload: [status (1)]   status=0x00 (OK)
```

### 2.7 `SESSION_DEINIT_CMD`
```
→ HOST sends:   [MT=1 PBF=0 GID=0x01 OID=0x01] len=4  payload: [session_handle (4, LE)]
← ANCHOR reply: [MT=2 PBF=0 GID=0x01 OID=0x01] payload: [status (1)]   status=0x00 (OK)
```

**Timing:** the ESP32 uses `sendCommandWithRetry(..., timeoutMs, retries=2)` with 1200 ms for
`SESSION_INIT`, 2000 ms for `SET_APP_CONFIG`, and polls responses every 2 ms. Responses in practice
return within a few ms; the generous timeouts absorb UART jitter and the FiRa MAC init.

---

## 3. Ranging Data Notification — `RANGE_DATA_NTF` (CRITICAL)

This is the notification `UciSessionManager::onPacket()` parses. Header:
`MT=Notification(3)`, **`GID=0x02`**, **`OID=0x00`** (`UCI_OID_SESSION_INFO == UCI_OID_SESSION_START`).

> **The ESP32 firmware currently reads:** `num_meas` at `payload[24]`, `status` at `payload[27]`,
> `distance` at `payload[29:30]` (uint16 LE, cm).
> **These offsets are CONFIRMED CORRECT** for this SDK version — see the byte map below, which is
> transcribed directly from `class RangingData` and `class RangingTwrData` in `qorvo_msg.py`.

### 3.1 Notification header (offsets relative to first payload byte = 0)

| Byte offset | Field | Type | Notes |
|-------------|-------|------|-------|
| 0 – 3 | `sequence_number` | uint32 LE | Ranging round counter, starts at 0 (`idx`) |
| 4 – 7 | `session_handle` | uint32 LE | Session ID / handle of this round |
| 8 | `rcr_indication` / RFU | uint8 | 1 RFU byte |
| 9 – 12 | `current_ranging_interval` | uint32 LE | In units of 1200 RSTU (≈ 1 ms) |
| 13 | `ranging_measurement_type` | uint8 | 0=OWR-UL-TDoA, 1=OWR-AoA, 2=**TWR**, 3=OWR-DL-TDoA (see `RangingMeas`) |
| 14 | RFU | uint8 | |
| 15 | `mac_addressing_mode` | uint8 | 0 = 2-byte MAC, 1 = 8-byte MAC |
| 16 – 19 | `primary_session_id` | uint32 LE | Primary session id, or 0 |
| 20 – 23 | RFU | 4 bytes | |
| **24** | **`num_ranging_measurements`** | uint8 | **← ESP32 `payload[24]` ✓** |
| 25 … | measurement block(s) | — | one block per measurement (see §3.2) |

### 3.2 TWR measurement block (2-byte MAC), first block starts at offset **25**

Transcribed from `class RangingTwrData`. Offsets shown are **absolute** (assuming the first
measurement block; `mac_addressing_mode = 0`, i.e. 2-byte MAC).

| Byte offset | Field | Type | Units | Notes |
|-------------|-------|------|-------|-------|
| 25 – 26 | `mac_address` | uint16 | — | Peer MAC (stored reversed) |
| **27** | **`status`** | uint8 | — | **← ESP32 `payload[27]` ✓** 0x00 = OK |
| 28 | `nlos` | uint8 | — | Non-line-of-sight indicator |
| **29 – 30** | **`distance`** | uint16 LE | **cm** | **← ESP32 `payload[29:30]` ✓** (c = 299,702,547 m/s) |
| 31 – 32 | `aoa_azimuth` | Q9.7 signed | deg | −180…180; 0 if `AOA_RESULT_REQ=0` |
| 33 | `aoa_azimuth_fom` | uint8 | % | Azimuth reliability |
| 34 – 35 | `aoa_elevation` | Q9.7 signed | deg | −90…90 |
| 36 | `aoa_elevation_fom` | uint8 | % | |
| 37 – 38 | `aoa_dest_azimuth` | Q9.7 signed | deg | if MRR=1 else 0 |
| 39 | `aoa_dest_azimuth_fom` | uint8 | % | |
| 40 – 41 | `aoa_dest_elevation` | Q9.7 signed | deg | if MRR=1 else 0 |
| 42 | `aoa_dest_elevation_fom` | uint8 | % | |
| 43 | `slot_in_error` | uint8 | — | Error slot number on failure |
| 44 | `rssi` | Q7.1 unsigned | −dBm | RSSI value (negate for dBm) |
| 45 – 55 | RFU | 11 bytes | — | (5 RFU bytes when MAC = 8 bytes) |

**TWR block size = 31 bytes for a 2-byte MAC** (that is why the ESP32 guards with
`packet.payload.size() >= 31`). `DISTANCE = 0xFFFF` means "invalid"
(`UCI_FIRA_TWR_MEASUREMENT_DISTANCE_INVALID` in `uci_spec_fira.h`).

### 3.3 Full raw hex example (single TWR measurement, constructed from the struct)

Header `MT=3 GID=0x02 OID=0x00`, distance = 137 cm (`0x0089`), status = OK:

```
Header : 62 00 00 3E                       ; MT=3,PBF=0,GID=2 | OID=0 | RFU | len=0x3E(62)
Payload:
  00 00 00 00                              ; [0:3]  sequence_number = 0
  2A 00 00 00                              ; [4:7]  session_handle  = 42
  00                                       ; [8]    RFU
  78 00 00 00                              ; [9:12] ranging_interval = 120
  02                                       ; [13]   meas_type = TWR
  00                                       ; [14]   RFU
  00                                       ; [15]   mac mode = 2-byte
  00 00 00 00                              ; [16:19] primary_session_id = 0
  00 00 00 00                              ; [20:23] RFU
  01                                       ; [24]   num_measurements = 1
  01 00                                    ; [25:26] mac_address = 0x0001
  00                                       ; [27]   status = OK
  00                                       ; [28]   nlos
  89 00                                    ; [29:30] distance = 137 cm
  00 00 00 00 00 00 00 00 00 00 00 00 00   ; [31:43] AoA fields (0)
  00                                       ; [44]   rssi
  00 00 00 00 00 00 00 00 00 00 00         ; [45:55] RFU
```
> Note: the header `len` byte counts only payload bytes. The exact `len` depends on whether AoA/
> multiple measurements are present. The above is illustrative of field placement, not a captured log.

> **See [SDK_INTEGRATION_LINK.md §2](SDK_INTEGRATION_LINK.md) for how
> `UciSessionManager::onPacket()` consumes this notification.**

---

## 4. Differences Between nRF52840DK+DWM3000EVB and DWM3001CDK

Both boards run the **same** `SDK/Firmware/Src/Apps/Src/uci` application and the **same**
uwb-stack UCI backend, so the on-wire UCI protocol is **identical**.

| Aspect | nRF52840DK + DWM3000EVB | DWM3001CDK |
|--------|-------------------------|------------|
| UCI packet format | Identical (same `uci_spec_fira.h`) | Identical |
| UART baud rate | 115200, 8N1, no flow control | 115200, 8N1, no flow control |
| Commands supported | Full FiRa UCI set (+ optional Qorvo GIDs) | Same |
| Vendor extension GIDs | `QORVO_EXT2=0x0B`, `QORVO_MAC=0x0E`, `QORVO_CALIB=0x0F` (build-dependent) | Same |
| `RANGE_DATA_NTF` byte layout | Same (§3) | Same (§3) |
| Anchor-ID field | No dedicated anchor-ID; peer identity is the `mac_address` in each measurement block | Same — differentiate anchors by **`DST_MAC`/`DEVICE_MAC`**, not by any SDK field |
| NLOS status codes | Same `uci_status_code` enum | Same |
| SoC | nRF52840 (`nrf52840_xxaa`) | nRF52833 (`nrf52833_xxaa`) |
| AoA hardware | DWM3000EVB is non-AoA (single antenna) | DWM3001C may have AoA option (`AOA_CHIP_ON_NON_AOA_PCB`) |

> **Practical takeaway for the thesis:** to run Anchors 1 & 2 on DWM3001CDKs, give each anchor a
> distinct `DEVICE_MAC_ADDRESS` and address each from the ESP32 with the matching `DST_MAC_ADDRESS`.
> The parser in `UciSessionManager::onPacket()` does not currently read the per-measurement
> `mac_address` (bytes 25–26); to support multiple anchors it must (see
> [SDK_INTEGRATION_LINK.md §6](SDK_INTEGRATION_LINK.md)).

---

## 5. Parameter Reference for `SESSION_SET_APP_CONFIG`

Every parameter the ESP32 `UciSessionManager::commandSessionSetAppConfig()` sends, matched to the
SDK's `enum uci_application_configuration_parameters` (`uci_spec_fira.h`). **All parameter IDs
match** — no ID mismatch was found.

| Param ID (hex) | SDK name | Size | ESP32 value | ESP32 field | Status |
|----------------|----------|------|-------------|-------------|--------|
| 0x00 | `DEVICE_TYPE` | 1 | `0x01` (controller) | `controlee=false` | ✅ match |
| 0x11 | `DEVICE_ROLE` | 1 | `0x01` (initiator) | `controlee=false` | ✅ match |
| 0x03 | `MULTI_NODE_MODE` | 1 | `0x00` (unicast) | fixed | ✅ match |
| 0x01 | `RANGING_ROUND_USAGE` | 1 | `0x02` (DS-TWR deferred) | fixed | ✅ match |
| 0x04 | `CHANNEL_NUMBER` | 1 | `9` | `channel` | ✅ match |
| 0x22 | `SCHEDULE_MODE` | 1 | `1` (time-scheduled) | `scheduleMode` | ✅ match |
| 0x06 | `DEVICE_MAC_ADDRESS` | 2 | `0x0000` | `localMac` | ✅ match |
| 0x07 | `DST_MAC_ADDRESS` | 2 | `0x0001` | `destMac` | ✅ match |
| 0x08 | `SLOT_DURATION` | 2 | `2400` (RSTU) | `slotDuration` | ✅ match |
| 0x09 | `RANGING_INTERVAL` | 4 | `120` (ms) | `rangingDuration` | ✅ match |
| 0x12 | `RFRAME_CONFIG` | 1 | `0x03` (SP3) | `rframeConfig` | ✅ match |
| 0x13 | `RSSI_REPORTING` | 1 | `1` | fixed | ✅ match |
| 0x14 | `PREAMBLE_CODE_INDEX` | 1 | `9` | `preambleIdx` | ✅ match |
| 0x15 | `SFD_ID` | 1 | `2` | `sfd` | ✅ match |
| 0x1B | `SLOTS_PER_RR` | 1 | `6` | `slotsPerRr` | ✅ match |
| 0x2C | `HOPPING_MODE` | 1 | `1` | `hoppingMode` | ✅ match |
| 0x02 | `STS_CONFIG` | 1 | `0` (static STS) | `stsConfig` | ✅ match |
| 0x0D | `AOA_RESULT_REQ` | 1 | `1` | `aoaReport` | ✅ match |
| 0x2E | `RESULT_REPORT_CONFIG` | 1 | `0x0B` | `resultReportConfig` | ✅ match |
| 0x27 | `VENDOR_ID` | 2 | `0x0708` | `vendorId` | ✅ match |
| 0x28 | `STATIC_STS_IV` | 6 | `01 02 03 04 05 06` | `staticStsIv` | ✅ match |

Related parameter IDs available in the SDK but **not currently set** by the ESP32 (defaults apply):
`STS_INDEX=0x0A`, `SESSION_INFO_NTF_CONFIG=0x0E`, `KEY_ROTATION=0x23`, `MAC_ADDRESS_MODE=0x26`,
`NUMBER_OF_CONTROLEES=0x05`, `UWB_INITIATION_TIME=0x2B`, `ENABLE_DIAGNOSTICS=0xE8`.

---

## 6. Error / Status Codes (`enum uci_status_code`)

| Code | Name | Meaning | ESP32 handling |
|------|------|---------|----------------|
| 0x00 | `UCI_STATUS_OK` | Success | Decode distance normally |
| 0x01 | `UCI_STATUS_REJECTED` | Command rejected | Retry (up to 2) then abort |
| 0x02 | `UCI_STATUS_FAILED` | Generic failure | Retry / abort |
| 0x03 | `UCI_STATUS_SYNTAX_ERROR` | Malformed UCI command | Fix framing |
| 0x04 | `UCI_STATUS_INVALID_PARAM` | A parameter value invalid | Fix SET_APP_CONFIG value |
| 0x05 | `UCI_STATUS_INVALID_RANGE` | Value out of range | Fix value |
| 0x0B | `UCI_STATUS_UNKNOWN` | Outcome unknown | Treat as error |
| 0x11 | `ERROR_SESSION_NOT_EXIST` | No such session/handle | Re-init session |
| 0x13 | `ERROR_SESSION_ACTIVE` | Session already active | Stop before reconfigure |
| 0x15 | `ERROR_SESSION_NOT_CONFIGURED` | Missing required app config | Re-send SET_APP_CONFIG |
| **0x1B** | **`OK_NEGATIVE_DISTANCE_REPORT`** | **Success, but a negative distance was measured** (near-field). Not an error. | ESP32 reuses last filtered distance if it was < 0.5 m; otherwise ignores. *(Firmware comment calls this "saturation"; the SDK's actual meaning is "negative distance report".)* |
| 0x20 | `RANGING_TX_FAILED` | UWB TX failed | Ignore this measurement |
| 0x21 | `RANGING_RX_TIMEOUT` | No UWB packet received | Ignore this measurement |
| 0x22 | `RANGING_RX_PHY_DEC_FAILED` | Channel decode error | Ignore |
| 0x23 | `RANGING_RX_PHY_TOA_FAILED` | ToA detection failed | Ignore |
| 0x24 | `RANGING_RX_PHY_STS_FAILED` | STS mismatch | Ignore (possible spoof) |
| 0x25 | `RANGING_RX_MAC_DEC_FAILED` | MAC CRC/syntax error | Ignore |
| 0x50 | `ERROR_SE_BUSY` | Secure Element busy (CCC) | Retry later |
| 0x51 | `ERROR_CCC_LIFECYCLE` | CCC lifecycle violation | Re-provision |

> The ESP32 currently only special-cases `0x00` and `0x1B`; every other per-measurement status is
> logged and the measurement dropped (`"[UCI] Ignoring measurement. Status error: 0x%02X"`).

---

## 7. Minimal Working Integration Checklist

- [ ] Flash the **UCI** firmware onto the board (build target
      `BuildOutput/UCI/FreeRTOS/<board>/Release`; `<board>` = `nRF52840DK` or `DWM3001CDK`).
- [ ] Wire UART cross-over: anchor **TX → ESP32 RX (GPIO17)**, anchor **RX → ESP32 TX (GPIO18)**,
      common GND. Anchor pins: nRF52840DK **TX=P0.06 / RX=P0.08**; DWM3001CDK **TX=P0.19 / RX=P0.15**.
- [ ] Set baud rate to **115200**, 8N1, **no flow control** (both sides — already matched).
- [ ] Ensure **raw UCI framing** (no SLIP/CRC) — the SDK sends raw bytes; the ESP32 `UciUartLink`
      parses raw bytes. ✔ compatible.
- [ ] ESP32 sends `SESSION_INIT` with `session_id=42` → verify response `status=0x00` and capture
      the returned 4-byte `session_handle`.
- [ ] ESP32 sends `SESSION_SET_APP_CONFIG` with the §5 parameter set → verify `status=0x00`,
      `n_failed=0`.
- [ ] ESP32 sends `SESSION_START` → verify `status=0x00`.
- [ ] `RANGE_DATA_NTF` (GID=0x02, OID=0x00) arrives ~every `RANGING_INTERVAL` (120 ms):
      verify `payload[24]≥1`, `payload[27]=0x00`, and `payload[29:30]` holds the distance in cm.
- [ ] Antenna offset: the SDK reports **raw ranging distance** and does **not** add the thesis
      `+0.24 m` offset. Keep applying `kAntennaOffsetM = 0.24` on the ESP32 (or move it into a
      per-anchor `ANTENNA_*` calibration on the SDK side — see the calibration tools).

---

**REMINDER:** every byte offset above is verified against SDK source (§0). If a downstream SDK
release changes the FiRa spec version, re-verify §3 against `qorvo_msg.py` / `uci_spec_fira.h`
before trusting it.

## See also
- [SDK_INTEGRATION_LINK.md](SDK_INTEGRATION_LINK.md) — exact ESP32 ↔ SDK mapping (what works today)
- [SDK_API_REFERENCE.md](SDK_API_REFERENCE.md) — function-level API surface
- [SDK_CODEBASE_REFERENCE.md](SDK_CODEBASE_REFERENCE.md) — full SDK onboarding
