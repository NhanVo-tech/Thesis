#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""localization_demo.py — live/replay UWB localization demo.

Draws the car (4 m x 2 m, front pointing up) centred on the origin with its
3 anchors, then animates the phone/tag position in real time.

  * The **EKF estimate** is the bright, "final" position (marker + recent trail).
  * The **raw trilateration** fixes are drawn as dim dots so the EKF's smoothing
    is visually obvious.
  * A stats panel shows live x/y, velocity, and distances to each anchor.

Usage:
  # live, reading the ESP32 USB-CDC serial port:
  python localization_demo.py --port COM9

  # replay a previously captured log (see README_ANALYZE_EKF.md):
  python localization_demo.py --log capture.log

Parsed lines (same format as analyze_ekf.py):
  [POS2D] t=<ms> x=.. y=.. rms=..
  [EKF]   t=<ms> x=.. y=.. vx=.. vy=.. v=..
"""

import argparse
import collections
import re
import sys
import time

import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Circle

# -----------------------------------------------------------------------------
# Geometry — MUST match iot/include/uwb/uwb_geometry.h (kAnchorX / kAnchorY)
# and iot/include/uwb/access_controller.h (kUnlockPointX/Y, UNLOCK_RADIUS_M).
# -----------------------------------------------------------------------------

CAR_LENGTH = 4.0   # along Y (front/rear)
CAR_WIDTH = 2.0    # along X (left/right)

# New anchor layout: 0 = left B-pillar, 1 = right (opposite), 2 = rear centre.
# Coordinates reuse the values from uwb_geometry.h (left B-pillar -0.85 m,
# rear -2.0 m); update both files together when the physical anchors move.
ANCHORS = [
    (-0.85, 0.0),   # anchor 0 — left B-pillar (driver door)
    (0.85, 0.0),    # anchor 1 — right side (opposite anchor 0)
    (0.0, -2.0),    # anchor 2 — rear centre
]

UNLOCK_POINT = ANCHORS[0]     # driver door
UNLOCK_RADIUS = 2.0           # access_controller.h UNLOCK_RADIUS_M

RAW_KEEP = 40     # dim raw dots kept on screen
EKF_KEEP = 60     # bright EKF trail length

# -----------------------------------------------------------------------------
# Log parsing (shared format with analyze_ekf.py)
# -----------------------------------------------------------------------------

POS_RE = re.compile(
    r"\[POS2D\]\s+t=(\d+)\s+x=(-?[\d.]+)\s+y=(-?[\d.]+)\s+rms=(-?[\d.]+)"
)
EKF_RE = re.compile(
    r"\[EKF\]\s+t=(\d+)\s+x=(-?[\d.]+)\s+y=(-?[\d.]+)\s+"
    r"vx=(-?[\d.]+)\s+vy=(-?[\d.]+)\s+v=(-?[\d.]+)"
)


def parse_line(line):
    m = POS_RE.search(line)
    if m:
        return ("raw", int(m.group(1)), float(m.group(2)), float(m.group(3)),
                float(m.group(4)))
    m = EKF_RE.search(line)
    if m:
        return ("ekf", int(m.group(1)), float(m.group(2)), float(m.group(3)),
                float(m.group(4)), float(m.group(5)))
    return None


def iter_events(args):
    """Yield parsed (kind, t, ...) tuples from serial or a log file."""
    if args.port:
        import serial
        ser = serial.Serial(args.port, args.baud, timeout=0.2)
        print(f"Reading live from {args.port} @ {args.baud} ...")
        while True:
            line = ser.readline().decode("utf-8", errors="ignore")
            ev = parse_line(line)
            if ev:
                yield ev
    elif args.log:
        with open(args.log, "r", encoding="utf-8", errors="ignore") as fh:
            for line in fh:
                ev = parse_line(line)
                if ev:
                    yield ev
    else:
        sys.exit("Provide --port COMx (live) or --log file (replay).")


# -----------------------------------------------------------------------------
# Plot state
# -----------------------------------------------------------------------------

def build_figure():
    fig = plt.figure(figsize=(11, 7))
    gs = fig.add_gridspec(1, 2, width_ratios=[3.0, 1.2], wspace=0.12)

    ax = fig.add_subplot(gs[0, 0])
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.25, lw=0.5)
    ax.set_xlim(-4.5, 4.5)
    ax.set_ylim(-4.0, 4.0)
    ax.axhline(0, color="0.7", lw=0.6)
    ax.axvline(0, color="0.7", lw=0.6)
    ax.set_xlabel("x (m)")
    ax.set_ylabel("y (m)")
    ax.set_title("UWB localization — car frame (front ↑)")

    # Car: 4 m x 2 m rectangle centred on origin, front pointing +Y.
    car = Rectangle((-CAR_WIDTH / 2, -CAR_LENGTH / 2), CAR_WIDTH, CAR_LENGTH,
                    facecolor="#dbe7f5", edgecolor="#273671", lw=2, zorder=1)
    ax.add_patch(car)
    # Front indicator (windshield / headlight hint near y = +CAR_LENGTH/2).
    ax.plot([-CAR_WIDTH / 2, CAR_WIDTH / 2],
            [CAR_LENGTH / 2 - 0.5, CAR_LENGTH / 2 - 0.5],
            color="#273671", lw=1.5, zorder=2)
    ax.text(0, CAR_LENGTH / 2 - 0.7, "FRONT", ha="center", fontsize=8,
            color="#273671", zorder=3)

    # Anchors.
    for i, (axx, ayy) in enumerate(ANCHORS):
        ax.plot(axx, ayy, marker="s", ms=11, color="#c0392b", zorder=4)
        ax.text(axx, ayy - 0.35, f"A{i}", ha="center", fontsize=9,
                color="#c0392b", weight="bold", zorder=5)

    # Unlock zone around the driver door.
    ax.add_patch(Circle(UNLOCK_POINT, UNLOCK_RADIUS, fill=False,
                        ec="#27ae60", ls="--", lw=1.2, zorder=1))
    ax.text(UNLOCK_POINT[0] + 0.2, UNLOCK_POINT[1] + UNLOCK_RADIUS + 0.15,
            "unlock zone", fontsize=8, color="#27ae60", zorder=3)

    # Track artists (updated every frame).
    raw_sc = ax.scatter([], [], s=16, color="#9aa7b8", alpha=0.35,
                        label="raw trilateration", zorder=6)
    ekf_line, = ax.plot([], [], color="#e67e22", lw=2.0, label="EKF path",
                        zorder=7)
    ekf_pt, = ax.plot([], [], marker="o", ms=9, color="#e67e22",
                      mfc="#e67e22", mec="#7a3b00", zorder=8)
    ax.legend(loc="lower right", fontsize=8, framealpha=0.9)

    # Stats panel (right).
    sa = fig.add_subplot(gs[0, 1])
    sa.axis("off")
    sa.set_title("Live stats", fontsize=11, weight="bold", loc="left")
    stats_text = sa.text(0.0, 1.0, "", va="top", ha="left",
                         family="monospace", fontsize=9.5)

    fig.suptitle("")
    return fig, ax, sa, stats_text, raw_sc, ekf_line, ekf_pt


def fmt_stats(t_ms, ekf, raw, n):
    lines = []
    if ekf is not None:
        x, y, vx, vy = ekf
        speed = float(np.hypot(vx, vy))
        lines += [
            f"time       {t_ms / 1000.0:7.2f} s",
            "",
            "EKF (final)",
            f"  x        {x:+7.2f} m",
            f"  y        {y:+7.2f} m",
            f"  vx       {vx:+7.2f} m/s",
            f"  vy       {vy:+7.2f} m/s",
            f"  speed    {speed:7.2f} m/s",
        ]
        dist_car = float(np.hypot(x, y))
        lines.append(f"  d(center){dist_car:7.2f} m")
        d_door = float(np.hypot(x - UNLOCK_POINT[0], y - UNLOCK_POINT[1]))
        lines.append(f"  d(door)  {d_door:7.2f} m")
    if raw is not None:
        rx, ry = raw
        lines += [
            "",
            "RAW (trilateration)",
            f"  x        {rx:+7.2f} m",
            f"  y        {ry:+7.2f} m",
        ]
        if ekf is not None:
            ex, ey = ekf[0], ekf[1]
            lines.append(f"  |raw-ekf|{np.hypot(rx - ex, ry - ey):7.2f} m")
    lines += [
        "",
        f"samples    {n:7d}",
        "",
        "anchors",
    ]
    for i, (axx, ayy) in enumerate(ANCHORS):
        if ekf is not None:
            d = float(np.hypot(ekf[0] - axx, ekf[1] - ayy))
            lines.append(f"  A{i} {d:7.2f} m")
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", default=None, help="serial port (live), e.g. COM9")
    ap.add_argument("--baud", type=int, default=115200, help="serial baud rate")
    ap.add_argument("--log", default=None, help="captured log file (replay)")
    args = ap.parse_args()

    if not args.port and not args.log:
        ap.error("provide --port (live) or --log (replay)")

    fig, ax, sa, stats_text, raw_sc, ekf_line, ekf_pt = build_figure()

    raw_hist = collections.deque(maxlen=RAW_KEEP)   # (x, y)
    ekf_hist = collections.deque(maxlen=EKF_KEEP)   # (x, y)
    last_ekf = None
    last_raw = None
    last_t = 0
    count = 0

    plt.ion()
    plt.show()

    try:
        for ev in iter_events(args):
            kind = ev[0]
            if kind == "raw":
                _, t, x, y, rms = ev
                last_raw = (x, y)
                raw_hist.append((x, y))
            else:  # ekf
                _, t, x, y, vx, vy = ev
                last_ekf = (x, y, vx, vy)
                ekf_hist.append((x, y))
                count += 1
            last_t = t

            # update artists
            if raw_hist:
                raw_sc.set_offsets(np.array(raw_hist))
            if ekf_hist:
                eh = np.array(ekf_hist)
                ekf_line.set_data(eh[:, 0], eh[:, 1])
                ekf_pt.set_data([eh[-1, 0]], [eh[-1, 1]])
            stats_text.set_text(fmt_stats(last_t, last_ekf, last_raw, count))

            fig.canvas.draw_idle()
            fig.canvas.flush_events()
            time.sleep(0.02)
    except KeyboardInterrupt:
        pass
    finally:
        if args.port:
            pass
        print("Stopped.")


if __name__ == "__main__":
    main()
