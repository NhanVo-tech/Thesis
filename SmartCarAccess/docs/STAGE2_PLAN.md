# STAGE2_PLAN.md

> Engineering plan to evolve **Smart Car Access** from Stage 1 (single-point 1‑D UWB ranging + LSTM relay detection) to Stage 2 (**multi-anchor 2‑D localization** with trilateration, EKF, CNN‑LSTM intent recognition, and geofenced actuation).
> This plan is grounded in the Stage 1 codebase as documented in [CODEBASE_REFERENCE.md](CODEBASE_REFERENCE.md), [ARCHITECTURE.md](ARCHITECTURE.md), [FEATURES.md](FEATURES.md), and [DATA_CONTRACTS.md](DATA_CONTRACTS.md).

---

## 0.0 Status update (2026-09-02)

Milestones **M3 (trilateration)** and **M4 (EKF)** are now **DONE on the ESP32**, not just in
simulation. The firmware pipeline is live end-to-end:

```
PC bridge (run_fira_bridge.py) --RANGE:d0,d1,d2,valid--> UwbBridge
   -> Trilateration::solve (2-circle / Gauss-Newton, iot/src/uwb/trilateration.cpp)
   -> Ekf::update (2-D constant-velocity KF [x,y,vx,vy], iot/src/uwb/ekf_stub.cpp)
   -> AccessController::handlePosition(x,y,vx,vy) geometric zone unlock (GPIO26)
```

Also changed since the sections below were written:
- **Ranging start/stop is now driven by the phone** via CCC tunnel instructions `0x84`/`0x85`
  (`ble_auth.cpp`), which call `UwbBridge::sendStart/sendStop` → `CMD:START/STOP_RANGING`. It is
  **no longer** tied to FSM secure-channel enter/exit hooks.
- **AccessController gained a radial-velocity approach gate** (rejects users moving away/passing
  by) in addition to the distance hysteresis.
- The **1-D LSTM remains dormant** (present but not called); the CNN-LSTM intent classifier
  (**M5/M6**) and geofenced multi-zone actuation (**M7/M8**) are the next open milestones.

The remainder of this document is the original Stage 2 plan and is kept for roadmap context;
treat §0.1–§0.3 milestone statuses through the lens of this update.

---

## 0. Current Progress (2026-08-09)

### 0.1 Simulation UI for UWB Trilateration — COMPLETED

A dedicated simulation and visualization project (`thesis-ui/`) has been built to **validate the trilateration algorithm (M3) offline** before porting to C++ on the ESP32.

**Location**: `C:\thesis-ui\`

**Key files**:

| File | Purpose |
|------|---------|
| `scripts/uwb_trilateration.py` | Core trilateration module: Gauss-Newton iterative solver, per-anchor Kalman filters, zone classifier, majority-vote debounce |
| `scripts/simulation_pipeline.py` | Full pipeline: random trajectory generator, UWB noise model, orchestrator |
| `ui-demo/main_screen.py` | NiceGUI 3D visualization: car model, zone rings, live phone/target markers, telemetry HUD |
| `ui-demo/three_script.py` | Three.js 3D scene: VinFast Lux GLB model, zone rings, phone marker, camera controls |

**Algorithm**: Gauss-Newton iterative least-squares trilateration (ported from Android Kotlin UWB code), 3 anchors, seeded with previous estimate to avoid wrong-intersection ambiguity.

**3-anchor layout** (car frame, metres):

| Anchor | X | Y | Location |
|--------|---|---|----------|
| A | 0.00 | 2.00 | Front centre |
| B | -0.85 | 0.00 | Left B-pillar |
| C | 0.00 | -2.00 | Rear centre |

**Pipeline**: `raw dist[3] → Kalman1D ×3 → Gauss-Newton trilateration → zone classification → debounce → UwbResult(x, y, zone, rms, valid)`

**Noise model**: UWB noise σ = 0.08 + 0.010 × distance [m], 1% outlier probability.

**Performance** (simulation, 1200 frames, 2 laps):
- Mean RMS error: ~0.039 m
- Max RMS error: ~0.859 m
- 99.7% frames have RMS < 0.2 m

**UI features**:
- 4 camera views (OVERVIEW, FRONT, SIDE, TOP)
- Manual zone-jump buttons (OUT, WELCOME, NEAR, UNLOCK)
- ALGO SIM button: random 360° approach/retreat trajectory
- Left HUD: 3 UWB distance readings + RMS bar + valid/invalid indicator
- Right HUD: true/estimated (x,y) position + delta + distance error + lap/phase counter
- Red dot (true position) vs blue phone (estimated position) on 3D canvas

**Self-tests**: `python scripts/uwb_trilateration.py` — all 7 test categories pass (config validation, input validation, zero-noise zones, noisy stability, Gauss-Newton convergence).

**Dependencies**: `nicegui>=1.4.0` only.

**How to run**:
```bash
pip install -r requirements.txt
cd C:\thesis-ui
python ui-demo\main_screen.py
# Open http://localhost:8080
```

**Next step — M3 C++ port**: The Gauss-Newton algorithm from `uwb_trilateration.py` must be ported to C++ (`iot/include/uwb/trilateration.h` + `iot/src/uwb/trilateration.cpp`). The Python reference implementation serves as the golden model for cross-validation.

---

### 0.2 UWB Data Collection Transport — DECISION (2026-08-30)

**Final decision for multi-anchor UWB data collection:**

- Collect ranging from **3 anchors with a single Python script running on the PC**, talking **UART/USB directly** to each anchor. **No RS485, no ESP-NOW.**
- The **`run_fira_multianchor.py`** script (in the DW3 QM33 SDK tools) opens 3 COM ports in a single process, collects `d0/d1/d2`, and runs **Gauss-Newton trilateration on the PC** to produce `(x, y)`.
- This is the platform for validating the algorithm (M3) with real data before porting to embedded.

**Architecture impact:**

| Layer | Before (Stage 1.5) | After (final) |
|---|---|---|
| Data collection | `3 anchors → UART → 3× ESP32-C3 → ESP-NOW → ESP32-S3` | `3 anchors → USB/UART → PC (Python)` |
| Trilateration | C++ on ESP32 | Python on PC (golden model) |
| C3 bridges / ESP-NOW | Required | **Removed** (for UWB collection) |

> R2 (UART multiplexing / ESP-NOW) and R1 (one-to-many interop) are now practically resolved by the direct Python script — see §5.

---

### 0.3 Cleanup — single-ranging/UCI/OOB removed (2026-09-01)

Multi-anchor ranging over USB/USB-CDC is now validated end-to-end and the Stage 1 single-anchor pipeline has been **deleted** from both firmware and app:

**Current working pipeline:**

```
Phone (android.ranging multicast DS-TWR, controller 0x06C1)
      │  UWB PHY → 3 anchors (responders, mac 0/1/2)
      ▼
Anchor0/1/2 ──USB/UART──▶ PC (run_fira_bridge.py)
      │  collects d0,d1,d2 → sends "RANGE:d0=..,d1=..,d2=..,valid=.." over USB-CDC
      ▼
ESP32-S3 (UwbBridge) ── parse RANGE → trilateration → EKF stub → AccessController (2D unlock, GPIO26)
      ▲
      └── sends "CMD:START_RANGING"/"CMD:STOP_RANGING" on FSM secure-channel enter/exit
```

**Deleted (superseded by the PC-bridge + multicast path):**

| Area | Removed files |
|------|---------------|
| UCI session / UART link / OOB | `iot/{include,src}/uwb/uci_session_manager.*`, `uci_uart_link.*`, `uci_oob.*` |
| 1-D Kalman filter | `iot/lib/Kalman/` |
| ESP-NOW bridge / host bridge | `anchor_bridge.*`, `espnow_bridge.*`, `uci_host_bridge.*`, `oob_parser.*`, `session_config.*` (removed earlier) |
| Phone unicast / OOB | `uwb_service.dart`, `UwbRangingBridge.kt` (multicast `UwbMulticastBridge` + `UwbMultiService` + `UwbRangingSection` remain) |
| BLE admin UWB stubs | `0x50/0x51` commands and UWB OOB characteristic (`0005`) removed from `ble_admin.cpp` |

**Repurposed (not deleted):**

| Area | Change |
|------|--------|
| Single-anchor door unlock | `uci_door_unlock.*` → **`access_controller.*`**: renamed and converted from 1-D `handleRangingDistance(d)` to 2-D `handlePosition(x, y)` (Euclidean distance to the unlock point + same hysteresis). Wired into `UwbBridge::tick()` after trilateration. |
| Single-point LSTM | `lstm_inference.*` + `uwb_lstm_model.h` restored as-is (kept untouched for the future AI/CNN-LSTM milestone M5/M6). |

The multiranging path (M0–M4 transport, trilateration, EKF stub) is untouched. Embedded port of the EKF/CNN-LSTM remains a later milestone per §4.

---

## 1. Stage 2 Goals

By the end of Stage 2 the system must achieve the following (mapped 1:1 to the four thesis requirements):

- **G1 — Multi-anchor UWB ranging.** Collect simultaneous ranging distances from **3 UWB anchors** to the phone (FiRa one-to-many Initiator), aggregate them on the ESP32 Master in real time, with per-anchor timestamps and freshness tracking, coordinated by FreeRTOS.
- **G2 — Trilateration + EKF.** Convert the 3 synchronized distances into a planar `(x, y)` position via trilateration, then run an **Extended Kalman Filter** that fuses the position measurements to suppress NLOS/multipath spikes and estimate velocity `(vx, vy)`.
- **G3 — TinyML intent recognition.** Train a **Hybrid CNN‑LSTM** model on `(x, y, vx, vy)` trajectory windows and deploy it via **TensorFlow Lite for Microcontrollers** on the ESP32 to classify behavior: **approaching / passing-by / anomaly** (extensible to more classes).
- **G4 — Geofencing & control.** Classify the fused position into virtual zones (**Welcome Zone, Driver Door, Trunk Zone, Inside Cabin**) and, gated by the existing cryptographic session and the intent classifier, drive **relay outputs** (door unlock, trunk kick, engine-start authorization).

**Non-goals for Stage 2:** replacing NFC Phase A / BLE Phase B security, changing the CCC mailbox trust model, or moving immobilizer secrets off-device. Those remain exactly as in Stage 1 (see §7).

---

## 2. Hardware Inventory & Role Assignment

### 2.1 Current Architecture (Stage 1.5 — ESP32-C3 Bridge, 2026-08-08)

| Component | Stage 1.5 Role | Comm. interface | Notes |
|-----------|---------------|-----------------|-------|
| **ESP32-S3** (`yolo_uno`) | **Master**: BLE (OOB from phone), FSM, LSTM inference, ESP-NOW to bridges | BLE to phone; ESP-NOW to C3 bridges; GPIO to relays | No longer talks UCI directly to anchors |
| **ESP32-C3** (x1-3, `thesis253_workspace`) | **UCI Bridge**: receives StartMsg via ESP-NOW, runs UCI session with nRF52840DK, sends RangingMsg back via ESP-NOW | ESP-NOW to Master; UART (GPIO20/21) to nRF52840DK | One C3 per anchor; ANCHOR_ID=0/1/2 |
| **Anchor 0** — nRF52840DK + DWM3000EVB | UWB **Responder**, DK UCI server firmware | UART → ESP32-C3 Bridge | Reused from Stage 1 |
| **Anchor 1** — DWM3001CDK | UWB **Responder** (planned for Stage 2) | UART → ESP32-C3 Bridge #1 | New node |
| **Anchor 2** — DWM3001CDK | UWB **Responder** (planned for Stage 2) | UART → ESP32-C3 Bridge #2 | New node |
| **Android phone** | UWB **Initiator** + BLE OOB sender | BLE → Master; UWB PHY → anchors | Single-anchor one-to-one (tested working) |

### 2.2 Stage 2 Hardware (Target)

| Component | Stage 2 Role | Comm. interface | Change from Stage 1.5 |
|-----------|--------------|-----------------|----------------------|
| **PC (Python)** | **Data collection + fusion node**: one script opens 3 COM ports, collects `d0/d1/d2`, runs trilateration → `(x,y)`. Serves as the "golden model" for M3 validation. | USB/UART directly to 3 anchors | **New** — replaces ESP32-C3 bridges + ESP-NOW |
| **Anchor 0** — nRF52840DK + DWM3000EVB | UWB **Responder** at coordinate `A0` (mac 0) | USB/UART → PC | **Reused**; one-to-many FiRa confirmed |
| **Anchor 1** — DWM3001CDK | UWB **Responder** at coordinate `A1` (mac 1) | USB/UART → PC | **New** node |
| **Anchor 2** — DWM3001CDK | UWB **Responder** at coordinate `A2` (mac 2) | USB/UART → PC | **New** node |
| **Android phone** | UWB **Initiator** (FiRa one-to-many): ranges all 3 anchors in one round | UWB PHY → anchors | **Extended**: 3-anchor config (already working) |
| **ESP32-S3** (`yolo_uno`) | Hosts NFC + BLE + FSM + relays. **No longer does UWB fusion** — UWB now runs on the PC; embedded port is a later milestone. | BLE → phone; GPIO → relays | **Simplified**: drops multi-anchor aggregation + ESP-NOW |
| **ESP32-C3** (×3) | ~~UCI Bridge~~ | — | **Removed** |
| **Relays** | Door unlock (GPIO26), trunk kick (new), engine-start auth (new) | GPIO from ESP32-S3 | **Extended** from single relay to relay bank |

> **Transport finalized (§0.2)**: collect UWB with **one Python script + direct USB/UART** (no RS485, no ESP-NOW). See updated §5 Risk R2.

---

## 3. System Architecture Delta

### 3.1 What changes vs. Stage 1
- **Data-collection layer**: Stage 1 had a single UWB source (`DW3000 → UciSessionManager.onPacket()`) producing one scalar distance. Stage 2 introduces **3 concurrent distance streams** that must be time-aligned into a single "ranging frame" `{d0, d1, d2, t}`.
- **Processing layer**: Stage 1 filtered one distance with a **1‑D Kalman** (`iot/lib/Kalman/Kalman.h`). Stage 2 replaces the scalar filter path with **trilateration + a 2‑D EKF** producing a full state `(x, y, vx, vy)`.
- **Intelligence layer**: Stage 1 fed `[distance, residual, velocity]` into `LstmInference`. Stage 2 feeds **trajectory windows of `(x, y, vx, vy)`** into a **CNN‑LSTM** and adds a **geofencing** classifier on the fused position.
- **Actuation layer**: Stage 1 fired a single door relay from `UwbDoorUnlock`. Stage 2 introduces a **zone→relay decision table** driving multiple actuators, still gated by BLE session validity.

The NFC/BLE/CCC/FSM security spine is **unchanged**; Stage 2 is a new pipeline hanging off the UWB task.

### 3.2 Layered ASCII diagram

```
                         ┌──────────────────────── DATA COLLECTION LAYER ───────────────────────┐
 Phone (FiRa Initiator)  │   Anchor0 (nRF52840+DWM3000EVB)   Anchor1 (DWM3001CDK)   Anchor2      │
      one-to-many        │            │ d0                        │ d1                 │ d2       │
   ranging (UWB PHY) ───▶│            ▼                           ▼                    ▼          │
                         │        USB/UART ─────────▶  PC (Python): run_fira_multianchor.py      │
                         │                     collect d0,d1,d2 → trilateration → (x,y)         │
                         └──────────────────────────────────┬───────────────────────────────────┘
                                                             ▼
                         ┌──────────────────────── PROCESSING LAYER ───────────────────────────┐
                         │   Trilateration (Python, validated) → (x_meas, y_meas)                 │
                         │            │                                                          │
                         │            ▼                                                          │
                         │   EKF  state=(x,y,vx,vy)  → suppress NLOS/multipath, estimate vel     │
                         └──────────────────────────────────┬───────────────────────────────────┘
                                                             ▼
                         ┌──────────────────────── INTELLIGENCE LAYER ─────────────────────────┐
                         │   TinyML CNN-LSTM (TFLM): window[(x,y,vx,vy) × N] → intent class      │
                         │        {approaching | passing | anomaly} + confidence                │
                         │            │                                                          │
                         │            ▼                                                          │
                         │   Geofencing: (x,y) → zone {Welcome | DriverDoor | Trunk | Cabin}     │
                         └──────────────────────────────────┬───────────────────────────────────┘
                                                             ▼
                         ┌──────────────────────── ACTUATION LAYER ────────────────────────────┐
                         │   Decision gate: BLE session valid? AND intent==approaching?          │
                         │            │                                                          │
                         │            ▼                                                          │
                         │   Zone→Relay table:  DriverDoor→unlock  Trunk→trunk-kick              │
                         │                       Welcome→prime/light  Cabin→engine-start auth     │
                         └───────────────────────────────────────────────────────────────────────┘
```

---

## 4. Implementation Milestones

Milestones are strictly ordered by dependency. Each proves out one layer before the next consumes it. Complexity is Low / Medium / High.

### M0 — Anchor geometry & one-to-many bring-up ✅ (done via Python)
- **Goal**: Confirm one-to-many FiRa works with 3 mixed anchors; define and freeze the `(x,y)` coordinate of each anchor.
- **Dependencies**: None.
- **Status**: ✅ **Done.** `run_fira_multianchor.py` runs 3 anchors (nRF52840DK+DWM3000EVB + 2× DWM3001CDK) with the phone as Initiator, collecting `d0/d1/d2`.
- **Deliverable**: `d0/d1/d2` + `(x,y)` log in the terminal; anchor coordinates configured via the `--anchor` flag.
- **Files**: `run_fira_multianchor.py` (in the DW3 QM33 SDK tools); anchor coordinates in §0.1 / CLI flags.

### M1 — DWM3001CDK anchor firmware ✅ (done, no bridge needed)
- **Goal**: Bring the 2 DWM3001CDK anchors online as UWB responders by flashing the UCI firmware (DW3000).
- **Dependencies**: M0 (frame format + anchor ID convention frozen).
- **Status**: ✅ **Done.** Flash `DWM3001CDK-UCI-FreeRTOS.hex` (or build from source), run in `run_fira_multianchor.py`.
- **Deliverable**: each anchor produces a distance visible in the terminal, tagged with its anchor index.

### M2 — Anchor aggregation into synchronized RangingFrame ✅ (done in Python)
- **Goal**: Collect `d0, d1, d2` simultaneously into one time frame.
- **Dependencies**: M1 (all 3 distances arriving).
- **Status**: ✅ **Done.** `run_fira_multianchor.py` stores each anchor's distance (thread-safe via `AnchorState`); the main loop reads all 3 each cycle to run trilateration. Missing anchors are shown as `d=---` instead of being silently dropped.
- **Deliverable**: terminal prints `d0/d1/d2 => (x,y)` with a timestamp.

### M3 — Trilateration (distances → x,y)
- **Goal**: Compute a planar position `(x_meas, y_meas)` from a `RangingFrame` and the frozen anchor coordinates.
- **Dependencies**: M2 (synchronized frames).
- **Deliverable**: Log `[POS2D],t,x,y,residual` for a walked reference path; validated against tape-measured ground truth within an agreed tolerance.
- **Status**: ✅ **Algorithm validated in Python with real data as well** — `run_fira_multianchor.py` runs Gauss-Newton directly on real `d0/d1/d2` (besides the offline simulation `thesis-ui/scripts/uwb_trilateration.py`). The C++ port remains a later step.
- **Files to create/modify**:
  - New `iot/include/uwb/trilateration.h` + `iot/src/uwb/trilateration.cpp` (Gauss-Newton / linearized 3-circle solve; return position + RMS residual). Port from Python reference in `thesis-ui/scripts/uwb_trilateration.py`.
  - Reuse `iot/include/uwb/anchor_geometry.h` from M0.
- **Complexity**: Medium (reduced — algorithm already validated offline).

### M4 — Extended Kalman Filter (2‑D position + velocity)
- **Goal**: Replace the raw trilateration output with an EKF that fuses noisy positions, rejects NLOS spikes, and outputs `(x, y, vx, vy)`.
- **Dependencies**: M3 (position measurements to feed the filter).
- **Deliverable**: Log `[EKF],t,x,y,vx,vy` showing visibly smoothed trajectory with spike rejection vs. raw `[POS2D]`; side-by-side plot via an extended visualizer.
- **Files to create/modify**:
  - New `iot/include/uwb/ekf_tracker.h` + `iot/src/uwb/ekf_tracker.cpp` (constant-velocity model; predict/update; NLOS gating via innovation threshold). The scalar `Kalman` from Stage 1 was removed in the 2026-09-01 cleanup.
  - Extend `iot/tools/realtime_lstm_visualizer.py` (or a new `realtime_traj_visualizer.py`) to plot 2‑D trajectory + velocity.
- **Complexity**: High.

### M5 — Trajectory dataset collection + CNN‑LSTM training (offline)
- **Goal**: Capture labeled `(x,y,vx,vy)` trajectory windows for **approaching / passing / anomaly**, train a Hybrid CNN‑LSTM, and export a TFLite Micro flatbuffer.
- **Dependencies**: M4 (EKF output is the feature source).
- **Deliverable**: A quantized `.tflite` model + generated C header `iot/include/uwb/uwb_traj_model.h`; offline accuracy/confusion report. Datasets stored alongside Stage 1 CSVs (`iot/uwb_traj_data_label{0,1,2}.csv`).
- **Files to create/modify**:
  - New Python training pipeline under `iot/tools/` (e.g. `train_traj_cnn_lstm.py`), reusing the `[EKF]`/CSV logging convention from Stage 1 tools (`serial_csv_logger.py`).
  - New `iot/include/uwb/uwb_traj_model.h` (generated C header from the `.tflite` flatbuffer).
- **Complexity**: High.

### M6 — On-device CNN‑LSTM inference (TFLM)
- **Goal**: Run the trained model on the ESP32 over a sliding window of EKF states and output intent + confidence.
- **Dependencies**: M5 (model header) and M4 (live feature source).
- **Deliverable**: Log `[INTENT],t,approach,pass,anomaly` at the ranging cadence; warm-up handling analogous to Stage 1.
- **Files to create/modify**:
  - New `iot/include/uwb/traj_inference.h` + `iot/src/uwb/traj_inference.cpp` (sliding window, z-score scaler, `MicroInterpreter`), modeled on the restored Stage 1 `LstmInference` in `iot/src/uwb/lstm_inference.cpp`. Update `TIME_STEPS`/`NUM_FEATURES` for the 4-feature trajectory.
  - Modify `iot/platformio.ini` if the TFLM arena/`build_flags` must grow (see R3). `TensorFlowLite_ESP32` is already present for the restored LSTM.
- **Complexity**: High.

### M7 — Geofencing zone classifier
- **Goal**: Map the fused `(x, y)` to virtual zones with hysteresis to prevent boundary chatter.
- **Dependencies**: M4 (position). Can proceed in parallel-conceptually with M6 but is listed after because the actuation gate needs both.
- **Deliverable**: Log `[ZONE],t,zoneName`; demonstrated stable zone transitions along a reference walk.
- **Files to create/modify**:
  - New `iot/include/uwb/geofence.h` + `iot/src/uwb/geofence.cpp` (zone polygon/rectangle definitions + hysteresis; the Stage 1 `uci_door_unlock.cpp` hysteresis reference was removed in the cleanup).
- **Complexity**: Medium.

### M8 — Actuation gate + relay bank
- **Goal**: Combine **BLE session validity** (`BLEAuth::isSessionReady()` / `BLEMod::isSessionReady()`), **intent class**, and **zone** into a decision that drives the correct relay.
- **Dependencies**: M6 (intent) and M7 (zone).
- **Deliverable**: End-to-end demo: authenticated phone walking into Driver-Door zone while classified "approaching" fires the unlock relay; trunk zone fires trunk-kick; anomaly/no-session fires nothing.
- **Files to create/modify**:
  - ~~New~~ `iot/include/uwb/access_controller.h` + `iot/src/uwb/access_controller.cpp` — **created** (2026-09-01). The Stage 1 1D `uci_door_unlock.*` was renamed to `AccessController` and converted to 2D: `handlePosition(x, y)` computes the Euclidean distance to the unlock point (`kUnlockPointX/Y`) and applies the same hysteresis + consecutive-hit debounce before firing `RELAY_PIN=26`. Remaining work: add `handleZoneIntent(zone, intent, conf)` (intent gate + relay bank) once M6/M7 land, and keep the door relay behind the BLE session-validity gate.
  - Optionally add FSM states/events in `iot/include/fsm/fsm_states.h` (e.g. `UNLOCKING_CHECK_PROXIMITY` already exists) and bridges in `iot/include/fsm/fsm_integration.h` to keep orchestration centralized.
- **Complexity**: Medium.

### M9 — Phone-side one-to-many Initiator ✅ (done; OOB removed)
- **Goal**: Make the Android app configure and drive a FiRa **one-to-many** session against all 3 anchors.
- **Dependencies**: M0/M1 (anchor MACs + session params known).
- **Status**: ✅ **Done.** The app uses `android.ranging` multicast DS-TWR (`UwbMulticastBridge.kt` + `UwbMultiService.dart` + `UwbRangingSection.dart`), phone controller address pinned to `0x06C1`, ranging anchors `0/1/2`. The BLE OOB path (37-byte payload over Admin `0005`) was **removed** in the cleanup — the PC bridge drives the anchors directly over USB/UART.
- **Deliverable**: App `test_uwb.dart` screen shows 3 simultaneous distances with a start/stop button and live log.
- **Complexity**: ~~High~~ Done.

### M10 — System integration, tuning & evaluation
- **Goal**: Tune EKF noise, geofence boundaries, intent thresholds, GDOP-driven anchor placement; produce evaluation figures.
- **Dependencies**: M8 + M9.
- **Deliverable**: Reproducible scenarios (approach / pass-by / anomaly / trunk) with exported plots, position-accuracy metrics, and false-actuation rate.
- **Files to create/modify**: tuning constants across the new modules; extended Python visualizer; evaluation notes in `docs/`.
- **Complexity**: High.

---

## 5. Critical Unknowns & Risks

### R1 — FiRa one-to-many interop between DWM3000EVB and DWM3001CDK
- **Status**: ✅ **Resolved (tested in practice).** 3 mixed anchors (nRF52840DK + DWM3000EVB + 2× DWM3001CDK) run one-to-many with the phone as Initiator successfully using `run_fira_multianchor.py`, with a common parameter set: channel 9, preamble 9, session 42, multicast DS-TWR, static STS (vendor 0x0708).
- **Note**: both boards use the **DW3000** chip (the nRF52840DK must be built with `USE_DRV_DW3000 1`, not the default DW3720).

### R2 — UART transport for multi-anchor (RESOLVED — PC + direct USB)
- **Status**: ✅ **Resolved (finalized in §0.2).** Collect 3-anchor data with **one Python script on the PC, talking USB/UART directly** (no RS485, no ESP-NOW). Each anchor = one COM port; the script opens 3 COM ports concurrently in one process.
- **USB note**: USB has CRC + retry, so it does not silently lose payload (unlike RS485). Cables ≤5 m per segment; use a powered hub / active cable for longer runs.
- **Embedded port (later)**: porting to ESP32 will require re-solving the transport (UART hub / mux, or reverting to bridges) — but this is no longer a blocker for validating the Stage 2 algorithm.

### R3 — TFLite Micro memory constraints on ESP32
- **Description**: The Stage 1 LSTM (`LstmInference`, `uwb_lstm_model.h`, `TensorFlowLite_ESP32`) is **kept as-is** for now and will be worked on after the system is feature-complete (M5/M6). Adding a **CNN‑LSTM** with a larger tensor arena, plus EKF matrices, plus NimBLE, may exceed RAM. The `uwbTask` already uses a 20 KB stack.
- **Worst case**: Model fails to allocate its arena or the system OOM-crashes at runtime.
- **Mitigation**: Quantize to int8; keep the trajectory window small (tune `TIME_STEPS`); size the arena empirically and pin it as a `constexpr`; consider replacing the Stage 1 3-class LSTM rather than running both models simultaneously; monitor heap via existing serial logging; if needed raise `uwbTask` stack and use PSRAM on the S3. Adjust `iot/platformio.ini` `build_flags`.

### R4 — GDOP geometry for anchor placement
- **Description**: Geometric Dilution of Precision: poorly placed / near-collinear anchors amplify ranging noise into large `(x,y)` error, especially far from the anchor triangle.
- **Worst case**: Position is unusable in exactly the zones that matter (door/trunk), causing false or missed actuations.
- **Mitigation**: Choose a non-collinear triangle enclosing the operating area; compute expected GDOP for candidate layouts before mounting; feed the trilateration residual into the EKF measurement covariance so high-GDOP frames are trusted less; document chosen coordinates in `anchor_geometry.h`. Validate accuracy per-zone in M10.

### R5 (secondary) — Frame time-alignment / clock skew
- **Description**: If anchors are sampled round-robin (R1 fallback), `d0/d1/d2` are not truly simultaneous; a moving target introduces position error proportional to velocity × skew.
- **Worst case**: Systematic trajectory distortion that confuses the intent classifier.
- **Mitigation**: Bound the per-frame acquisition window in `AnchorAggregator`; include timestamp deltas in the EKF; train the CNN‑LSTM on data captured with the same skew profile.

---

## 6. Data Flow Specification

### 6.1 Anchor → PC (Python)
- **Format**: Each anchor sends `RANGE_DATA_NTF` (UCI). The Python script decodes it with the `RangingData` class (from the `uwb-uci` library) → `meas.distance` (cm) + `meas.status`. Anchors are distinguished by **COM port / index** (not by MAC, since all 3 anchors report the phone 0x06C1 as the peer).
- **Frequency**: One record per anchor per ranging round (~120–200 ms, driven by the phone Initiator).
- **Transport**: Direct USB/UART (one COM port per anchor), handled concurrently by `run_fira_multianchor.py` (3 serial reader threads).

### 6.2 Master → Trilateration (RangingFrame)
```c
struct RangingFrame {
  uint32_t t_ms;        // aggregation timestamp (millis)
  double   d[3];        // anchor distances in meters (antenna-offset corrected)
  uint8_t  valid_mask;  // bit i set => d[i] fresh & in-bounds
};
```
- Distances are antenna-offset corrected (reuse `kAntennaOffsetM = 0.24 m`) and sanity-bounded (`-1..30 m`) exactly as Stage 1 before entering trilateration. A frame is solvable only when `valid_mask == 0b111`.

### 6.3 Trilateration → EKF (measurement)
- **Input to EKF update**: `z = (x_meas, y_meas)` in meters (planar), plus a scalar geometric `residual`/GDOP hint used to scale measurement covariance `R`.

### 6.4 EKF state vector
```
x_state = [ x, y, vx, vy ]^T      // position (m), velocity (m/s)
Model:   constant-velocity, dt from RangingFrame.t_ms deltas
Predict: x_k = F(dt) x_{k-1};  P = F P F^T + Q
Update:  innovation gating rejects NLOS spikes (|y - Hx| > k·σ ⇒ down-weight/skip)
Output:  (x, y, vx, vy)
```

### 6.5 EKF → TinyML (feature vector)
- **Per-frame feature**: `f_t = [x, y, vx, vy]` (`NUM_FEATURES = 4`).
- **Window**: last `TIME_STEPS` frames → tensor `window[TIME_STEPS][4]`, z-score normalized with a scaler exported from training (mirror the `scaler_mean` / `scaler_scale` pattern in `iot/include/uwb/lstm_inference.h`).

### 6.6 TinyML → Geofencing (output)
- **Classes**: `{ p_approach, p_pass, p_anomaly }` softmax (extensible).
- **Confidence threshold (initial, tunable)**: act only if `p_approach ≥ 0.80`; treat `p_anomaly ≥ 0.70` as a hard block — mirroring the Stage 1 gate values in `uci_door_unlock.cpp` (`p_walk > 0.80`, `p_attack > 0.70`).

### 6.7 Geofencing → Relay (decision mapping)

| Zone | Required intent | Session gate | Relay / action | Notes |
|------|-----------------|--------------|----------------|-------|
| **Welcome Zone** | approaching | session ready | Welcome lights / prime (optional GPIO) | No unlock; comfort/feedback only |
| **Driver Door** | approaching (`p_approach ≥ 0.80`) | `BLEAuth::isSessionReady()` true | **Door unlock** relay (existing GPIO26) | Keep Stage 1 hysteresis (3 consecutive) |
| **Trunk Zone** | approaching | session ready | **Trunk kick** relay (new GPIO) | Separate debounce |
| **Inside Cabin** | n/a (presence) | session ready | **Engine-start authorization** (new GPIO, logic-level) | Authorization only; not direct start |
| **Any** | `p_anomaly ≥ 0.70` OR no session | — | **No actuation** (deny) | Anomaly/relay-attack protection |

---

## 7. What NOT to Change from Stage 1

These components are load-bearing for security/correctness and must remain **untouched or only minimally, additively extended**:

- **CCC mailbox & trust model** — `iot/src/ccc_mailbox.cpp`, `iot/include/ccc_mailbox.h`. Immobilizer tokens and vehicle identity stay device-local. Stage 2 makes **no** localization decision that bypasses this root of trust. *(Why: core security invariant — secrets never leave the vehicle.)*
- **NFC Phase A provisioning** — `iot/src/nfc_session.cpp`, `iot/src/provisioning_phase.cpp`, and the Android HCE service. Owner enrollment is orthogonal to localization. *(Why: changing it risks breaking a validated fail-closed APDU flow.)*
- **BLE Phase B authentication** — `iot/src/ble/ble_auth.cpp` and phone-side `pke_auth_orchestrator.dart` / `PhaseBCrypto.kt`. Stage 2 **consumes** `isSessionReady()` as an actuation gate but must not alter the handshake, ECDH, or session-key derivation. *(Why: the relay decision must stay cryptographically gated.)*
- **BLE service topology & UUIDs** — `ble.cpp`, `ble_admin/attestation/echo.*`. Only **add** characteristics/OOB fields; do not renumber existing UUIDs (phone and firmware are coupled). *(Why: cross-side protocol compatibility.)*
- **FSM core** — `iot/src/fsm/*`. Reuse and **extend** with new states/events/bridges (`fsm_states.h`, `fsm_integration.h`) rather than rewriting the transition engine or its `validateConfiguration()` checks. *(Why: it is the validated orchestration spine.)*
- **Serial logging conventions** — the `[BLE]`, `[FSM]`, `[AUTH]`, `[PhaseA]` tag scheme consumed by `iot/tools/*.py`. Add new tags (`[RANGE3]`, `[POS2D]`, `[EKF]`, `[INTENT]`, `[ZONE]`) rather than repurposing existing ones. *(Why: keeps existing tools working.)*
- **FreeRTOS task structure & priorities** in `main.cpp` (FSMTask/NFCTask/UWBTask). Extend `uwbTask`; don't destabilize BLE/NFC timing. *(Why: BLE callbacks and NFC timing are already balanced.)*

> Note: the Stage 1 single-anchor path was largely **removed** in the 2026-09-01 cleanup (§0.3): UCI session/UART link, UWB OOB, and the 1-D `Kalman` filter are gone. `UwbDoorUnlock` was renamed to **`AccessController`** and converted to 2-D (`handlePosition(x, y)`); `LstmInference`/`uwb_lstm_model.h` were kept as-is for the future CNN-LSTM milestone (M5/M6).

---

## 8. Open Questions for Developer

Decisions that cannot be resolved from the codebase alone and are required before implementation starts:

- [x] **Anchor coordinate frame**: ✅ **Defined in simulation** (`thesis-ui/`): A0=Front centre (0, 2.00), A1=Left B-pillar (-0.85, 0), A2=Rear centre (0, -2.00). Origin at vehicle centre, X positive → right, Y positive → forward. Final physical coordinates on target vehicle TBD but algorithm is geometry-agnostic (configurable `UwbConfig.ax/ay`).
- [x] **UART strategy (R2)**: ✅ **Resolved.** Collect UWB with **one Python script + direct USB/UART** to the 3 anchors (no RS485, no ESP-NOW).
- [x] **FiRa parameter set (R1)**: ✅ **Resolved.** channel 9, preamble 9, session 42, multicast DS-TWR, static STS (vendor 0x0708) — 3 mixed anchors run successfully.
- [x] **Ranging role**: ✅ **Resolved.** The phone is the sole **Initiator**; the 3 anchors are responders (controlee).
- [ ] **Zone geometry**: Exact boundary definitions (rectangles/polygons) and dimensions for Welcome / Driver Door / Trunk / Inside Cabin relative to the anchor frame.
- [ ] **Intent classes**: Are exactly three classes (approaching / passing / anomaly) final, or do we need per-door directional intent (approaching-driver vs approaching-trunk)?
- [ ] **Actuator wiring**: Which GPIOs and relay/logic levels for trunk-kick and engine-start authorization? Any automotive-safety interlocks required?
- [ ] **Model policy (R3)**: Do we **replace** the Stage 1 3-class LSTM with the CNN‑LSTM, or run both? This drives the RAM/arena budget.
- [ ] **Update rate & latency budget**: Target end-to-end latency from ranging round to relay actuation (affects EKF `dt`, window length, and acceptable GDOP).
- [ ] **Ground-truth method**: How is position accuracy measured for M3/M4/M10 evaluation (tape measure grid, optical tracking, marked path)?
- [ ] **Dataset labeling**: Who labels approach/pass/anomaly trajectories and by what protocol, to keep the training distribution matched to the deployed skew profile (R5)?
