# analyze_ekf.py — UWB EKF analysis & plotting

Offline tool to quantify and visualize how much the on-device EKF smooths the
raw UWB trilateration fixes. It reads a captured ESP32 serial log and produces
numbers + plots you can show to an examiner.

## What it answers

- How noisy is the raw trilateration when the tag is **stationary**?
- How flat does the **EKF** output become?
- How well does the filter follow a **walking / turning** trajectory?
- Which `kAccelStd` value is best (tuned from data, not by feel)?

## Requirements

- Python 3.8+
- `numpy`, `matplotlib` (already listed in `requirements.txt`)

```bash
pip install -r requirements.txt
```

## Capture a log

The tool parses two lines produced by the firmware (`uwb_bridge.cpp`):

```
[RANGE3] t=<ms> d0=.. d1=.. d2=.. valid=..
[POS2D]  t=<ms> x=.. y=.. rms=..        # raw trilateration fix
[EKF]    t=<ms> x=.. y=.. vx=.. vy=.. v=..  # EKF output
```

Any line prefix is fine — PlatformIO `monitor_filters = time` stamps and the
`[ESP] ` prefix from `run_fira_bridge.py --esp-debug` are ignored.

Two easy ways to capture:

1. **PlatformIO monitor**

   ```bash
   platformio device monitor --filter time > capture.log
   ```

2. **Through the PC bridge**

   ```bash
   python run_fira_bridge.py -p COM11 COM19 COM12 --macs 0 1 2 \
       --esp-port COM5 --esp-debug > capture.log
   ```

   Keep the tag still (or walk your scenario), then stop the process.

## Usage

```bash
python analyze_ekf.py --log capture.log [--mode MODE] [--axis x|y] [--out PREFIX]
```

### Options

| Flag | Default | Meaning |
|------|---------|---------|
| `--log FILE` | *(required)* | serial log to analyze |
| `--mode` | `all` | `metrics` / `noise` / `trajectory` / `sweep` / `all` |
| `--axis` | `x` | coordinate used by `noise` / `sweep` / `metrics` |
| `--out PREFIX` | *(none)* | save PNGs instead of showing them (`PREFIX_noise.png`, …) |
| `--sweep-kaccel` | `0.5,1.2,2.0` | `kAccelStd` values for `--mode sweep` |

### Modes

| Mode | Produces |
|------|----------|
| `metrics` | console table: mean / std / max deviation / noise-reduction ratio |
| `noise` | **time-series noise plot** — raw vs EKF for one coordinate (tag stationary) |
| `trajectory` | 2-D plot: raw fixes vs EKF path, with the 2 m unlock circle |
| `sweep` | re-filters raw data with several `kAccelStd` values and overlays them |
| `all` | `metrics` + `noise` + `trajectory` (default) |

## Examples

### 1. Stationary tag — noise reduction (best for examiner)

Place the tag somewhere fixed near the car and capture ~10–20 s of log.

```bash
python analyze_ekf.py --log capture.log --axis x --mode noise
```

Expected: raw `x` jitters roughly ±10–30 cm while the EKF line is nearly flat.
The title and console report the two standard deviations and the ratio, e.g.:

```
std (stationary)   raw= 12.3 cm   filtered=  2.8 cm
noise reduction    4.4x
```

### 2. Walking trajectory

Walk around the car and capture, then:

```bash
python analyze_ekf.py --log capture.log --mode trajectory
```

The red EKF path should track the blue raw fixes without the corner-cutting
lag being too large.

### 3. Tune `kAccelStd` from data

```bash
python analyze_ekf.py --log capture.log --mode sweep --sweep-kaccel 0.5,1.2,2.0
```

Pick the smallest `kAccelStd` that still follows real turns (see the legend's
per-value σ). A value too low lags turns; too high lets jitter through.

### 4. Save plots for the report

```bash
python analyze_ekf.py --log capture.log --mode all --axis x --out fig/ekf
# writes fig/ekf_noise.png, fig/ekf_trajectory.png
```

## Interpreting the metrics

| Metric | Meaning |
|--------|---------|
| `mean` | bias of the estimate around its centre |
| `std (stationary)` | spread; raw should be ~10–30 cm, filtered < ~5 cm |
| `max |dev|` | worst single-sample excursion |
| `noise reduction` | `raw_std / filtered_std`; >3× is a good smoothing result |

> Note: metrics are computed over the whole log. For the "stationary" numbers
> to be meaningful, capture a **stationary** segment (or trim the log to one).
