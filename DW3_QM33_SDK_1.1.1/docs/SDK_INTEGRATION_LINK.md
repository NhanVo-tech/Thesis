# SDK_INTEGRATION_LINK.md

> Describes the integration between the ESP32 Smart Car Access thesis firmware
> and the DW3_QM33_SDK_1.1.1 anchor firmware.
> **Current architecture: Stage 1.5 — ESP32-C3 Bridge (2026-08-08).**
> This is the ground truth for what is actually working today.

---

## 1. Integration Overview

### 1.1 Current Architecture (Stage 1.5 — ESP32-C3 Bridge)

The system now uses a **separated-node architecture** to decouple the Master ESP32-S3 from raw UCI
protocol handling:

```
Phone (Flutter)                  ESP32-S3 Master            ESP32-C3 Bridge              nRF52840DK
┌─────────────┐    BLE(GATT)    ┌───────────────┐ ESP-NOW  ┌───────────────┐  UART/UCI  ┌──────────────┐
│ OOB payload  │──────────────▶│ UwbUciHost      │────────▶│ EspNowLink      │──────────▶│ UCI server     │
│ (37 bytes)  │               │ (submitBleOob) │         │ uci_session.cpp │          │ (task_uci.c)  │
│             │               │ sendStartToAux │         │ uci_uart.cpp    │          │ DW3000 UWB   │
│             │               │                │◀────────│ EspNowLink      │◀─────────│ RANGE_DATA    │
│             │               │ tick() → dist  │ Ranging │ sendRanging()  │ RANGE/   │ _NTF (56 B)   │
└─────────────┘               └───────────────┘         └───────────────┘  status   └──────────────┘
```

**Data flow (one-to-one, single anchor):**
1. Phone sends 37-byte OOB payload over BLE GATT to Master ESP32-S3
2. Master parses OOB → `UciRunConfig`, caches it
3. On `requestStart()`, Master sends `StartSession` struct over ESP-NOW to ESP32-C3 Bridge
4. ESP32-C3 runs full UCI flow: SESSION_INIT → SET_APP_CONFIG → RANGING_START over UART to nRF52840DK
5. nRF52840DK+ DW3000EVB performs FiRa TWR ranging with phone, emits `RANGE_DATA_NTF` over UART
6. ESP32-C3 parses `RANGE_DATA_NTF`, extracts distance/status, sends `RangingMsg` back to Master over ESP-NOW
7. Master feeds distance into Kalman → LSTM → DoorUnlock pipeline

**Why the ESP32-C3 bridge?**
- Master ESP32-S3 already handles BLE (NimBLE), NFC (PN532), FSM, LSTM inference simultaneously
- Offloading UCI session management to a dedicated ESP32-C3 prevents UART timing conflicts
- ESP-NOW provides reliable, low-latency wireless link between nodes
- ESP32-C3 bridge is identical for all anchors (per-anchor `ANCHOR_ID` compile flag); 3 anchors = 3 identical C3 nodes

### 1.2 Legacy Architecture (Stage 1 — Direct UART, still functional as fallback)

```
ESP32-S3 (yolo_uno board)               nRF52840DK + DWM3000EVB (SDK UCI FW)
┌─────────────────────────┐             ┌──────────────────────────────┐
│ UART1 RX = GPIO17       │◀────────────│ UART0 TX = P0.06             │
│ UART1 TX = GPIO18       │────────────▶│ UART0 RX = P0.08             │
│ @ 115200 baud, 8N1      │   GND──GND   │ @ 115200 8N1, no flow ctrl   │
│ no flow control         │             │ raw UCI, UCI server app      │
└─────────────────────────┘             └──────────────────────────────┘
   UciUartLink / UciSessionManager          Src/Apps/Src/uci (task_uci.c)
```

DWM3001CDK variant: UART0 TX = P0.19, RX = P0.15 (same baud/framing).

Pin sources: [nRF52840DK custom_board.h](../SDK/Firmware/Src/Boards/Src/nRF52840DK/Common/custom_board.h)
(`TX_PIN_NUMBER`/`RX_PIN_NUMBER` from the pca10056 BSP → P0.06 / P0.08),
[DWM3001CDK custom_board.h](../SDK/Firmware/Src/Boards/Src/DWM3001CDK/Common/custom_board.h)
(`TX_PIN_NUMBER=19`, `RX_PIN_NUMBER=15`), ESP32-C3 Bridge
[`thesis253_workspace/platformio.ini`](../../thesis253_workspace/platformio.ini) (`ANCHOR_UART_RX=20`, `ANCHOR_UART_TX=21`).

---

## 2. Mapping: ESP32 Firmware → SDK Concept

### 2.1 ESP32-C3 Bridge (current Stage 1.5)

| ESP32-C3 Bridge (`thesis253_workspace`) | SDK component | Notes |
|---|---|---|
| `UciUart::begin(20, 21, 115200)` | `HAL_uart.c → deca_uart_init()` | baud **115200** confirmed both sides |
| `UciUart::poll(onRanging)` | `RANGE_DATA_NTF` (GID=0x02, OID=0x00) | parses p[27]=status, p[29:30]=distance |
| `UciUart::sendCommand(gid,oid,payload,len)` | UCI Common Packet Header | `octet0=(1<<5)|gid`, `octet1=oid` |
| `UciSession::run(cfg)` | SESSION_INIT → SET_APP_CONFIG → SESSION_START | same flow as master's UciSessionManager |
| `EspNowLink::begin()` / `sendRanging()` | *(ESP-NOW OOB channel)* | StartMsg from master, RangingMsg back |
| `UciSession::Config.controlee → devType` | `DEVICE_TYPE=0x00` / `DEVICE_ROLE=0x11` | conditional, not hardcoded (fixed 2026-08-08) |
| `K_GID_SESSION = 0x01` | `UCI_GID_SESSION_CONFIG = 0x01` | ✅ correct |
| `K_GID_RANGING = 0x02` | `UCI_GID_SESSION_CONTROL = 0x02` | ✅ correct |
| `payload[24] = num_meas` | `RANGE_DATA_NTF` num_measurements | ✅ offset correct |
| `payload[27] = status` | TWR measurement `status` | ✅ offset correct |
| `payload[29:30] = distance_cm` | TWR measurement `distance` (uint16 LE, cm) | ✅ offset correct |

### 2.2 ESP32-S3 Master (legacy direct-UART, Stage 1)

| ESP32-S3 Master firmware | SDK component | Notes / verification |
|---|---|---|
| `UciUartLink::begin(Serial1, 17, 18, 115200)` | UART init in `HAL_uart.c → deca_uart_init()` | baud **115200** confirmed both sides |
| `UciUartLink::poll()` | `uci_tp_read()` framing over `cc_buff` | raw UCI, 4-byte header + `data[3]` length |
| `UciUartLink::sendPacket(mt,gid,oid,payload,pbf)` | UCI Common Packet Header build | `octet0=(mt<<5)|(pbf<<4)|gid`, `octet1=oid`, `octet3=len` — matches `uci_spec_fira.h` |
| `UciSessionManager::runOnce(cfg)` | host drives `SESSION_INIT → SET_APP_CONFIG → SESSION_START` | server is passive; host sequences it |
| `UciSessionManager::onPacket()` | receives `RANGE_DATA_NTF` (GID=0x02, OID=0x00) | see §5 for offset verification |
| `kGidSession = 0x01` | `UCI_GID_SESSION_CONFIG = 0x01` | ✅ correct |
| `kGidRanging = 0x02` | `UCI_GID_SESSION_CONTROL = 0x02` | ✅ correct |
| `kOidSessionInit = 0x00` | `UCI_OID_SESSION_INIT = 0x00` | ✅ correct |
| `kOidSessionSetAppConfig = 0x03` | `UCI_OID_SESSION_SET_APP_CONFIG = 0x03` | ✅ correct |
| `kOidRangingStart = 0x00` | `UCI_OID_SESSION_START = 0x00` | ✅ correct |
| `kOidRangingStop = 0x01` | `UCI_OID_SESSION_STOP = 0x01` | ✅ correct |
| `kOidSessionDeinit = 0x01` | `UCI_OID_SESSION_DEINIT = 0x01` | ✅ correct |
| `UciRunConfig.channel=9` | `SET_APP_CONFIG` `CHANNEL_NUMBER = 0x04` | ✅ param ID correct |
| `UciRunConfig.destMac=0x0001` | `DST_MAC_ADDRESS = 0x07` | ✅ param ID correct |
| `UciRunConfig.stsConfig=0` | `STS_CONFIG = 0x02` (0 = static STS) | ✅ param ID correct |
| `payload[24] = num_meas` | `RANGE_DATA_NTF` num_measurements | ✅ offset correct |
| `payload[27] = status` | TWR measurement `status` | ✅ offset correct |
| `payload[29:30] = distance_cm` | TWR measurement `distance` (uint16 LE, cm) | ✅ offset correct |
| `kAntennaOffsetM = 0.24 m` | *(no SDK equivalent applied in the reported distance)* | ⚠️ SDK reports raw distance; offset is ESP32-only — see GAP-2 |

---

## 3. Command Sequence as Executed by `UciSessionManager::runOnce()`

```
T+0ms     ESP32 → ANCHOR: SESSION_INIT_CMD     [GID=0x01 OID=0x00]  payload: session_id=42(LE), session_type=0x00
T+~5ms    ANCHOR → ESP32: SESSION_INIT_RSP     [GID=0x01 OID=0x00]  status=0x00, session_handle(4, LE)
          (ANCHOR → ESP32: SESSION_STATUS_NTF  [GID=0x01 OID=0x02]  state=INIT/IDLE — optional)

T+Xms     ESP32 → ANCHOR: SET_APP_CONFIG_CMD   [GID=0x01 OID=0x03]  payload: session_handle(4), n_params, TLVs…
          params: DEVICE_TYPE=controller, DEVICE_ROLE=initiator, MULTI_NODE_MODE=0,
                  RANGING_ROUND_USAGE=2, CHANNEL=9, SCHEDULE_MODE=1, DEVICE_MAC=0x0000,
                  DST_MAC=0x0001, SLOT_DURATION=2400, RANGING_INTERVAL=120, RFRAME_CONFIG=3,
                  RSSI_REPORTING=1, PREAMBLE_CODE_INDEX=9, SFD_ID=2, SLOTS_PER_RR=6,
                  HOPPING_MODE=1, STS_CONFIG=0, AOA_RESULT_REQ=1, RESULT_REPORT_CONFIG=0x0B,
                  VENDOR_ID=0x0708, STATIC_STS_IV=01..06
T+Xms     ANCHOR → ESP32: SET_APP_CONFIG_RSP   [GID=0x01 OID=0x03]  status=0x00, n_failed=0

T+Xms     ESP32 → ANCHOR: SESSION_START_CMD    [GID=0x02 OID=0x00]  payload: session_handle(4, LE)
T+Xms     ANCHOR → ESP32: SESSION_START_RSP    [GID=0x02 OID=0x00]  status=0x00

[every ~120ms — set by RANGING_INTERVAL]
T+Xms     ANCHOR → ESP32: RANGE_DATA_NTF       [GID=0x02 OID=0x00]  distance @ payload[29:30] cm
...

T+Xms     ESP32 → ANCHOR: SESSION_STOP_CMD     [GID=0x02 OID=0x01]  payload: session_handle(4, LE)
T+Xms     ANCHOR → ESP32: SESSION_STOP_RSP     [GID=0x02 OID=0x01]  status=0x00
T+Xms     ESP32 → ANCHOR: SESSION_DEINIT_CMD   [GID=0x01 OID=0x01]  payload: session_handle(4, LE)
T+Xms     ANCHOR → ESP32: SESSION_DEINIT_RSP   [GID=0x01 OID=0x01]  status=0x00
```

Timing notes:
- The notification cadence is governed by `RANGING_INTERVAL` (`UciRunConfig.rangingDuration = 120` ms).
- `SLOT_DURATION = 2400` RSTU and `SLOTS_PER_RR = 6` set the internal ranging-round structure.
- The ESP32 retries each command up to 2× (`sendCommandWithRetry`), polling for the matching
  response GID/OID; `SESSION_INIT` timeout = 1200 ms, `SET_APP_CONFIG` = 2000 ms.

---

## 4. Verified Working Parameters

| `UciRunConfig` field | Value | SDK `SET_APP_CONFIG` param ID | Verified? |
|---|---|---|---|
| `sessionId` | 42 | (SESSION_INIT arg, not app-config) | ✅ |
| `controlee` → `DEVICE_TYPE`/`DEVICE_ROLE` | false → controller/initiator | 0x00 / 0x11 | ✅ |
| `channel` | 9 | 0x04 `CHANNEL_NUMBER` | ✅ |
| `scheduleMode` | 1 | 0x22 `SCHEDULE_MODE` | ✅ |
| `preambleIdx` | 9 | 0x14 `PREAMBLE_CODE_INDEX` | ✅ |
| `sfd` | 2 | 0x15 `SFD_ID` | ✅ |
| `slotDuration` | 2400 | 0x08 `SLOT_DURATION` | ✅ |
| `rangingDuration` | 120 | 0x09 `RANGING_INTERVAL` | ✅ |
| `slotsPerRr` | 6 | 0x1B `SLOTS_PER_RR` | ✅ |
| `hoppingMode` | 1 | 0x2C `HOPPING_MODE` | ✅ |
| `stsConfig` | 0 | 0x02 `STS_CONFIG` | ✅ |
| `aoaReport` | 1 | 0x0D `AOA_RESULT_REQ` | ✅ |
| `vendorId` | 0x0708 | 0x27 `VENDOR_ID` | ✅ |
| `staticStsIv` | 01..06 | 0x28 `STATIC_STS_IV` | ✅ |
| `resultReportConfig` | 0x0B | 0x2E `RESULT_REPORT_CONFIG` | ✅ |
| `rframeConfig` | 0x03 | 0x12 `RFRAME_CONFIG` | ✅ |
| `localMac` | 0x0000 | 0x06 `DEVICE_MAC_ADDRESS` | ✅ |
| `destMac` | 0x0001 | 0x07 `DST_MAC_ADDRESS` | ✅ |

Every parameter ID the ESP32 sends exists in the SDK's
`enum uci_application_configuration_parameters` with the same value. No `❌` mismatches found.

---

## 5. Known Gaps / Integration Issues (Stage 1)

**GAP-1: Byte offsets — no mismatch (verification result).**
- ESP32 firmware assumes: `num_meas @ payload[24]`, `status @ payload[27]`,
  `distance @ payload[29:30]` (uint16 LE, cm).
- SDK actually does: identical layout (verified against `class RangingData` / `class RangingTwrData`
  in `qorvo_msg.py`; see [SDK_UCI_FLOW.md §3](SDK_UCI_FLOW.md)).
- Impact: **none** — offsets are correct.
- Fix: none required.

**GAP-2: Antenna offset applied twice / assumption.**
- ESP32 firmware assumes: it must add `+0.24 m` (`kAntennaOffsetM`) to every reported distance.
- SDK actually does: reports the **raw** ranging distance in the `RANGE_DATA_NTF`; it applies antenna
  delay only if a per-device calibration is loaded (see the `load_cal`/`set_cal` tools and
  `calib_files/`). The stock UCI firmware ships no thesis-specific 0.24 m compensation.
- Impact: warning — if a calibration is later flashed on the anchor, the offset could be applied
  twice.
- Fix: keep `kAntennaOffsetM` on the ESP32 **or** move it into an anchor-side antenna calibration,
  but not both. File: [iot/src/uwb/uci_session_manager.cpp](../../SmartCarAccess-main/iot/src/uwb/uci_session_manager.cpp).

**GAP-3: Status `0x1B` semantics.**
- ESP32 firmware assumes: `0x1B` == near-field "saturation" and reuses the last filtered distance if
  it was < 0.5 m.
- SDK actually does: `0x1B == UCI_STATUS_OK_NEGATIVE_DISTANCE_REPORT` — a *successful* measurement
  that yielded a negative distance (target essentially on top of the antenna).
- Impact: none functionally — the heuristic is reasonable — but the comment/name is misleading.
- Fix: rename the constant/comment to reflect "negative distance report". File: `uci_session_manager.cpp`.

**GAP-4: Per-measurement MAC address ignored.**
- ESP32 firmware assumes: a single anchor; it never reads `mac_address` at `payload[25:26]`.
- SDK actually does: includes the peer MAC in every measurement block (needed to tell anchors apart).
- Impact: none for one anchor; **blocks multi-anchor** (Anchors 1 & 2).
- Fix: read `payload[25:26]` and route by MAC when adding DWM3001CDK anchors. File: `uci_session_manager.cpp`.

**GAP-5: Notifications not consumed.**
- ESP32 firmware ignores: `SESSION_STATUS_NTF` (GID=0x01 OID=0x02) and `CORE_DEVICE_STATUS_NTF`
  (GID=0x00 OID=0x01).
- SDK actually sends: a `CORE_DEVICE_STATUS_NTF(READY)` on boot and `SESSION_STATUS_NTF` on state
  changes.
- Impact: warning — the ESP32 relies on command/response timing instead of the READY notification;
  works, but a reset mid-session could be missed.
- Fix (optional): gate `runOnce()` on the boot READY notification. File: `uci_session_manager.cpp`.

---

## 6. nRF52840DK+DWM3000EVB vs DWM3001CDK Integration Delta

For adding DWM3001CDK as Anchors 1 & 2:

| Item | Detail |
|------|--------|
| Build target / binary | `BuildOutput/UCI/FreeRTOS/DWM3001CDK/Release` (flash with `nrf52833_xxaa`) vs. `.../nRF52840DK/Release` (`nrf52840_xxaa`) |
| UART pins | DWM3001CDK **TX=P0.19, RX=P0.15**; nRF52840DK **TX=P0.06, RX=P0.08** — only the anchor-side wiring changes; ESP32 side (GPIO17/18) is unchanged |
| UCI payload format | **Identical** — same app + same uwb-stack backend; `onPacket()` needs no format change |
| `UciRunConfig` reuse | Reusable as-is, **except** give each anchor a unique `DEVICE_MAC_ADDRESS` and set the matching `destMac` per anchor |
| Parameter IDs | Identical between both variants (same `uci_spec_fira.h`) |
| Code change required | Implement GAP-4 (read `payload[25:26]` MAC) so three anchors can be distinguished in one firmware |
| Baud / framing | Identical (115200 8N1, raw UCI) |

---

## See also
- [SDK_UCI_FLOW.md](SDK_UCI_FLOW.md) — byte-level UCI protocol (the authority for §5)
- [SDK_API_REFERENCE.md](SDK_API_REFERENCE.md) — SDK API surface
- [SDK_CODEBASE_REFERENCE.md](SDK_CODEBASE_REFERENCE.md) — full SDK onboarding
