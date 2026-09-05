# AI_INTENT_RECOGNITION.md

TinyML **intent recognition** for Stage 2 (milestones M-D → M-F): a Conv1D classifier
running on-device (TensorFlow Lite for Microcontrollers) that recognises the user's behaviour
from a sliding window of the fused UWB trajectory `[x, y, vx, vy]`, and gates the deterministic
access decisions of `ACCESS_CONTROL.md`.

> Last updated: 2026-09-05.

---

## 1. Objectives

1. **G1 — Recognise intent, not just position.** Turn a short trajectory window
   `[x, y, vx, vy]` (≈2.5 s @ 10 Hz) into one of four behaviour classes:
   `approach / seated / leave / passing`.
2. **G2 — Run on-device.** Deploy the model on the ESP32-S3 with TFLM, matching the memory
   budget already proven by the Stage-1 Conv1D (35 KB arena).
3. **G3 — Gate, don't decide.** The AI output *gates* the deterministic transitions in
   `AccessController` (unlock / seat-settle / leave), rather than replacing them. This keeps the
   fail-safe deterministic core while adding intent-level robustness.
4. **G4 — Reproduce Stage-1 quality.** Reach ≥ the Stage-1 accuracy (~77%) on a held-out test
   set, with a clean confusion matrix, and demo the three scenarios reliably.

**Non-goals:** replacing secure ranging (DS-TWR + STS) or the BLE session gate; the AI is not the
primary security boundary. The Stage-1 "attack" class is intentionally **dropped** in this stage
(secure ranging already blocks relay at the PHY layer) — see §11.

---

## 2. From Stage 1 to Stage 2

| | Stage 1 (1-D) | Stage 2 (2-D) |
|---|---|---|
| Sensor | single-anchor distance | 3-anchor → trilateration → EKF |
| Feature | `[distance, residual, velocity]` | `[x, y, vx, vy]` |
| Window | 25 frames | 25 frames |
| Classes | walk / loiter / attack | approach / seated / leave / passing |
| Model | Conv1D | Conv1D (same family) |
| Role | relay-attack detection + walk confirm | intent recognition + gate |
| Scaler | RobustScaler (median/IQR) | StandardScaler (mean/std) |

Stage-1 lessons carried forward: the **hybrid pattern** (deterministic core + AI gate), the
**Conv1D** architecture (light, ESP32-friendly), the **run-stratified train/test split** (no data
leakage), and the **softmax thresholds** (`p ≥ 0.80` to act, `p ≥ 0.70` to block).

---

## 3. Architecture & data flow

```
PC bridge (d0,d1,d2) ──> Trilateration ──> (x,y) + RMS
                      ──> EKF ──> (x,y,vx,vy) @ 10 Hz   [UwbBridge::tick]
                             │
                             ├──> Geofence ──> stable Zone
                             ├──> AccessController (deterministic core)
                             └──> TrajInference (TFLM) ──> p_approach/p_seated/p_leave/p_passing
                                         │
                                         └──> gate the AccessController transitions
```

`UwbBridge::tick()` already drives `AccessController::handlePosition(x,y,vx,vy)` at a fixed 100 ms
cadence. The inference runs in the same path so that every 100 ms the AI sees one EKF frame, forms
a sliding window, and (once warm) emits a softmax vector.

---

## 4. Behaviour classes

Four classes, one per demo scenario plus a reject class. Labels are fixed:

| idx | class | Definition | Motion pattern (what to act out) |
|-----|-------|------------|-----------------------------------|
| 0 | `approach` | walk toward the door with intent to enter | distance-to-door ↓, speed ↓ (decelerate), ends near door (−0.95, 0) |
| 1 | `seated` | stationary at the driver seat | position stable at (−0.35, 0), speed ≈ 0 for a sustained period |
| 2 | `leave` | walk away from the car | distance ↑, speed > 0 outward, ends beyond 4 m |
| 3 | `passing` | walk past the car without stopping | distance ↓ then ↑, speed constant (never slows to a stop) |

`passing` is the **negative** class: it must suppress the unlock that the geometric "approach +
deceleration" gate could otherwise admit in a tangential pass.

---

## 5. Feature engineering

Per-frame feature vector (4 dims):

```
f_t = [ x, y, vx, vy ]
```

- `x, y` — EKF-smoothed position in the car frame (metres). They encode the geometry (door at
  (−0.95, 0), seat at (−0.35, 0)) implicitly, so the Conv1D learns "near door & stopping" vs
  "sweeping past".
- `vx, vy` — EKF velocity (m/s). These are the strongest signal for approach/leave/passing:
  deceleration, sustained ~0, outward motion.

**Normalization:** z-score (`StandardScaler`) fitted on the **training** runs only, then applied to
train and test. Mean/std are exported to the C++ module as `scaler_mean[NUM_FEATURES]` /
`scaler_scale[NUM_FEATURES]`. (If outliers prove problematic, switch to `RobustScaler` as Stage 1
did — the pipeline supports both.)

**Window:** `TIME_STEPS = 25` frames @ 10 Hz ≈ 2.5 s — enough to see a full "approach then stop"
or "walk past" gesture, matching Stage 1.

---

## 6. Model architecture & hyperparameters

Conv1D (identical family to Stage 1; verified to run in TFLM):

```
Input            (25, 4)
Conv1D(16, k=3, relu)
MaxPooling1D(2)
Conv1D(32, k=3, relu)
MaxPooling1D(2)
Flatten
Dense(16, relu)
Dropout(0.2)
Dense(4, softmax)
```

| Hyperparameter | Value |
|----------------|-------|
| `TIME_STEPS` | 25 |
| `NUM_FEATURES` | 4 |
| `STEP_SIZE` | 1 |
| `BATCH_SIZE` | 32 |
| `EPOCHS` | 100 (with `ReduceLROnPlateau`) |
| `TEST_SPLIT` | 0.2 (split **by run**, not by row) |
| Optimizer | Adam |
| Loss | sparse categorical crossentropy |
| Class weighting | `compute_class_weight('balanced')` |
| Export ops | `TFLITE_BUILTINS` only |

---

## 7. M-D — Data collection (step-by-step)

### 7.1 Prerequisites
- Firmware flashed with the M-B/M-C build (logs `[EKF]`, `[STATE]`, `[IGNITION]`, `[ZONE]`).
- 3 anchors on tripods, phone held in hand, **always line-of-sight**.
- `localization_demo.py` runnable (UCI + pyserial; see its docstring).

### 7.2 Record sessions (one capture per behaviour)

For each class, record a capture file while acting out that behaviour. Repeat **10–15 runs per
class**, each run ≈ 5–10 s (≈ 50–100 frames @ 10 Hz). More variance = better generalisation.

```
python localization_demo.py -p COM11 COM19 COM12 --macs 0 1 2 --esp-port COM5 \
       --autostart --capture approach.log
```

Act out the behaviour, vary the approach angle/speed between runs, then `Ctrl+C`. Repeat for
`seated.log`, `leave.log`, `passing.log`.

**Acting protocol per class** (keep it natural, vary it across runs):

| class | What to do |
|-------|------------|
| approach | start > 4 m out, walk toward the door, **decelerate and stop** at the door |
| seated | stand at the seat point (−0.35, 0) and hold still; vary entry direction |
| leave | from the door/seat, walk away decisively past 4 m |
| passing | walk a tangential line (rear-left → front-left) through the door at a steady pace, never stop |

### 7.3 Convert captures to labelled CSVs

```
python collect_traj.py --log approach.log --label 0 --run 1 -o uwb_traj_data_label0.csv
python collect_traj.py --log seated.log   --label 1 --run 1 -o uwb_traj_data_label1.csv
python collect_traj.py --log leave.log    --label 2 --run 1 -o uwb_traj_data_label2.csv
python collect_traj.py --log passing.log  --label 3 --run 1 -o uwb_traj_data_label3.csv
```

Increment `--run` for each distinct session. `collect_traj.py` appends, so multiple runs share one
CSV per class. CSV columns: `run_id, timestamp_ms, x, y, vx, vy, label`.

**Quality checks before training:**
- Each class file has ≥ 5 runs and ≥ 300 rows total.
- No run is shorter than `TIME_STEPS` (25) frames.
- Label distribution is roughly balanced (class weights handle mild imbalance).

---

## 8. M-E — Training (step-by-step)

Run in Google Colab (tensorflow + sklearn available).

1. Upload the four CSVs and `train_traj_model.py`.
2. In a cell: `!pip install -q tensorflow scikit-learn matplotlib seaborn` (if needed).
3. `%run train_traj_model.py` (or copy it into a cell).
4. Review:
   - **Test accuracy** (target ≥ 77%).
   - **Confusion matrix** — the worst-case cells should be `passing↔approach`; if `seated` is
     confused with `approach`, increase seated data / lengthen the seated runs.
5. Download the two outputs:
   - `uwb_traj_model.h` → place in `iot/include/uwb/uwb_traj_model.h`.
   - The printed `scaler_mean` / `scaler_scale` → paste into `traj_inference.cpp`.

---

## 9. M-F — On-device inference + gating (design)

### 9.1 `traj_inference.h/.cpp` (to implement)

Mirrors the Stage-1 `LstmInference` but for 4 features / 4 classes:

```cpp
// traj_inference.h
#define TIME_STEPS 25
#define NUM_FEATURES 4
#define NUM_CLASSES 4

class TrajInference {
 public:
  bool begin();                                   // load model + allocate arena
  bool predict(float x, float y, float vx, float vy,
               float p[4]);                       // sliding window + z-score + Invoke
  int  getFrameCount() const;
 private:
  float window[TIME_STEPS][NUM_FEATURES];
  int   frame_count;
  const float scaler_mean[NUM_FEATURES];          // from training
  const float scaler_scale[NUM_FEATURES];
  // TFLM: model / interpreter / arena (35–48 KB) / input / output
};
```

Key behaviours (from Stage 1, re-validated):
- Warm-up: `predict()` returns `false` until the window is full (25 frames).
- `begin()` returns `false` gracefully if the model/arena fails → AI disabled, deterministic core
  keeps working (fail-safe).
- Arena size starts at 35 KB, grow to ~48 KB if `AllocateTensors()` fails; monitor heap.

### 9.2 Gating logic (integration)

`AccessController::handlePosition` gains the softmax probs (or the argmax + confidence). Combined
logic (deterministic first, AI gates second) — thresholds mirror Stage 1:

| Transition | Deterministic (unchanged) | AI gate (added) |
|------------|---------------------------|-----------------|
| unlock (`LOCKED→DOOR_UNLOCKED`) | zone ∈ {DOOR,SEAT} ∧ speed < 0.4, 3 hits | `p_approach ≥ 0.80` |
| seat (`DOOR_UNLOCKED→OCCUPIED`) | zone = SEAT ∧ speed < 0.4 for 3 s | `p_seated ≥ 0.80` |
| leave (`OCCUPIED→LOCKED`) | zone = OUTSIDE ∧ moving away, 3 hits | `p_leave ≥ 0.80` |
| **suppress unlock** | — | `p_passing ≥ 0.70` hard-blocks unlock |

The `p_passing` hard-block is the AI's headline contribution: it rejects tangential pass-bys that
the deceleration gate alone might let through at low speeds.

### 9.3 Wiring

In `UwbBridge::tick()`, alongside the existing `AccessController::handlePosition(...)` call:
1. Call `TrajInference::predict(x, y, vx, vy, p)`.
2. Log `[INTENT] p_approach=.. p_seated=.. p_leave=.. p_passing=..` (new tag).
3. Pass `p` into `AccessController` (or let `AccessController` hold a `TrajInference&`).

`localization_demo.py` will parse `[INTENT]` and show the top class + confidence.

---

## 10. Division of responsibility (deterministic vs AI)

| Concern | Owner | Why |
|---------|-------|-----|
| Position/velocity estimation | Trilateration + EKF | accurate, low-latency |
| Zone classification | Geofence | pure geometry |
| Approach/leave velocity gate, seat timer | AccessController | deterministic, fail-safe |
| **Intent recognition** (approach/seated/leave/passing) | **AI (TFLM)** | temporal pattern, non-trivial thresholds |
| Final relay/GPIO actuation | AccessController | keeps fail-safe + debounce |

Principle: **the AI never fires an actuator directly** — it only raises/lowers confidence gates on
the deterministic transitions. If the AI fails (model missing / arena OOM), the system degrades to
the deterministic core (still functional, slightly more false-positives).

---

## 11. Attack detection — intentionally dropped

Stage-2 secure ranging (DS-TWR + static STS, CCC tunnel) already defeats relay at the PHY layer
(speed-of-light bound + scrambled timestamps). The Stage-1 `attack` class was a soft mitigation for
the weaker 1-D setup. Therefore Stage 2 **does not** train an `attack` class. If a defence-in-depth
`anomaly` class is later desired, add it as a 5th class — but do not block Stage 2 on it.

---

## 12. Evaluation & acceptance criteria

- **Model accuracy** on held-out runs ≥ 77% (match Stage 1); confusion matrix reviewed.
- **End-to-end demo**: the three scenarios (unlock / seat→lock+ignition / leave→lock) each work
  with the AI gate enabled, and a tangential pass **does not** unlock.
- **False-positive rate**: zero unlock on `passing` trajectories in the evaluation set.
- **Latency**: inference adds ≤ a few ms per 100 ms tick (no perceptible impact).
- **Memory**: TFLM arena fits with the existing NimBLE + EKF budget; `[AI] begin ok` on boot.

---

## 13. Risks & mitigations

| Risk | Mitigation |
|------|-----------|
| Hand-held micro-motion pollutes `seated` | rely on EKF-smoothed `vx,vy`; keep `speed<0.4` gate; add seated data with natural sway |
| `approach` vs `passing` confusion | collect passing at varied speeds/angles; tune the `p_passing ≥ 0.70` block |
| Data leakage (same run in train+test) | split by `run_id` (already in pipeline) |
| TFLM arena OOM | start 35 KB, grow to 48 KB; keep `#if`-guardable; monitor `[AI]` boot log |
| Overfit to fixed anchor layout | acceptable for the demo (anchors fixed); re-collect if geometry changes |

---

## 14. Checklist

- [ ] M-D: capture 4 behaviour sets (10–15 runs each) via `localization_demo.py --capture`
- [ ] M-D: convert to labelled CSVs via `collect_traj.py`
- [ ] M-E: train in Colab; review accuracy + confusion matrix
- [ ] M-E: export `uwb_traj_model.h` + scaler params
- [ ] M-F: implement `traj_inference.h/.cpp` (TFLM)
- [ ] M-F: wire into `UwbBridge::tick` + gate `AccessController`
- [ ] M-F: add `[INTENT]` log + `localization_demo.py` display
- [ ] M-G: end-to-end demo of the 3 scenarios + passing rejection
