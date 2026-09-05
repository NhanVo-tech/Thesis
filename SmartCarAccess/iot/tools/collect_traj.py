#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""collect_traj.py — capture [EKF] trajectory rows (x, y, vx, vy) into a labelled CSV.

Reads the ESP32 serial stream (or a captured log file) and records one row per
[EKF] line, tagged with a ground-truth behaviour label + run id for the Stage-2
intent classifier (train_traj_model.py):

    label 0 = approach (walk toward the door, then slow/stop)
    label 1 = seated   (stand still at the driver seat)
    label 2 = leave    (walk away from the car)
    label 3 = passing  (walk past the car without stopping)

Output CSV columns:
    run_id, timestamp_ms, x, y, vx, vy, label

Usage (live serial):
    python collect_traj.py --port COM5 --label 0 --run 1 -o uwb_traj_data_label0.csv

Usage (from a previously captured log, same format as localization_demo --log):
    python collect_traj.py --log capture.log --label 2 --run 3 -o uwb_traj_data_label2.csv
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # pragma: no cover
    serial = None
    list_ports = None

EKF_RE = re.compile(
    r"\[EKF\]\s+(?:t=(\d+)\s+)?x=(-?[\d.]+)\s+y=(-?[\d.]+)\s+"
    r"vx=(-?[\d.]+)\s+vy=(-?[\d.]+)\s+v=(-?[\d.]+)"
)

LABELS = {0: "approach", 1: "seated", 2: "leave", 3: "passing"}


def default_port() -> str | None:
    if list_ports is None:
        return None
    ports = list(list_ports.comports())
    if len(ports) == 1:
        return ports[0].device
    return None


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--port", help="ESP32 USB-CDC serial port (e.g. COM5)")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--log", help="replay a captured log file instead of live serial")
    p.add_argument("--label", type=int, required=True, choices=LABELS,
                   help="ground-truth behaviour label")
    p.add_argument("--run", type=int, required=True,
                   help="run/session id (separates discrete trajectories)")
    p.add_argument("-o", "--output", type=Path,
                   default=Path("uwb_traj_data.csv"),
                   help="output CSV path (append mode)")
    return p.parse_args()


def write_row(writer, run, ts, x, y, vx, vy, label):
    writer.writerow([run, ts, x, y, vx, vy, label])


def main() -> int:
    args = parse_args()
    if args.log is None and not args.port:
        args.port = default_port()
    if args.log is None and not args.port:
        print("No --log and no serial port available.", file=sys.stderr)
        return 2

    args.output.parent.mkdir(parents=True, exist_ok=True)
    print(f"[TRAJ] label={args.label} ({LABELS[args.label]}) run={args.run} -> {args.output}")
    print("[TRAJ] Stop with Ctrl+C")

    ser = None
    if args.log is None:
        ser = serial.Serial()
        ser.port = args.port
        ser.baudrate = args.baud
        ser.timeout = 1
        ser.dtr = False
        ser.rts = False
        try:
            ser.open()
        except Exception as e:
            print(f"[TRAJ] cannot open {args.port}: {e}", file=sys.stderr)
            return 1
        print(f"[TRAJ] reading {args.port} @ {args.baud}")

    def lines():
        if args.log is not None:
            with open(args.log, "r", encoding="utf-8", errors="ignore") as fh:
                for line in fh:
                    yield line
        else:
            while True:
                raw = ser.readline()
                if not raw:
                    continue
                yield raw.decode("utf-8", errors="ignore")

    header = ["run_id", "timestamp_ms", "x", "y", "vx", "vy", "label"]
    try:
        with args.output.open("a", newline="") as csv_file:
            writer = csv.writer(csv_file)
            if csv_file.tell() == 0:
                writer.writerow(header)
            for line in lines():
                m = EKF_RE.search(line)
                if not m:
                    continue
                ts = m.group(1) if m.group(1) is not None else "0"
                x, y = float(m.group(2)), float(m.group(3))
                vx, vy = float(m.group(4)), float(m.group(5))
                writer.writerow([args.run, ts, x, y, vx, vy, args.label])
                csv_file.flush()
    except KeyboardInterrupt:
        print("\n[TRAJ] stopped")
    finally:
        if ser is not None:
            try:
                ser.close()
            except Exception:
                pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
