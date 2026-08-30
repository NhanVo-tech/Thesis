#!/usr/bin/env python3
"""Read serial logs, extract [LSTM_DATA] rows, and write them to CSV with labels and run IDs.

Expected firmware format:
[LSTM_DATA],<timestamp_ms>,<raw_m>,<filtered_m>,<residual_m>
"""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

import serial
from serial.tools import list_ports


def default_port() -> str | None:
    ports = list(list_ports.comports())
    if not ports:
        return None
    if len(ports) == 1:
        return ports[0].device
    return None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Capture [LSTM_DATA] lines from serial and save CSV")
    parser.add_argument("--port", help="Serial port (e.g. COM9 or /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--output", type=Path, default=Path("uwb_lstm_data.csv"), help="Output CSV path")
    
    # Required label argument for ML training
    parser.add_argument("--label", type=int, required=True, 
                        help="Ground truth label (e.g., 0: Normal walk, 1: Loitering, 2: Relay Attack)")
    
    # THÊM MỚI: Bắt buộc khai báo Run ID / Session ID
    parser.add_argument("--run", type=int, required=True, 
                        help="Run ID / Session ID (e.g., 1, 2, 3...) to separate discrete time series")
    
    return parser.parse_args()

def main() -> int:
    args = parse_args()
    port = args.port or default_port()
    if not port:
        print("No serial port specified and no single obvious port detected.", file=sys.stderr)
        return 2

    args.output.parent.mkdir(parents=True, exist_ok=True)
    print(f"[CSV] Reading {port} @ {args.baud} -> {args.output}")
    print(f"[CSV] Applying Label: {args.label} | Run ID: {args.run}")
    print("[CSV] Stop with Ctrl+C")

    # --- SỬA Ở ĐÂY: Cấu hình ngắt Auto-Reset ---
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = args.baud
    ser.timeout = 1
    # Tắt tín hiệu DTR và RTS để ngăn ESP32 bị khởi động lại
    ser.dtr = False
    ser.rts = False
    
    try:
        ser.open()
    except Exception as e:
        print(f"[LỖI] Không thể mở cổng {port}: {e}")
        return 1

    # Thay đổi cách dùng "with" do ta đã tự open() ở trên
    with ser, args.output.open("a", newline="") as csv_file:
        writer = csv.writer(csv_file)
        if csv_file.tell() == 0:
            writer.writerow(["run_id", "timestamp_ms", "raw_m", "filtered_m", "residual_m", "label"])

        try:
            while True:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if not line.startswith("[LSTM_DATA],"):
                    continue

                parts = line.split(",")
                if len(parts) != 5:
                    continue

                timestamp_ms, raw_m, filtered_m, residual_m = parts[1:]
                writer.writerow([args.run, timestamp_ms, raw_m, filtered_m, residual_m, args.label])
                csv_file.flush()
                
                print(f"[CSV] Run: {args.run} | Lbl: {args.label} | T: {timestamp_ms} | Raw: {raw_m} | Filt: {filtered_m} | Res: {residual_m}")
                
        except KeyboardInterrupt:
            print("\n[CSV] Stopped")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())