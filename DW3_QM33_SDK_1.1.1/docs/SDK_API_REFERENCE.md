# SDK_API_REFERENCE.md

Public functions, structs and callbacks of DW3_QM33_SDK_1.1.1 that matter for thesis integration.
Signatures reflect the SDK headers. Only the **current (Stage 1)** application/backend surface is
covered — the full DW3xxx register-level driver API is out of scope (see the SDK Developer Manual).

> Format mirrors [SmartCarAccess-main/docs/API_REFERENCE.md](../../SmartCarAccess-main/docs/API_REFERENCE.md):
> tables of `Signature | Description | Returns | Side effects`.

---

## 1. UCI application task — [SDK/Firmware/Src/Apps/Src/uci/task_uci/task_uci.h](../SDK/Firmware/Src/Apps/Src/uci/task_uci/task_uci.h)

The top-level UCI server application. Started once at boot; thereafter the host (ESP32) drives it
purely over the UCI wire protocol.

| Signature | Description | Returns | Side effects |
|-----------|-------------|---------|--------------|
| `void uci_helper(void const *arg)` | Create and launch the UCI FreeRTOS task | void | allocates task stack, spawns thread `"UCI"` |
| `void uci_task(void *argument)` | UCI task main loop; dequeues UART data and feeds the parser | (noreturn) | processes `UCI_DATA_IN` messages |
| `int uci_open_backends(void)` | Init uwbmac, UCI server, coordinator, core + FiRa backends | 0 / err | allocates MAC + UCI contexts, sends device-ready ntf |
| `void uci_close_backends(void)` | Tear down all backends | void | frees contexts |
| `bool uci_sw_reset(void)` | Software reset: close + reopen backends | success | re-inits UWBS |
| `void uci_interface_select(void)` | Bind the UCI transport to UART0 on first traffic | void | attaches transport |
| `void uci_terminate(void)` | Kill the UCI task and release resources | void | stops thread, deinits MAC |

## 2. UWB MAC helper — [SDK/Firmware/Src/Apps/Src/uci/uwbmac_helper/include/uwbmac_helper.h](../SDK/Firmware/Src/Apps/Src/uci/uwbmac_helper/include/uwbmac_helper.h)

| Signature | Description | Returns | Side effects |
|-----------|-------------|---------|--------------|
| `int uwbmac_helper_init_fira(void)` | Initialise the DW3000/QM33 driver for FiRa ranging (default) | 0 / err | configures the radio |
| `void uwbmac_helper_init_mcps(void)` | Initialise the MAC in MCPS mode | void | radio config |
| `void uwbmac_helper_deinit(void)` | Release the MAC/driver | void | powers down radio |

## 3. UCI transport — [SDK/Firmware/Src/Apps/Src/uci/uci_transport/include/uci_transport.h](../SDK/Firmware/Src/Apps/Src/uci/uci_transport/include/uci_transport.h)

Platform-agnostic UCI framing layer. **This is where the raw UCI byte framing lives** (§1.2 of
`SDK_UCI_FLOW.md`).

| Signature | Description | Returns | Side effects |
|-----------|-------------|---------|--------------|
| `int uci_tp_read(struct uci_tp *tr, struct cc_buff *buf)` | Consume UART bytes; assemble complete UCI packets by header length `data[3]`; hand to UCI core | bytes read / err | may call `uci_packet_recv()`; flushes on 100 ms garbage timeout |
| `void uci_tp_attach(struct uci_transport *tr, struct uci *uci)` | Bind transport to a UCI server instance | void | — |
| `void uci_tp_detach(struct uci_transport *tr)` | Unbind transport | void | frees pending rx block |
| `void uci_tp_usb_packet_send_ready(struct uci_transport *tr)` | Flush ready UCI packets to the reporter (UART/USB TX) | void | writes bytes to UART |
| `void uci_tp_flush(struct uci_tp *tr)` | Drop the partially received packet | void | frees rx block |

`enum uci_if_e { UCI_NONE, UCI_UART0, UCI_UART1 }` selects the output interface (UART0 is used).

## 4. UCI parser — [SDK/Firmware/Src/Apps/Src/uci/uci_parser.h](../SDK/Firmware/Src/Apps/Src/uci/uci_parser.h)

| Signature | Description | Returns | Side effects |
|-----------|-------------|---------|--------------|
| `usb_data_e uci_on_rx(struct cc_buff *buf)` | RX hook; reports `DATA_READY` when the circular buffer is non-empty (UCI works directly on the buffer) | `DATA_READY` / `NO_DATA` | — |

## 5. FiRa UCI backend — [SDK/Firmware/Libs/.../uci_bundle/uci_backend/uci_backend_fira.h](../SDK/Firmware/Libs/uwbstack_libs/delivery/full/Release/include/uci_bundle/uci_backend/uci_backend_fira.h)

Bridges ranging-type UCI sessions to the MAC's FiRa region. This is the component that ultimately
**builds the `RANGE_DATA_NTF`** consumed by the ESP32.

| Signature | Description | Returns | Side effects |
|-----------|-------------|---------|--------------|
| `enum qerr uci_backend_fira_init(ctx, core_ctx, uci, uwbmac_ctx, coord, sess_man)` | Attach the FiRa backend to the UCI server | `QERR_SUCCESS`/err | registers session ops |
| `void uci_backend_fira_set_antenna_conf(ctx, antennas_params)` | Provide antenna config for AoA sessions | void | affects AoA output |
| `enum qerr uci_backend_fira_get_supported_channels(ctx, *channel_number)` | Query supported channels bitmap | `QERR_SUCCESS`/err | — |
| `void uci_backend_fira_release(ctx)` | Free backend resources | void | — |

`struct uci_backend_fira_context` holds: `fira_context`, `uwbmac_context`, `antennas`, `uci`,
`uci_backend_core_context`, `coord`, `sess_man`, `session_ops`.

## 6. HAL UART — [SDK/Firmware/Src/HAL/Inc/HAL_uart.h](../SDK/Firmware/Src/HAL/Src/nrfx/HAL_uart.c)

| Signature | Description | Returns | Side effects |
|-----------|-------------|---------|--------------|
| `void deca_uart_init(CommRxCallback callback)` | Init UART0 at 115200 8N1, no flow control; register RX callback | void | opens `app_uart` FIFO |
| `int deca_uart_transmit(uint8_t *ptr, uint16_t size)` | Send bytes over UART | `NRF_SUCCESS`/err | UART TX |
| `void deca_uart_receive(void)` | ISR-context RX drain into `cc_buff` | void | fills rx buffer |
| `void deca_uart_close(void)` | Flush and close UART | void | — |

---

## CRITICAL-1 — Ranging result path (fires when a distance is ready)

There is **no host-registerable C callback**; the anchor emits the result as a UCI notification over
UART. The consumer is the host's packet handler.

- **Emitter:** the FiRa backend (`uci_backend_fira`) → `RANGE_DATA_NTF`
  (`MT=Notification`, `GID=0x02`, `OID=0x00`).
- **Result struct (over the wire):** see [SDK_UCI_FLOW.md §3](SDK_UCI_FLOW.md). Key offsets:
  `num_measurements @ payload[24]`, per-TWR-measurement `status @ payload[27]`,
  `distance @ payload[29:30]` (uint16 LE, cm).
- **Host consumer:** `UciSessionManager::onPacket()` (thesis firmware).

## CRITICAL-2 — Start a session (`SESSION_INIT`)

- **Wire API:** `MT=Command GID=0x01 OID=0x00`; payload `[session_id(4 LE)][session_type(1)]`
  (`session_type=0x00` = FiRa ranging).
- **Response:** `GID=0x01 OID=0x00`, payload `[status(1)][session_handle(4 LE)]`.
- There is no separate host C function — the session is created by sending the UCI command; inside
  the SDK it is realised by the backend manager dispatching to `uci_backend_fira`.

## CRITICAL-3 — Configure a session (`SET_APP_CONFIG`)

- **Wire API:** `MT=Command GID=0x01 OID=0x03`; payload
  `[session_handle(4 LE)][n_params(1)][TLV…]`, TLV = `[param_id(1)][len(1)][value]`.
- **Configurable parameters:** full list in `enum uci_application_configuration_parameters`
  (`uci_spec_fira.h`). The thesis-relevant subset and defaults are tabulated in
  [SDK_UCI_FLOW.md §5](SDK_UCI_FLOW.md) — includes `CHANNEL_NUMBER(0x04)`, `PREAMBLE_CODE_INDEX(0x14)`,
  `SFD_ID(0x15)`, `STS_CONFIG(0x02)`, `SLOT_DURATION(0x08)`, `DST_MAC_ADDRESS(0x07)`,
  `DEVICE_TYPE(0x00)`, `DEVICE_ROLE(0x11)`, `RANGING_INTERVAL(0x09)`, `RESULT_REPORT_CONFIG(0x2E)`,
  `VENDOR_ID(0x27)`, `STATIC_STS_IV(0x28)`.

## CRITICAL-4 — Start / stop ranging

- **Start:** `MT=Command GID=0x02 OID=0x00`, payload `[session_handle(4 LE)]`; response
  `[status(1)]`. Ranging notifications (`GID=0x02 OID=0x00`) then stream.
- **Stop:** `MT=Command GID=0x02 OID=0x01`, payload `[session_handle(4 LE)]`; response `[status(1)]`.

## CRITICAL-5 — OOB payload structure

**Not an SDK feature.** DW3_QM33_SDK_1.1.1 has **no BLE OOB session-parameter module** in the UCI
application — all session parameters are set in-band via `SET_APP_CONFIG`. The OOB struct
(`UciOobPayloadV1`, 37 bytes) lives **only in the ESP32 thesis firmware**
([iot/include/uwb/uci_oob.h](../../SmartCarAccess-main/iot/include/uwb/uci_oob.h)) and is delivered
over the phone↔ESP32 BLE link, not to the anchor. See
[SDK_INTEGRATION_LINK.md](SDK_INTEGRATION_LINK.md).

---

## UCI Packet Reference Table

Values from `uci_spec_fira.h`. Direction: H→A = host (ESP32) → anchor; A→H = anchor → host.

| Message | Dir | GID | OID | Key payload fields (offset → meaning) |
|---------|-----|-----|-----|----------------------------------------|
| `CORE_DEVICE_RESET` | H→A | 0x00 | 0x00 | [0] reset config |
| `CORE_DEVICE_STATUS_NTF` | A→H | 0x00 | 0x01 | [0] device state (0x01=READY, 0x02=ACTIVE, 0xFF=ERROR) |
| `CORE_GET_DEVICE_INFO` | H→A | 0x00 | 0x02 | (none) |
| `CORE_GET_DEVICE_INFO_RSP` | A→H | 0x00 | 0x02 | [0] status, [1..] version fields + vendor data |
| `CORE_GET_CAPS_INFO` | H→A | 0x00 | 0x03 | (none) |
| `SESSION_INIT_CMD` | H→A | 0x01 | 0x00 | [0:3] session_id, [4] session_type |
| `SESSION_INIT_RSP` | A→H | 0x01 | 0x00 | [0] status, [1:4] session_handle |
| `SESSION_DEINIT_CMD` | H→A | 0x01 | 0x01 | [0:3] session_handle |
| `SESSION_STATUS_NTF` | A→H | 0x01 | 0x02 | [0:3] session_id, [4] state, [5] reason |
| `SESSION_SET_APP_CONFIG_CMD` | H→A | 0x01 | 0x03 | [0:3] session_handle, [4] n_params, [5:] TLVs |
| `SESSION_SET_APP_CONFIG_RSP` | A→H | 0x01 | 0x03 | [0] status, [1] n_failed, [2:] (param,status) |
| `SESSION_GET_APP_CONFIG_CMD` | H→A | 0x01 | 0x04 | [0:3] handle, [4] n, ids… |
| `SESSION_START_CMD` | H→A | 0x02 | 0x00 | [0:3] session_handle |
| `SESSION_START_RSP` | A→H | 0x02 | 0x00 | [0] status |
| `RANGE_DATA_NTF` (`SESSION_INFO_NTF`) | A→H | 0x02 | 0x00 | see [SDK_UCI_FLOW.md §3](SDK_UCI_FLOW.md) |
| `SESSION_STOP_CMD` | H→A | 0x02 | 0x01 | [0:3] session_handle |
| `SESSION_GET_RANGING_COUNT` | H→A | 0x02 | 0x03 | [0:3] session_handle |

Vendor GIDs present (build-dependent): `QORVO_EXT2=0x0B`, `ANDROID=0x0C`, `TEST=0x0D`,
`QORVO_MAC=0x0E`, `QORVO_CALIB=0x0F`.

---

## Callback & Event Reference

| Event / callback | Trigger | Parameters | Notes |
|------------------|---------|------------|-------|
| `uci_on_rx(cc_buff*)` | UART bytes available | rx buffer | returns `DATA_READY`/`NO_DATA` |
| `deca_uart_event_handle(app_uart_evt_t*)` | UART ISR event | event type | `DATA_READY` → `deca_uart_receive()` |
| `uci_reset_cb(reason, user_data)` | `CORE_DEVICE_RESET` received | reset reason | performs `uci_sw_reset()` |
| `RANGE_DATA_NTF` | Each ranging round completes (~`RANGING_INTERVAL`) | UCI notification | primary integration event |
| `CORE_DEVICE_STATUS_NTF` | Boot / reset complete | device state byte | emit READY before commands |
| `SESSION_STATUS_NTF` | Session state change | session_id, state, reason | INIT→IDLE→ACTIVE→DEINIT |

---

## See also
- [SDK_UCI_FLOW.md](SDK_UCI_FLOW.md) — byte-level protocol
- [SDK_INTEGRATION_LINK.md](SDK_INTEGRATION_LINK.md) — ESP32 ↔ SDK mapping
- [SDK_CODEBASE_REFERENCE.md](SDK_CODEBASE_REFERENCE.md) — full onboarding
