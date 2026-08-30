# Real-Time UWB + LSTM Visualization Suite

This directory contains Python tools for collecting and visualizing UWB distance measurements combined with real-time LSTM inference results from the ESP32 firmware.

---

## Overview

Two complementary tools are provided:

### 1. **serial_csv_logger.py** - Data Collection for Training
Captures serial `[LSTM_DATA]` lines and exports them to CSV with labels for offline ML training.

### 2. **realtime_lstm_visualizer.py** - Real-Time Visualization
Streams live serial data and plots three synchronized subplots showing distance, features, and AI probabilities.

---

## Installation

Install Python dependencies:

```bash
pip install -r requirements.txt
```

Dependencies:
- `pyserial>=3.5` — Serial port communication
- `numpy>=1.20.0` — Numerical computing
- `matplotlib>=3.5.0` — Real-time plotting

---

## Tool 1: serial_csv_logger.py

### Purpose
Collect labeled training data for future LSTM model retraining.

### Expected Firmware Output Format
```
[LSTM_DATA],<timestamp_ms>,<raw_m>,<filtered_m>,<residual_m>
```

### Usage

```bash
# Specify port explicitly
python serial_csv_logger.py --port COM6 --baud 115200 --output my_data.csv --label 0 --run 1

 python tools/realtime_lstm_visualizer.py --port COM6
```

### Arguments
- `--label <int>` **(required)** — Ground truth class:
  - `0` = Normal walking approach
  - `1` = Loitering / hovering behavior
  - `2` = Relay attack attempt
  
- `--run <int>` **(required)** — Session/Run ID to separate distinct time series (e.g., 1, 2, 3...)

- `--port <str>` — Serial port (auto-detects if omitted)

- `--baud <int>` — Baud rate (default: 115200)

- `--output <path>` — Output CSV file (default: `uwb_lstm_data.csv`)

### Output CSV Format
```
run_id, timestamp_ms, raw_m, filtered_m, residual_m, label
1,      1000,         2.50,  2.45,      0.05,        0
1,      1020,         2.48,  2.44,      0.04,        0
...
```

### Example Workflow

**Collect 3 scenarios:**

```bash
# Scenario 1: Normal walk-in (5 minutes)
python serial_csv_logger.py --label 0 --run 1 --port COM9

# Scenario 2: Relay attack simulation (3 minutes)
python serial_csv_logger.py --label 2 --run 2 --port COM9

# Scenario 3: Loitering behavior (5 minutes)
python serial_csv_logger.py --label 1 --run 3 --port COM9
```

Results append to `uwb_lstm_data.csv` automatically. Upload this CSV to Colab for model retraining.

---

## Tool 2: realtime_lstm_visualizer.py

### Purpose
Real-time visualization of UWB distance and LSTM predictions for validation, debugging, and academic poster presentation.

### Expected Firmware Output Format
```
[LSTM_DATA],<timestamp_ms>,<raw_m>,<filtered_m>,<residual_m>
[AI] Walk: 0.85 | Loiter: 0.10 | Attack: 0.05
[DOOR] *** FIRING UNLOCK RELAY *** (signals door unlock event)
```

### Usage

```bash
# Real-time visualization (auto-detect port)
python realtime_lstm_visualizer.py

# Specify port explicitly
python realtime_lstm_visualizer.py --port COM9

# Adjust display window
python realtime_lstm_visualizer.py --window 300

# Export to PDF after closing window
python realtime_lstm_visualizer.py --export-pdf results.pdf

# Export to SVG (vector format for publication)
python realtime_lstm_visualizer.py --export-svg results.svg
```

### Arguments
- `--port <str>` — Serial port (auto-detects if omitted)
- `--baud <int>` — Baud rate (default: 115200)
- `--window <int>` — Frames to display (default: 200)
- `--export-pdf <path>` — Save plot as PDF on exit
- `--export-svg <path>` — Save plot as SVG on exit

### Plot Layout

The visualization displays **3 synchronized subplots**:

#### **Subplot 1: Distance (m)**
- Gray faint line: Raw UWB distance
- Blue thick line: Kalman-filtered distance (smoothed)
- Red dashed line: Door unlock threshold (2.0m)

**Purpose:** Show filter effectiveness and user approach trajectory.

#### **Subplot 2: Features (Normalized)**
- Orange line: Kalman residual (raw - filtered)
- Green line: Instantaneous velocity
- X-axis: Time (seconds)

**Purpose:** Visualize feature extraction quality; relay attacks show residual spikes.

#### **Subplot 3: LSTM Probabilities**
- Green line: P(Walk) — probability of normal approach
- Orange line: P(Loiter) — probability of hovering
- Red line: P(Attack) — probability of relay attack
- Black dashed line: Confidence threshold (0.8)
- **Blue vertical band:** Marks moments when door relay fired

**Purpose:** Show AI decision process; blue bands correlate with successful unlocks.

### Professional Features

✓ **Academic Styling:**
  - Serif fonts (Times New Roman style) for publication-ready appearance
  - Grid lines for easy data point reference
  - Clear axis labels with units

✓ **Real-Time Animation:**
  - Updates every 100ms
  - Scrolling time window
  - Smooth line rendering

✓ **Door Unlock Events:**
  - Light blue vertical bands overlay when relay fires
  - Visually confirms AI + hysteresis agreement

✓ **Vector Export:**
  - PDF format (300 DPI) for poster printing without blur
  - SVG format for editing in Inkscape or Adobe Illustrator

### Example Scenarios

#### **Scenario A: Normal Walk-In**
```
Time 0-2s:  Distance drops from 5m → 2m (smooth curve)
           Raw ≈ Filtered (small residuals ~0.05m)
           Velocity smoothly decreases
           P(Walk) rises to 0.95+
           ➜ Blue band appears (door opens)
```

#### **Scenario B: Relay Attack**
```
Time 1-1.5s: Distance suddenly drops to 1.5m
             Residual spikes to 0.3-0.5m (latency artifact)
             Velocity shows discontinuity
             P(Attack) jumps to 0.85+
             P(Walk) crashes to <0.2
             ➜ NO blue band (door blocked)
```

#### **Scenario C: Loitering**
```
Time 0-3s:  Distance bounces around 2.2m
            Velocity oscillates +/- 0.2m/s
            Residuals erratic
            P(Loiter) dominates (0.6+)
            P(Walk) stays <0.5
            ➜ No blue band (no unlock)
```

---

## Workflow: From Collection to Publication

### Step 1: Collect Training Data
```bash
# Multiple sessions for each scenario class
for i in {1..5}; do
  echo "Collecting normal walk #$i..."
  python serial_csv_logger.py --label 0 --run $i
done

for i in {6..8}; do
  echo "Collecting attack #$(($i-5))..."
  python serial_csv_logger.py --label 2 --run $i
done
```

Upload `uwb_lstm_data.csv` to Colab for LSTM retraining.

### Step 2: Deploy Retrained Model
Replace `uwb_lstm_model.h` in firmware and rebuild.

### Step 3: Validate with Real-Time Visualization
```bash
python realtime_lstm_visualizer.py --export-pdf scenario_normal.pdf
python realtime_lstm_visualizer.py --export-pdf scenario_attack.pdf
python realtime_lstm_visualizer.py --export-pdf scenario_loiter.pdf
```

### Step 4: Publication
Include PDFs in thesis/poster with captions:

> **Figure X (a):** Normal user approach. Kalman filter smooths raw UWB distance. LSTM correctly predicts P(Walk) = 0.94, triggering relay (blue band).
>
> **Figure X (b):** Relay attack attempt. Sudden distance drop with high residual spikes. LSTM correctly rejects (P(Attack) = 0.87). No unlock.
>
> **Figure X (c):** Loitering behavior. User lingers at 2.2m with erratic velocity. LSTM identifies P(Loiter) = 0.72. Door remains locked.

---

## Troubleshooting

### No data appearing in plot
- Verify firmware is running and logging `[LSTM_DATA]` + `[AI]` lines
- Check serial monitor: `pio run -t monitor`
- Ensure baud rate matches (`--baud 115200`)

### Plot exports are blurry
- Always use `--export-pdf` or `--export-svg` (not PNG)
- Vector formats preserve quality at any zoom level

### Serial port not auto-detected
- List available ports: `python -m serial.tools.list_ports`
- Specify explicitly: `--port /dev/ttyUSB0` (Linux) or `--port COM9` (Windows)

### Memory errors on ESP32 during collect
- Reduce logging frequency in firmware
- Check UART buffer size settings

---

## References

- **Kalman Filter:** Input smoothing in `uci_session_manager.cpp`
- **LSTM Model:** TensorFlow Lite Micro in `lstm_inference.cpp`
- **Door Logic:** Hysteresis + AI gating in `uci_door_unlock.cpp`
- **Serial Format:** Defined in `main.cpp` and `uwb/` modules

---

## Author Notes

These tools support your thesis research workflow:
1. **Data Collection** → CSV export for offline ML
2. **Model Validation** → Real-time visualization confirms AI decisions
3. **Publication** → Professional PDFs for posters and papers

The visualization clearly demonstrates the cause-effect relationship between:
- **Physical observation** (distance, velocity)
- **Feature extraction** (residuals)
- **AI decision** (probabilities)
- **System action** (relay firing)

This holistic view is ideal for defending your work to the thesis committee. 🎓
