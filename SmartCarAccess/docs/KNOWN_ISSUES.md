# KNOWN_ISSUES.md

Annotated limitations, TODOs, and research-grade shortcuts. Severity: 🔴 high · 🟠 medium ·
🟡 low.

> Last updated: 2026-09-02. Legacy UCI/OOB/ESP-NOW issues have been removed (those modules no
> longer exist). New entries reflect the PC-bridge + trilateration + EKF pipeline.

---

## 1. Architecture / Transport

| # | Severity | Issue |
|---|----------|-------|
| 1 | 🟠 | **PC bridge is a research-grade transport.** UWB distances reach the vehicle through a PC (`run_fira_bridge.py`) over USB-CDC. This is convenient for experiments but is not a production topology; a real vehicle would host the UWB radios directly. |
| 2 | 🟡 | **Single-car / single 3-anchor session.** The bridge collects one 3-anchor session at a time; multi-vehicle or multi-user concurrency is not implemented. |
| 3 | 🟡 | **No active retransmission on the USB-CDC link.** Dropped `RANGE` lines are tolerated by EKF prediction (up to `kMaxGapS = 2 s`) but not re-requested. |
| 4 | 🟡 | **Anchor geometry is hard-coded** in `uwb_geometry.h` (A0/A1/A2). Runtime/field calibration is not supported. |

## 2. UWB Fusion / Unlock

| # | Severity | Issue |
|---|----------|-------|
| 5 | 🟠 | **Geometric unlock only — no learned intent yet.** The relay is gated purely by zone geometry + approach gate. The CNN-LSTM intent classifier (approach/pass-by/anomaly) is not yet integrated (STAGE2_PLAN M5/M6). |
| 6 | ⏸ | **1-D LSTM is dormant.** `lstm_inference.cpp` / `uwb_lstm_model.h` remain in the tree with baked-in scalers but are not called. They will be replaced, not reused as-is, by the 4-D trajectory model. |
| 7 | 🟡 | **EKF tuning is fixed.** `kAccelStd = 1.2 m/s²` and the 0.05–1.0 m measurement-noise clamp are constants; not adaptive to environment/NLOS conditions. |
| 8 | 🟡 | **Unlock constants are hardware-coupled** (unlock/reset radius, relay GPIO26, pulse 500 ms). They assume the reference anchor layout and a single driver-door unlock point. |
| 9 | 🟡 | **Trilateration 2-anchor fallback is ambiguous.** With only two valid anchors the solver picks one intersection; there is no seeding from the previous fix at that layer (the EKF absorbs some of this). |

## 3. Security / Policy

| # | Severity | Issue |
|---|----------|-------|
| 10 | 🟠 | **Friend key sharing (slots 1–7) is disabled by policy.** Attestation verifies but the owner-only gate rejects slots 1–7 (`ERR_SLOT_LOCKED`); token MAC binding for share payloads is not implemented. |
| 11 | 🟠 | **No trusted time source enforced.** Time-bound attestations rely on epoch time-sync over BLE; NTP/RTC is not mandatory, so validity windows can be influenced by the phone. |
| 12 | 🟡 | **Ranging is gated by the BLE session but not by continuous liveness.** Once `s_session_keys_ready`, `START_RANGING` is accepted; there is no periodic re-attestation during a long ranging session. |
| 13 | 🟡 | **Debug/test bypasses exist** (admin force-provision, RSSI monitor-only, verbose logs). These must be disabled for any real deployment. |

## 4. App / Cloud

| # | Severity | Issue |
|---|----------|-------|
| 14 | 🟡 | **Access-pattern anomaly ≠ physical relay-attack detection.** The app's time/location/frequency + Gemini scoring protects key *usage*; it does not gate the vehicle relay. |
| 15 | 🟡 | **Phone-side UWB requires Android 16+ (API 36).** `android.ranging` availability limits the deployable device set. |
| 16 | 🟡 | **Background BLE stability depends on Doze exemption.** Without battery-optimization exemption the foreground service may be throttled. |
| 17 | 🟡 | **Gemini calls require network + API key.** Anomaly enrichment degrades to rule-based when offline. |

## 5. Documentation drift to watch

- Keep `run_fira_bridge.py` defaults (rate/fresh/bounds/session/channel) in sync with
  [API_REFERENCE.md](API_REFERENCE.md) §2 and [CODEBASE_REFERENCE.md](CODEBASE_REFERENCE.md) §6.
- When the CNN-LSTM lands, update [FEATURES.md](FEATURES.md), [DATA_CONTRACTS.md](DATA_CONTRACTS.md)
  §1.8, and [STAGE2_PLAN.md](STAGE2_PLAN.md) milestones.
