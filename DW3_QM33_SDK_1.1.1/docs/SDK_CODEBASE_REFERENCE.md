# SDK_CODEBASE_REFERENCE.md

> Complete onboarding document for **DW3_QM33_SDK_1.1.1**.
> Written for AI-assisted development sessions targeting the **ESP32 Smart Car Access** thesis
> integration. Covers **ONLY** current (Stage 1) SDK capabilities.

---

## 1. SDK Overview

### 1.1 Purpose / Domain

DW3_QM33_SDK_1.1.1 is Qorvo's development kit for **UWB (Ultra-Wideband) secure ranging** on the
DW3000 / QM33 radio family. It provides:

- A **DW3xxx low-level driver** (`Drivers/`) with register-level access and worked ranging examples
  for the nRF52840-DK and ST Nucleo-F429 platforms.
- A **FreeRTOS firmware SDK** (`SDK/Firmware/`) built on the Nordic nRF5 SDK 17.1 and Qorvo's
  **uwb-stack** (FiRa MAC + UCI stack), with ready-to-run applications: **UCI**, **FiRa CLI**,
  **Listener**, and **Reporter**.
- A **UCI (UWB Command Interface)** server that lets an external host control ranging over UART/USB,
  implementing the **FiRa Consortium UCI Generic Technical Specification 2.0.0** and Qorvo/CCC
  extensions.
- Host-side **Python tooling** (`SDK/Tools/uwb-qorvo-tools`) and a GUI (Qorvo UWB Explorer).

**Role in the thesis:** the anchors run the SDK's **UCI application** and act as the UWBS
(UWB Subsystem). The ESP32 is the **UCI host / Controller-Initiator**; it configures a ranging
session and reads distance from `RANGE_DATA_NTF` messages. The phone is the mobile Responder in the
overall system, but for the SDK the relevant peer is the ESP32 over UART.

### 1.2 Tech Stack Summary

| Layer | Language / Framework | Key libraries / tools |
|-------|----------------------|-----------------------|
| Radio driver | C | `dwt_uwb_driver` (DW3000/QM33 HAL) |
| MAC / protocol | C | Qorvo **uwb-stack** (FiRa region, MCPS 802.15.4), `niq` |
| UCI stack | C | `uci_bundle` (uci core, backends: core/fira/mac/pctt/cfg_mgr) |
| RTOS / BSP | C | FreeRTOS, Nordic nRF5 SDK 17.1, SoftDevice S113 (BLE, optional) |
| Build | CMake + arm-none-eabi-gcc 10.3 | `CreateTarget.py`, `make`, J-Link flashing |
| Host tools | Python 3.10+ | `uwb-qorvo-tools` (UCI over UART), Qorvo UWB Explorer GUI |

### 1.3 High-Level Architecture

```
   Phone (UWB Initiator/Responder)
        │  UWB PHY (air) — FiRa TWR ranging round
        ▼
 ┌─────────────────────────────────────────────┐
 │ Anchor board (nRF52840DK+DWM3000EVB /        │
 │                DWM3001CDK)                    │
 │  DW3000/QM33 radio ── dwt_uwb_driver         │
 │        │                                     │
 │  uwb-stack (FiRa MAC region, MCPS)           │
 │        │                                     │
 │  UCI backend (fira) → UCI server → transport │
 │        │  raw UCI packets                    │
 └────────┼─────────────────────────────────────┘
          │  UART0 @ 115200 8N1 (no flow ctrl)
          ▼
 ┌─────────────────────────────────────────────┐
 │ ESP32-S3 firmware (thesis)                   │
 │  UciUartLink → UciSessionManager             │
 │    → Kalman → LSTM → UwbDoorUnlock (relay)   │
 └─────────────────────────────────────────────┘
```

### 1.4 Hardware Targets

**Target A — nRF52840DK + DWM3000EVB** (thesis Anchor 0)

| Property | Value |
|----------|-------|
| Host MCU | Nordic **nRF52840** (`nrf52840_xxaa`) |
| Radio | DWM3000EVB (DW3000, non-AoA / single antenna) |
| SDK coverage | `SDK/Firmware/Projects/FreeRTOS/UCI/nRF52840DK`, `Src/Boards/Src/nRF52840DK` |
| UCI UART pins | `TX = P0.06`, `RX = P0.08` (from pca10056 BSP `TX_PIN_NUMBER`/`RX_PIN_NUMBER`) |
| Ranging role | Host-driven; typically Controller/Initiator when paired to the ESP32 |
| Quirks | No AoA hardware; DWM3000EVB is an Arduino-shield add-on to the DK |

**Target B — DWM3001CDK** (thesis Anchors 1 & 2)

| Property | Value |
|----------|-------|
| Host MCU | Nordic **nRF52833** (`nrf52833_xxaa`), inside the DWM3001C module |
| Radio | QM33 (DW3110) with optional AoA antenna (`AOA_CHIP_ON_NON_AOA_PCB`) |
| SDK coverage | `SDK/Firmware/Projects/FreeRTOS/UCI/DWM3001CDK`, `Src/Boards/Src/DWM3001CDK` |
| UCI UART pins | `TX = P0.19`, `RX = P0.15` (`custom_board.h`) |
| Ranging role | Host-driven; Controller/Initiator when paired to the ESP32 |
| Quirks | Integrated module (onboard J-Link), 4 LEDs, USB-CDC available in addition to UART |

Common: 115200 8N1, no HW flow control (`HAL_uart.c`). A third target, **Type2AB_EVB** (Murata),
also exists in the SDK but is not used by the thesis.

---

## 2. Directory & File Map

```
DW3_QM33_SDK_1.1.1/
├── README.md
├── Drivers/                                    # DW3xxx low-level driver + platform examples
│   ├── Changelog.md, README.md
│   ├── API/
│   │   ├── Shared/dwt_uwb_driver/              # [KEY MODULE] DW3000/QM33 register driver
│   │   ├── Src/
│   │   │   ├── config_options.{c,h}           # [CONFIG] driver config options
│   │   │   ├── example_selection.h            # [CONFIG] which example to build
│   │   │   ├── examples/                       # SS/DS-TWR, TX/RX, AES, etc. worked examples
│   │   │   ├── MAC_802_15_4/, MAC_802_15_8/    # MAC layer helpers
│   │   │   └── tests/
│   │   └── Build_Platforms/
│   │       ├── nRF52840-DK/                    # nRF52840 BSP for the driver examples
│   │       └── STM_Nucleo_F429/                # ST BSP
│   ├── cmake/                                  # [CONFIG] toolchain + config cmake
│   └── docs/html/                              # Doxygen driver docs
│
└── SDK/
    ├── Binaries/                               # Prebuilt .hex for DWM3001CDK, nRF52840DK, Type2AB
    ├── Documentation/                          # DeveloperManual, QuickStart, uwb-stack, Drivers
    ├── Firmware/
    │   ├── DW3_QM33_SDK.code-workspace         # [CONFIG] VS Code workspace
    │   ├── README.md, README_WARNING.md, requirements.txt
    │   ├── Libs/
    │   │   ├── dwt_uwb_driver/                 # driver (mirror)
    │   │   ├── niq/                            # Qorvo NIQ lib
    │   │   ├── uwb-stack/, uwbstack_libs/      # [KEY MODULE] FiRa MAC + UCI stack (precompiled)
    │   │   │   └── delivery/full/Release/include/uci_bundle/uci/uci_spec_fira.h  # ← UCI spec (authoritative)
    │   │   └── LICENSES/
    │   ├── Projects/FreeRTOS/
    │   │   ├── UCI/{DWM3001CDK,nRF52840DK,Type2AB_EVB}/       # [ENTRY][CONFIG] UCI app projects
    │   │   ├── FiRa/{…}/                        # CLI ranging app projects
    │   │   └── Listener/, …                     # other app projects
    │   ├── SDK_BSP/Nordic/SDK_17_1_0/          # Nordic nRF5 SDK (vendored)
    │   └── Src/
    │       ├── Apps/
    │       │   ├── Inc/apps_common.h, driver_app_config.h
    │       │   └── Src/
    │       │       ├── uci/                     # [KEY MODULE] UCI application
    │       │       │   ├── task_uci/task_uci.c  # [ENTRY] UCI task, backend wiring
    │       │       │   ├── uci_parser.{c,h}
    │       │       │   ├── uci_transport/       # [KEY MODULE] raw UCI framing over UART
    │       │       │   └── uwbmac_helper/       # MAC init glue (dw3000)
    │       │       ├── fira/                    # FiRa CLI ranging app (fira_app.c)
    │       │       ├── listener/                # sniffer app
    │       │       ├── reporter/                # UART/USB output routing
    │       │       └── common/                  # app framework, cmd shell, tasks, usb_uart
    │       ├── Boards/Src/
    │       │   ├── DWM3001CDK/Common/custom_board.h    # [CONFIG] UART/LED pins
    │       │   ├── nRF52840DK/Common/custom_board.h    # [CONFIG]
    │       │   └── Type2AB_EVB/…
    │       ├── HAL/Src/nrfx/HAL_uart.c          # [KEY MODULE] UART @115200 8N1
    │       ├── Comm/                            # USB CDC + BLE (niq/qnis/anis) transports
    │       ├── UWB/Inc/{uwb_frames,uwb_translate,uwb_utils}.h  # frame/param helpers
    │       ├── AppConfig/Inc/{appConfig,default_config}.h      # [CONFIG] default params
    │       ├── EventManager/, Logger/, Helpers/ # infra (cJSON, circular_buffer, deca_dbg)
    │       └── OS/OSAL FreeRTOS glue
    └── Tools/
        ├── GUI/                                 # Qorvo UWB Explorer installers
        └── uwb-qorvo-tools/                     # [KEY MODULE] Python UCI host tools
            ├── lib/uwb-uci/uci/                 # UCI codec (fira_msg, qorvo_msg, fira_app…)
            └── scripts/{fira,qorvo,device,utils}/  # run_fira_twr, decode_uci, get/set config…
```

---

## 3. Module / Component Breakdown

### 3.1 UCI application (`Src/Apps/Src/uci`)
- **Purpose:** implement a UCI server so an external host controls ranging over UART.
- **Key functions:** `uci_helper()` (launch task), `uci_open_backends()` (init MAC + UCI +
  FiRa backend), `uci_task()` (RX loop), `uci_tp_read()` (frame packets).
- **Inputs:** raw UCI bytes on UART0. **Outputs:** UCI responses/notifications on UART0.
- **Side effects:** allocates MAC/UCI contexts, drives the DW3000 radio.
- **Consumed by (thesis):** `UciUartLink` / `UciSessionManager` on the ESP32.

### 3.2 UCI transport (`uci/uci_transport`)
- **Purpose:** platform-agnostic framing of the raw UCI byte stream.
- **Key functions:** `uci_tp_read()` (header+length assembly, 100 ms garbage flush),
  `uci_tp_usb_packet_send_ready()` (TX flush to reporter).
- **I/O:** `cc_buff` in ↔ UCI core; bytes out via `reporter_instance.print()`.
- **Consumed by:** every UCI message the ESP32 exchanges.

### 3.3 uwbmac helper (`uci/uwbmac_helper`)
- **Purpose:** initialise the DW3000/QM33 driver for FiRa or MCPS.
- **Key functions:** `uwbmac_helper_init_fira()`, `uwbmac_helper_init_mcps()`, `..._deinit()`.
- **Side effects:** radio power/clock/config.

### 3.4 FiRa UCI backend (`Libs/.../uci_backend/uci_backend_fira.h`)
- **Purpose:** translate ranging-type UCI commands into uwb-stack FiRa-helper calls and **build the
  `RANGE_DATA_NTF`**.
- **Key functions:** `uci_backend_fira_init()`, `uci_backend_fira_set_antenna_conf()`,
  `uci_backend_fira_get_supported_channels()`.
- **Output (thesis-critical):** the ranging notification consumed by `UciSessionManager::onPacket()`.

### 3.5 HAL UART (`HAL/Src/nrfx/HAL_uart.c`)
- **Purpose:** 115200 8N1 UART, no flow control.
- **Key functions:** `deca_uart_init()`, `deca_uart_transmit()`, `deca_uart_receive()`.
- **Consumed by (thesis):** the physical link to `UciUartLink::begin()`.

### 3.6 FiRa CLI app (`Src/Apps/Src/fira`)
- **Purpose:** standalone, self-contained TWR ranging demo controlled by a serial CLI (not UCI).
- **Note:** useful for bench-testing a board without a host, but **not** the integration path.

### 3.7 Reporter / Comm (`Src/Apps/Src/reporter`, `Src/Comm`)
- **Purpose:** route output to UART or USB-CDC (and optionally BLE via `niq`/`qnis`/`anis`).
- **Relevance:** determines whether UCI leaves the board on hardware UART (used) or USB-CDC.

### 3.8 Host tools (`Tools/uwb-qorvo-tools`)
- **Purpose:** Python UCI client (`run_fira_twr`, `decode_uci`, `get/set_config`, `get_device_info`).
- **Relevance:** the `uci/` codec (`qorvo_msg.py`, `fira_msg.py`) is the authoritative reference for
  the `RANGE_DATA_NTF` byte layout used throughout these docs.

---

## 4. Data Structures & Interfaces

### 4.1 `RANGE_DATA_NTF` header (over the wire)
| Field | Type | Notes |
|-------|------|-------|
| sequence_number | uint32 LE | ranging round counter (offset 0) |
| session_handle | uint32 LE | offset 4 |
| ranging_interval | uint32 LE | ms; offset 9 |
| ranging_measurement_type | uint8 | 2 = TWR; offset 13 |
| mac_addressing_mode | uint8 | 0=2-byte, 1=8-byte; offset 15 |
| primary_session_id | uint32 LE | offset 16 |
| num_ranging_measurements | uint8 | **offset 24** |

### 4.2 TWR measurement block (2-byte MAC)
| Field | Type | Notes |
|-------|------|-------|
| mac_address | uint16 | offset 25 |
| status | uint8 | **offset 27** |
| nlos | uint8 | offset 28 |
| distance | uint16 LE (cm) | **offset 29** |
| aoa_azimuth / _fom | Q9.7 / uint8 | offset 31 / 33 |
| aoa_elevation / _fom | Q9.7 / uint8 | offset 34 / 36 |
| rssi | Q7.1 | offset 44 |

### 4.3 `struct uci_backend_fira_context`
Holds `fira_context`, `uwbmac_context`, `antennas`, `uci`, `uci_backend_core_context`, `coord`,
`sess_man`, `session_ops` — see [SDK_API_REFERENCE.md §5](SDK_API_REFERENCE.md).

### 4.4 `struct uci_tp` / `enum uci_if_e`
Transport binding to a UCI server + selected interface (`UCI_NONE`/`UCI_UART0`/`UCI_UART1`).

> Full byte-level tables live in [SDK_UCI_FLOW.md §3](SDK_UCI_FLOW.md).

---

## 5. Algorithms & Core Logic

### 5.1 UCI packet framing (byte layout)
Standard FiRa UCI: 4-byte header `[MT|PBF|GID][OID][RFU][Payload Length]` + payload. Raw bytes,
little-endian, no SLIP, no CRC. Details: [SDK_UCI_FLOW.md §1](SDK_UCI_FLOW.md).

### 5.2 Ranging notification format (CRITICAL)
`RANGE_DATA_NTF` = `GID=0x02 OID=0x00`. **Distance @ `payload[29:30]` (uint16 LE, cm), status @
`payload[27]`, count @ `payload[24]`** — verified against `qorvo_msg.py`. Full map:
[SDK_UCI_FLOW.md §3](SDK_UCI_FLOW.md).

### 5.3 Session state machine
`IDLE → INIT (SESSION_INIT) → IDLE/CONFIG (SET_APP_CONFIG) → ACTIVE (SESSION_START) →
IDLE (SESSION_STOP) → DEINIT (SESSION_DEINIT)`. State changes are reported by `SESSION_STATUS_NTF`.
See [SDK_ARCHITECTURE.md §3](SDK_ARCHITECTURE.md).

### 5.4 Filtering / compensation
The SDK reports **raw** ranging distance. Antenna-delay compensation is applied only if a per-device
calibration is loaded via the `load_cal`/`set_cal` tools (`calib_files/DWM3001CDK/…`). No temperature
or Kalman filtering is performed in the SDK — that is the ESP32's job.

### 5.5 FiRa / CCC parameter sets
`RANGING_ROUND_USAGE` selects SS-TWR / DS-TWR (deferred/non-deferred). `STS_CONFIG` selects static
(0) vs dynamic/provisioned STS. CCC-specific parameters (`HOP_MODE_KEY=0xA0`, `CCC_UWB_TIME0=0xA1`,
`SELECTED_UWB_CONFIG_ID`, `CCC_STS_INDEX=0xA8`) exist for CCC sessions. The thesis uses static-STS
FiRa DS-TWR (`RANGING_ROUND_USAGE=2`, `STS_CONFIG=0`).

---

## 6. Configuration & Constants

| Constant | Default | File | Hardware-dependent? |
|----------|---------|------|---------------------|
| UART baud | 115200 | `HAL_uart.c` | No |
| UART framing | 8N1, no flow control | `HAL_uart.c` | No |
| nRF52840DK UART TX/RX | P0.06 / P0.08 | `nRF52840DK/custom_board.h` (pca10056) | **Yes** |
| DWM3001CDK UART TX/RX | P0.19 / P0.15 | `DWM3001CDK/custom_board.h` | **Yes** |
| UCI header size | 4 bytes | `uci_transport.c` (`UCI_PACKET_HEADER_SIZE`) | No |
| UCI garbage timeout | 100 ms | `uci_transport.c` | No |
| Channel | 5 or 9 (host-set) | `SET_APP_CONFIG` `CHANNEL_NUMBER` | No |
| Preamble code index | host-set (thesis: 9) | `PREAMBLE_CODE_INDEX` | No |
| SFD ID | host-set (thesis: 2) | `SFD_ID` | No |
| STS config | static (0) | `STS_CONFIG` | No |
| Slot duration | host-set (thesis: 2400 RSTU) | `SLOT_DURATION` | No |
| Ranging interval | host-set (thesis: 120 ms) | `RANGING_INTERVAL` | No |
| SoC part (flash) | `nrf52840_xxaa` / `nrf52833_xxaa` | `README.md` | **Yes** |

Antenna delay / channel defaults per board are shipped as calibration JSON in
`Tools/uwb-qorvo-tools/scripts/device/load_cal/calib_files/`.

---

## 7. Hardware / Platform Dependencies

- **RTOS / SDK:** FreeRTOS + Nordic nRF5 SDK **17.1.0** (not Zephyr / nRF Connect SDK). SoftDevice
  **S113** is bundled for the optional BLE transport.
- **Toolchain:** arm-none-eabi-gcc **10.3-2021.10**, CMake ≥ 3.23, Python ≥ 3.10, J-Link, `make`.
- **Flash / RAM:** not fixed in source; the UCI task allocates a 6 KB stack
  (`UCI_TASK_STACK_SIZE_BYTES`) and a 1 KB RX buffer. Total footprint fits nRF52833/nRF52840.
- **GPIO:** UART TX/RX per board (§6). SPI to the DW3000/QM33 radio is defined in the board's
  `platform_l1_config.c` / BSP. Reset and IRQ lines are board-specific.
- **Clock:** the DW3xxx needs its 38.4 MHz reference crystal; the nRF host uses its HFXO.
- **UART vs USB-CDC:** UCI output can be routed to hardware UART (used by the thesis) **or** USB-CDC
  (DWM3001CDK has onboard USB). The reporter selects the interface.

---

## 8. Current Capabilities (what the SDK does TODAY)

| Feature | Status | File | Notes |
|---------|--------|------|-------|
| FiRa UCI 2.0 server over UART | ✅ | `uci/task_uci.c`, `uci_transport.c` | 115200 raw framing |
| SESSION_INIT / DEINIT | ✅ | `uci_spec_fira.h`, fira backend | GID 0x01 |
| SET_APP_CONFIG (full FiRa param set) | ✅ | `uci_spec_fira.h` | TLV params |
| SESSION_START / STOP | ✅ | fira backend | GID 0x02 |
| RANGE_DATA_NTF with distance | ✅ | fira backend, `qorvo_msg.py` | TWR block |
| DS-TWR / SS-TWR ranging | ✅ | `RANGING_ROUND_USAGE` | thesis uses DS-TWR |
| Static-STS | ✅ | `STS_CONFIG=0` | thesis default |
| AoA reporting | 🔄 | `AOA_RESULT_REQ`, backend antenna conf | needs AoA HW |
| CCC ranging | 🔄 | CCC param IDs 0xA0–0xA8 | present, not used by thesis |
| RSSI / diagnostics | 🔄 | `RSSI_REPORTING`, `ENABLE_DIAGNOSTICS` | configurable |
| USB-CDC output | ✅ | `reporter`, `Comm` | alt to UART |
| Host Python tooling | ✅ | `uwb-qorvo-tools` | run_fira_twr, decode_uci |

Detailed checklist: [SDK_FEATURES.md](SDK_FEATURES.md).

---

## 9. Known Limitations & TODOs

- The UCI application is **host-driven only** — it performs no ranging without a host sequencing the
  commands (by design).
- The stock UCI firmware reports **raw distances**; antenna-delay calibration must be flashed
  separately (`set_cal`/`load_cal`). No thesis-specific `+0.24 m` offset is applied on-board
  (see [SDK_INTEGRATION_LINK.md GAP-2](SDK_INTEGRATION_LINK.md)).
- Some backends are **compile-time gated** (`UCI_MAC_BACKEND`, `UCI_FTM_BACKEND`, `UCI_CONF_MANAGER`,
  `UCI_MAC_CALIB_BACKEND`); availability depends on the project build.
- `task_uci.c` contains a `qm_erase_certificates()` stub returning 0 (secure element cert erase not
  implemented in this build).
- Paths containing spaces break the build tools (`README_WARNING.md`).
- The uwb-stack is delivered as **precompiled libraries**; source-level tracing stops at the
  `uci_bundle` / `uwbstack_bundle` headers.

---

## See also
- [SDK_ARCHITECTURE.md](SDK_ARCHITECTURE.md) — system diagrams & data flows
- [SDK_UCI_FLOW.md](SDK_UCI_FLOW.md) — byte-level UCI protocol
- [SDK_API_REFERENCE.md](SDK_API_REFERENCE.md) — function/struct/callback reference
- [SDK_FEATURES.md](SDK_FEATURES.md) — capability checklist
- [SDK_INTEGRATION_LINK.md](SDK_INTEGRATION_LINK.md) — ESP32 ↔ SDK mapping
