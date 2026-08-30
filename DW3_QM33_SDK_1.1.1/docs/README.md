# DW3_QM33_SDK_1.1.1 — Documentation Suite

Documentation for the **DW3_QM33_SDK_1.1.1** UWB SDK, written for the **ESP32 Smart Car Access**
thesis integration. Covers **only current (Stage 1)** SDK capabilities: single-anchor, one-to-one
FiRa TWR ranging driven over UCI/UART.

## Documents

| Doc | What it covers |
|-----|----------------|
| [SDK_CODEBASE_REFERENCE.md](SDK_CODEBASE_REFERENCE.md) | Full onboarding: SDK overview, directory map, modules, data structures, algorithms, config, hardware, capabilities, limitations. |
| [SDK_ARCHITECTURE.md](SDK_ARCHITECTURE.md) | System diagrams (Mermaid), end-to-end ranging data flow, UCI session state machine, module boundaries, concurrency model. |
| [SDK_API_REFERENCE.md](SDK_API_REFERENCE.md) | Public functions, structs and callbacks of the UCI app + FiRa backend; UCI packet & event reference tables. |
| [SDK_FEATURES.md](SDK_FEATURES.md) | Capability checklist (✅/🔄/❌/⚠️) with source-file evidence. |
| [SDK_UCI_FLOW.md](SDK_UCI_FLOW.md) | **Most critical.** Byte-level UCI protocol: frame format, session startup command/response pairs, `RANGE_DATA_NTF` byte map, status codes, `SET_APP_CONFIG` parameter reference, integration checklist. |
| [SDK_INTEGRATION_LINK.md](SDK_INTEGRATION_LINK.md) | Exact mapping between the ESP32 firmware and the SDK anchor — wiring, command timeline, verified parameters, known gaps. |

## Which file to read first

- **Thesis integration (get ranging onto the ESP32):** start with
  [SDK_INTEGRATION_LINK.md](SDK_INTEGRATION_LINK.md), then [SDK_UCI_FLOW.md](SDK_UCI_FLOW.md).
- **UCI protocol debugging (byte offsets, GID/OID, status codes):**
  [SDK_UCI_FLOW.md](SDK_UCI_FLOW.md).
- **Hardware bring-up (flash a board, wire the UART):** [SDK_UCI_FLOW.md §7](SDK_UCI_FLOW.md) and
  [SDK_CODEBASE_REFERENCE.md §1.4 & §7](SDK_CODEBASE_REFERENCE.md).
- **General understanding / new to the SDK:** [SDK_CODEBASE_REFERENCE.md](SDK_CODEBASE_REFERENCE.md)
  then [SDK_ARCHITECTURE.md](SDK_ARCHITECTURE.md).

## Key facts at a glance

- Anchor firmware = SDK **UCI server** app (`SDK/Firmware/Src/Apps/Src/uci`), **host-driven**.
- UART: **115200 8N1, no flow control, raw UCI framing** (no SLIP/CRC).
- Session commands: `GID=0x01` (INIT/DEINIT/SET_APP_CONFIG); ranging: `GID=0x02` (START/STOP + NTF).
- `RANGE_DATA_NTF` (GID=0x02, OID=0x00): **count @ payload[24], status @ payload[27],
  distance @ payload[29:30] (uint16 LE, cm)** — verified correct against the ESP32 firmware.
- Boards: nRF52840DK+DWM3000EVB (`nrf52840_xxaa`, TX P0.06/RX P0.08) and DWM3001CDK
  (`nrf52833_xxaa`, TX P0.19/RX P0.15).

> All byte offsets and parameter IDs in this suite are verified against SDK source
> (`uci_spec_fira.h`, `uci_transport.c`, `qorvo_msg.py`, `HAL_uart.c`, board `custom_board.h`).
