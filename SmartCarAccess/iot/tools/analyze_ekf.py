#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Analyze the EKF smoothing from a captured ESP32 UWB serial log.

Reads a log containing lines of the form::

    [RANGE3] t=<ms> d0=.. d1=.. d2=.. valid=..
    [POS2D]  t=<ms> x=.. y=.. rms=..
    [EKF]    t=<ms> x=.. y=.. vx=.. vy=.. v=..

(These may be prefixed by a PlatformIO `monitor_filters = time` stamp or by
`[ESP] ` when captured through ``run_fira_bridge.py --esp-debug``; the parser
finds the markers anywhere in the line.)

Modes (``--mode``):

  metrics      print raw-vs-filtered statistics (stationary noise, reduction)
  noise        time-series plot of one coordinate vs time (tag stationary) —
               the most intuitive plot for an examiner
  trajectory   2D plot: raw trilateration fixes vs the EKF path
  sweep        re-run a Python EKF (same maths as the C++ one) with several
               ``kAccelStd`` values to tune the filter from real data
  all          metrics + noise + trajectory (default)

Examples::

  python analyze_ekf.py --log capture.log --axis X --mode noise
  python analyze_ekf.py --log capture.log --mode trajectory
  python analyze_ekf.py --log capture.log --mode sweep --sweep-kaccel 0.5,1.2,2.0
"""

import argparse
import re
import sys

import numpy as np
import matplotlib
matplotlib.use("TkAgg")  # interactive by default; --out saves without blocking
import matplotlib.pyplot as plt

POS_RE = re.compile(
    r"\[POS2D\]\s+t=(\d+)\s+x=(-?[\d.]+)\s+y=(-?[\d.]+)\s+rms=(-?[\d.]+)"
)
EKF_RE = re.compile(
    r"\[EKF\]\s+t=(\d+)\s+x=(-?[\d.]+)\s+y=(-?[\d.]+)\s+"
    r"vx=(-?[\d.]+)\s+vy=(-?[\d.]+)\s+v=(-?[\d.]+)"
)


def parse_log(path):
    """Return (raw, filt) arrays; each is a structured numpy array."""
    raw_t, raw_x, raw_y, raw_rms = [], [], [], []
    ekf_t, ekf_x, ekf_y, ekf_vx, ekf_vy = [], [], [], [], []

    with open(path, "r", encoding="utf-8", errors="ignore") as fh:
        for line in fh:
            m = POS_RE.search(line)
            if m:
                raw_t.append(int(m.group(1)))
                raw_x.append(float(m.group(2)))
                raw_y.append(float(m.group(3)))
                raw_rms.append(float(m.group(4)))
                continue
            m = EKF_RE.search(line)
            if m:
                ekf_t.append(int(m.group(1)))
                ekf_x.append(float(m.group(2)))
                ekf_y.append(float(m.group(3)))
                ekf_vx.append(float(m.group(4)))
                ekf_vy.append(float(m.group(5)))

    raw = np.rec.fromarrays(
        [np.array(raw_t, dtype=np.float64), np.array(raw_x),
         np.array(raw_y), np.array(raw_rms)],
        names="t,x,y,rms",
    )
    filt = np.rec.fromarrays(
        [np.array(ekf_t, dtype=np.float64), np.array(ekf_x),
         np.array(ekf_y), np.array(ekf_vx), np.array(ekf_vy)],
        names="t,x,y,vx,vy",
    )
    return raw, filt


def _stat(series):
    mean = float(np.mean(series))
    std = float(np.std(series))
    dev = float(np.max(np.abs(series - mean))) if len(series) else 0.0
    return mean, std, dev


def print_metrics(raw, filt, axis):
    ra, fa = raw[axis], filt[axis]
    r_mean, r_std, r_dev = _stat(ra)
    f_mean, f_std, f_dev = _stat(fa)
    ratio = (r_std / f_std) if f_std > 1e-9 else float("inf")

    print(f"=== EKF smoothing metrics - axis {axis.upper()} ===")
    print(f"  samples            raw={len(ra)}  filtered={len(fa)}")
    print(f"  mean               raw={r_mean:+.3f} m   filtered={f_mean:+.3f} m")
    print(f"  std (stationary)   raw={r_std * 100:5.1f} cm   filtered={f_std * 100:5.1f} cm")
    print(f"  max |dev|          raw={r_dev * 100:5.1f} cm   filtered={f_dev * 100:5.1f} cm")
    print(f"  noise reduction    {ratio:.2f}x")
    print()
    print("  Expected for a stationary tag: raw jitter ~10-30 cm, "
          "filtered near-flat (std < ~5 cm).")


def time_axis(t_ms):
    if len(t_ms) == 0:
        return t_ms
    return (t_ms - t_ms[0]) / 1000.0


def plot_noise(raw, filt, axis, out=None):
    ra, ta = raw[axis], time_axis(raw["t"])
    fa, ft = filt[axis], time_axis(filt["t"])

    fig, ax = plt.subplots(figsize=(11, 5))
    ax.plot(ta, ra, ".", color="tab:blue", alpha=0.45, ms=6,
            label=f"Raw trilateration {axis} (rms={raw['rms'].mean():.2f} m)")
    ax.plot(ft, fa, "-", color="tab:red", lw=2, label=f"EKF {axis}")

    r_mean, r_std, _ = _stat(ra)
    f_mean, f_std, _ = _stat(fa)
    ax.axhspan(f_mean - f_std, f_mean + f_std, color="tab:red", alpha=0.15)
    ax.axhspan(r_mean - r_std, r_mean + r_std, color="tab:blue", alpha=0.12)

    ax.axhline(r_mean, color="tab:blue", ls="--", lw=1, alpha=0.7)
    ax.axhline(f_mean, color="tab:red", ls="--", lw=1, alpha=0.7)

    ax.set_title(f"UWB coordinate {axis.upper()} — stationary tag "
                 f"(raw σ={r_std * 100:.1f} cm, EKF σ={f_std * 100:.1f} cm, "
                 f"{r_std / f_std if f_std else 0:.1f}x quieter)")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel(f"{axis.upper()} position (m)")
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    _finish(fig, out, "noise")


def plot_trajectory(raw, filt, out=None):
    fig, ax = plt.subplots(figsize=(7, 7))
    ax.plot(raw["x"], raw["y"], ".", color="tab:blue", alpha=0.4, ms=6,
            label="Raw trilateration")
    ax.plot(filt["x"], filt["y"], "-", color="tab:red", lw=2,
            label="EKF path")

    from matplotlib.patches import Circle  # noqa
    ax.add_patch(Circle((-0.85, 0.0), 2.0, fill=False, ec="green", ls="--",
                        lw=1.2, label="Unlock radius (2 m)"))

    ax.set_aspect("equal", adjustable="datalim")
    ax.set_title("2D trajectory — raw fixes vs EKF path")
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    _finish(fig, out, "trajectory")


def plot_sweep(raw, axis, kaccels, out=None):
    """Re-filter the raw fixes with a Python port of the C++ EKF for several
    kAccelStd values and overlay the resulting smoothed coordinate."""
    fig, ax = plt.subplots(figsize=(11, 5))
    t = time_axis(raw["t"])
    ax.plot(t, raw[axis], ".", color="tab:blue", alpha=0.4, ms=5,
            label="Raw trilateration")

    for ka in kaccels:
        ekf = PyEKF(kAccelStd=ka)
        out_axis = []
        for i in range(len(raw)):
            ekf.update(raw["x"][i], raw["y"][i], raw["t"][i], raw["rms"][i])
            out_axis.append(ekf.x[0] if axis == "x" else ekf.x[1])
        out_axis = np.array(out_axis)
        _, std, _ = _stat(out_axis)
        ax.plot(t, out_axis, "-", lw=1.6,
                label=f"kAccelStd={ka} m/s²  (σ={std * 100:.1f} cm)")

    ax.set_title(f"EKF kAccelStd sweep — coordinate {axis.upper()} "
                 "(stationary tag)")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel(f"{axis.upper()} position (m)")
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    _finish(fig, out, "sweep")


def _finish(fig, out, default_name):
    if out:
        path = out if out.endswith(".png") else f"{out}_{default_name}.png"
        fig.savefig(path, dpi=150)
        print(f"Saved plot: {path}")
        plt.close(fig)
    else:
        plt.show()


class PyEKF:
    """Minimal Python port of the C++ EKF in ekf_stub.cpp (constant-velocity,
    2D position measurement). Used only for offline sweep/tuning."""

    def __init__(self, kAccelStd=1.2, minMeasStd=0.05, maxMeasStd=1.00):
        self.q = kAccelStd * kAccelStd
        self.min_std = minMeasStd
        self.max_std = maxMeasStd
        self.x = np.zeros(4)          # [px, py, vx, vy]
        self.P = np.zeros((4, 4))
        self.init = False
        self.last_ms = 0

    def reset(self):
        self.x = np.zeros(4)
        self.P = np.zeros((4, 4))
        self.init = False
        self.last_ms = 0

    def _meas_var(self, std):
        if not std > 0:
            std = 0.15
        std = min(max(std, self.min_std), self.max_std)
        return std * std

    def _predict(self, dt):
        self.x[0] += self.x[2] * dt
        self.x[1] += self.x[3] * dt
        F = np.array([[1, 0, dt, 0], [0, 1, 0, dt],
                      [0, 0, 1, 0], [0, 0, 0, 1]])
        q, dt2, dt3, dt4 = self.q, dt * dt, dt ** 3, dt ** 4
        Q = np.zeros((4, 4))
        Q[0][0] = q * dt4 / 4
        Q[0][2] = q * dt3 / 2
        Q[2][0] = q * dt3 / 2
        Q[2][2] = q * dt2
        Q[1][1] = q * dt4 / 4
        Q[1][3] = q * dt3 / 2
        Q[3][1] = q * dt3 / 2
        Q[3][3] = q * dt2
        self.P = F @ self.P @ F.T + Q

    def _correct(self, zx, zy, meas_var):
        S = self.P[:2, :2] + meas_var * np.eye(2)
        det = np.linalg.det(S)
        if abs(det) < 1e-12:
            return
        iS = np.linalg.inv(S)
        K = self.P[:, :2] @ iS
        y = np.array([zx - self.x[0], zy - self.x[1]])
        self.x += K @ y
        M = np.eye(4)
        M[:, 0] -= K[:, 0]
        M[:, 1] -= K[:, 1]
        newP = M @ self.P
        self.P = 0.5 * (newP + newP.T)

    def update(self, x, y, t_ms, meas_std):
        meas_var = self._meas_var(meas_std)
        if not self.init:
            self.x[0], self.x[1] = x, y
            self.x[2], self.x[3] = 0.0, 0.0
            self.P = np.diag([meas_var, meas_var, 4.0, 4.0])
            self.init = True
            self.last_ms = t_ms
            return
        dt = (t_ms - self.last_ms) / 1000.0
        self.last_ms = t_ms
        if dt <= 0 or dt > 2.0:
            self.reset()
            self.update(x, y, t_ms, meas_std)
            return
        self._predict(dt)
        self._correct(x, y, meas_var)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--log", required=True, help="serial log file to analyze")
    ap.add_argument("--mode", default="all",
                    choices=["metrics", "noise", "trajectory", "sweep", "all"],
                    help="what to produce (default: all)")
    ap.add_argument("--axis", default="x", choices=["x", "y"],
                    help="coordinate for noise/sweep plots (default: x)")
    ap.add_argument("--out", default=None,
                    help="save plots to this PNG prefix instead of showing")
    ap.add_argument("--sweep-kaccel", default="0.5,1.2,2.0",
                    help="comma list of kAccelStd values for --mode sweep")
    args = ap.parse_args()

    raw, filt = parse_log(args.log)
    if len(raw) == 0 and len(filt) == 0:
        sys.exit("No [POS2D]/[EKF] lines found in log.")

    if len(raw):
        print(f"Parsed {len(raw)} raw [POS2D] samples, "
              f"{len(filt)} filtered [EKF] samples.")

    if args.mode in ("metrics", "all"):
        print_metrics(raw, filt, args.axis)
    if args.mode in ("noise", "all"):
        plot_noise(raw, filt, args.axis, args.out)
    if args.mode in ("trajectory", "all"):
        plot_trajectory(raw, filt, args.out)
    if args.mode == "sweep":
        kaccels = [float(v) for v in args.sweep_kaccel.split(",")]
        plot_sweep(raw, args.axis, kaccels, args.out)


if __name__ == "__main__":
    main()
