# SDK_FEATURES.md

Feature checklist for **DW3_QM33_SDK_1.1.1** (Stage 1, as consumed by the ESP32 Smart Car Access
thesis).

Status legend: ✅ Confirmed in source · 🔄 Partially implemented / needs config · ❌ Not present in SDK
· ⚠️ Present but untested in thesis context

> A feature is marked ✅ only where a specific source file can be pointed to.

---

## Core UWB Features

| Feature | Status | Files | Notes |
|---------|--------|-------|-------|
| FiRa TWR ranging (Two-Way Ranging) | ✅ | `Libs/.../uci_backend/uci_backend_fira.h`, `uci_spec_fira.h`, `Tools/.../qorvo_msg.py` (`RangingTwrData`) | DS-TWR & SS-TWR via `RANGING_ROUND_USAGE` |
| CCC ranging support | 🔄 | `uci_spec_fira.h` (CCC params 0xA0–0xA8) | Parameter IDs present; not exercised by thesis |
| One-to-one Controller/Controlee ranging | ✅ | `uci_spec_fira.h` (`MULTI_NODE_MODE=0`), fira backend | Thesis default (unicast) |
| One-to-many (multicast) ranging | 🔄 | `uci_spec_fira.h` (`MULTI_NODE_MODE`, `SESSION_UPDATE_CONTROLLER_MULTICAST_LIST=0x07`, `NUMBER_OF_CONTROLEES=0x05`) | Supported by stack; not used in Stage 1 |
| UCI command: SESSION_INIT | ✅ | `uci_spec_fira.h` (`UCI_OID_SESSION_INIT=0x00`, GID **0x01**) | Note: GID is 0x01, not 0x00 |
| UCI command: SET_APP_CONFIG | ✅ | `uci_spec_fira.h` (`UCI_OID_SESSION_SET_APP_CONFIG=0x03`, GID 0x01) | Full FiRa param TLV set |
| UCI command: RANGING_START | ✅ | `uci_spec_fira.h` (`UCI_OID_SESSION_START=0x00`, GID **0x02**) | |
| UCI command: RANGING_STOP | ✅ | `uci_spec_fira.h` (`UCI_OID_SESSION_STOP=0x01`, GID 0x02) | |
| UCI command: SESSION_DEINIT | ✅ | `uci_spec_fira.h` (`UCI_OID_SESSION_DEINIT=0x01`, GID 0x01) | |
| UCI notification: RANGE_DATA_NTF with distance | ✅ | `uci_backend_fira`, `qorvo_msg.py` (`RangingData`) | GID 0x02 / OID 0x00; distance @ payload[29:30] |
| Antenna delay calibration | 🔄 | `Tools/.../scripts/device/{load_cal,set_cal}`, `calib_files/DWM3001CDK/…` | Host-loaded per device; not in stock UCI FW |
| NLOS detection / status reporting | ✅ | `qorvo_msg.py` (`nlos` @ payload[28]), `uci_status_code` | NLOS byte + status codes present |
| Diagnostic frames (CIR, RSSI, metrics) | 🔄 | `uci_spec_fira.h` (`ENABLE_DIAGNOSTICS=0xE8`, `RSSI_REPORTING=0x13`), `RANGE_DIAGNOSTICS_NTF` in `qorvo_msg.py` | Config-gated |

## OOB / Session Configuration Features

| Feature | Status | Files | Notes |
|---------|--------|-------|-------|
| BLE OOB payload for session parameter exchange | ❌ | — | No OOB session-config module in the UCI app; params set in-band via `SET_APP_CONFIG`. (OOB struct exists only in ESP32 firmware.) |
| Static STS configuration | ✅ | `uci_spec_fira.h` (`STS_CONFIG=0x02` → 0), `STATIC_STS_IV=0x28`, `VENDOR_ID=0x27` | Thesis default |
| Dynamic STS / HKDF-derived STS | 🔄 | `uci_spec_fira.h` (`STS_CONFIG` dynamic, `SESSION_KEY=0x45`, `uci_backend_mctt_dynsts.h`) | Present; unused by thesis |
| Hopping mode | ✅ | `uci_spec_fira.h` (`HOPPING_MODE=0x2C`) | Thesis sets 1 |
| AOA result reporting (Angle of Arrival) | 🔄 | `uci_spec_fira.h` (`AOA_RESULT_REQ=0x0D`), `uci_backend_fira_set_antenna_conf()` | Needs AoA-capable HW |

## Hardware Targets

| Feature | Status | Files | Notes |
|---------|--------|-------|-------|
| nRF52840DK + DWM3000EVB: build target present | ✅ | `Projects/FreeRTOS/UCI/nRF52840DK`, `Src/Boards/Src/nRF52840DK` | flash `nrf52840_xxaa` |
| DWM3001CDK: build target present | ✅ | `Projects/FreeRTOS/UCI/DWM3001CDK`, `Src/Boards/Src/DWM3001CDK` | flash `nrf52833_xxaa` |
| Type2AB_EVB: build target present | ✅ | `Projects/FreeRTOS/UCI/Type2AB_EVB` | not used by thesis |
| UART output of UCI data at 115200 baud | ✅ | `HAL_uart.c` (`Baud115200`) | 8N1, no flow control |
| USB-CDC output of UCI data | ✅ | `Src/Comm/InterfUsb.c`, `reporter/usb_reporter` | DWM3001CDK onboard USB |
| Prebuilt binaries | ✅ | `SDK/Binaries/{DWM3001CDK,nRF52840DK,Type2AB_EVB}` | ready-to-flash `.hex` |

## Tooling & Testing

| Feature | Status | Files | Notes |
|---------|--------|-------|-------|
| Example application: UCI server | ✅ | `Src/Apps/Src/uci` | thesis integration path |
| Example application: FiRa CLI ranging | ✅ | `Src/Apps/Src/fira` (`fira_app.c`) | standalone, serial CLI |
| Example application: Listener (sniffer) | ✅ | `Src/Apps/Src/listener` | passive frame capture |
| Example application: Reporter | ✅ | `Src/Apps/Src/reporter` | output routing |
| Python host tool: run FiRa TWR | ✅ | `Tools/.../scripts/fira/run_fira_twr/run_fira_twr.py` | drives ranging from PC |
| Python host tool: decode UCI | ✅ | `Tools/.../scripts/utils/decode_uci/decode_uci.py` | UCI packet decoder |
| Python host tools: get/set config, device info, cal | ✅ | `Tools/.../scripts/device/*` | provisioning/diagnostics |
| Qorvo UWB Explorer GUI | ✅ | `Tools/GUI/{Windows,Linux,macOS}` | desktop ranging GUI |
| Logging / debug output tags | ✅ | `Src/Logger/log_processing.c`, `Helpers/deca_dbg.c`, `qlog` | `QLOGE/QLOGW` |
| CI / on-target unit tests | ⚠️ | `Drivers/API/Src/tests` | driver tests only; not thesis-verified |

---

## See also
- [SDK_CODEBASE_REFERENCE.md](SDK_CODEBASE_REFERENCE.md) · [SDK_UCI_FLOW.md](SDK_UCI_FLOW.md) ·
  [SDK_INTEGRATION_LINK.md](SDK_INTEGRATION_LINK.md)
