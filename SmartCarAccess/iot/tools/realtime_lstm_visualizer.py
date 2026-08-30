#!/usr/bin/env python3
"""
Real-time LSTM + UWB Visualization Tool

Reads live serial data from ESP32 and plots:
  1. Distance (raw vs filtered) with unlock threshold
  2. Residual & Velocity features
  3. LSTM probabilities (Walk, Loiter, Attack) with confidence threshold

Features:
  - Synchronized time axes across all subplots
  - Shaded background on door unlock events
  - Academic styling (serif fonts, grids, clear units)
  - Real-time animation with matplotlib
  - Export to PDF/SVG for high-quality publications
"""

from __future__ import annotations

import argparse
import csv
import sys
from collections import deque
from pathlib import Path
from threading import Thread
from typing import NamedTuple

import matplotlib
matplotlib.use('TkAgg')  # Use interactive backend
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.artist import Artist
from matplotlib.patches import Rectangle
import numpy as np
import serial
from serial.tools import list_ports


class DataPoint(NamedTuple):
    """Single frame of UWB + AI data"""
    time_s: float
    frame_num: int
    raw_m: float
    filtered_m: float
    residual_m: float
    velocity_m: float
    p_walk: float
    p_loiter: float
    p_attack: float
    door_open: bool


class SerialDataCollector:
    """Thread-safe serial data collection from ESP32"""
    
    def __init__(self, port: str, baudrate: int = 115200, buffer_size: int = 500):
        self.port = port
        self.baudrate = baudrate
        self.ser: serial.Serial | None = None
        self.data_buffer = deque(maxlen=buffer_size)
        self.door_event_times = deque(maxlen=buffer_size)
        self.running = False
        self.start_time_ms: int | None = None
        self.last_timestamp_ms: int | None = None
        self.frame_num = 0
        
    def connect(self) -> bool:
        try:
            # Disable auto-reset: set DTR/RTS BEFORE opening the port
            self.ser = serial.Serial()
            self.ser.port = self.port
            self.ser.baudrate = self.baudrate
            self.ser.timeout = 1
            self.ser.dtr = False
            self.ser.rts = False
            self.ser.open()
            return True
        except Exception as e:
            print(f"[ERROR] Cannot open {self.port}: {e}")
            return False
    
    def disconnect(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
    
    def run(self):
        """Main collection thread loop"""
        self.running = True
        lstm_data = {}  # Buffer the latest [LSTM_DATA]
        ai_data = {}    # Buffer the latest [AI]
        door_open_pending = False
        
        if not self.ser or not self.ser.is_open:
            print("[ERROR] Serial port not connected")
            self.running = False
            return
        
        while self.running:
            try:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue
                
                # Parse [LSTM_DATA] line
                if line.startswith('[LSTM_DATA],'):
                    parts = line.split(',')
                    if len(parts) == 5:
                        try:
                            lstm_data = {
                                'timestamp_ms': int(parts[1]),
                                'raw_m': float(parts[2]),
                                'filtered_m': float(parts[3]),
                                'residual_m': float(parts[4]),
                            }
                            self.last_timestamp_ms = lstm_data['timestamp_ms']
                            # Initialize start time on first frame
                            if self.start_time_ms is None:
                                self.start_time_ms = lstm_data['timestamp_ms']
                        except (ValueError, IndexError):
                            pass
                
                # Parse [AI] line (inference results)
                elif line.startswith('[AI] Walk:'):
                    try:
                        # Format: [AI] Walk: 0.85 | Loiter: 0.10 | Attack: 0.05
                        parts = line.split('|')
                        if len(parts) == 3:
                            p_walk = float(parts[0].split(':')[1].strip())
                            p_loiter = float(parts[1].split(':')[1].strip())
                            p_attack = float(parts[2].split(':')[1].strip())
                            ai_data = {
                                'p_walk': p_walk,
                                'p_loiter': p_loiter,
                                'p_attack': p_attack,
                            }
                    except (ValueError, IndexError):
                        pass
                
                # Detect door unlock event (latch until next data point)
                if (
                    '[DOOR] *** FIRING UNLOCK RELAY ***' in line or
                    '[DOOR] *** REACHED 3 CONSECUTIVE ATTEMPTS. UNLOCK DOOR ***' in line
                ):
                    door_open_pending = True
                    event_time_s = None
                    if self.start_time_ms is not None and self.last_timestamp_ms is not None:
                        event_time_s = (self.last_timestamp_ms - self.start_time_ms) / 1000.0
                    elif self.data_buffer:
                        event_time_s = self.data_buffer[-1].time_s
                    if event_time_s is not None:
                        self.door_event_times.append(event_time_s)
                
                # Merge and add to buffer when both LSTM_DATA and AI are available
                if lstm_data and ai_data:
                    time_s = (lstm_data['timestamp_ms'] - self.start_time_ms) / 1000.0
                    
                    # Compute velocity (simple delta)
                    last_point = self.data_buffer[-1] if self.data_buffer else None
                    if last_point:
                        dt = time_s - last_point.time_s
                        velocity_m = (lstm_data['filtered_m'] - last_point.filtered_m) / dt if dt > 0 else 0.0
                    else:
                        velocity_m = 0.0
                    
                    point = DataPoint(
                        time_s=time_s,
                        frame_num=self.frame_num,
                        raw_m=lstm_data['raw_m'],
                        filtered_m=lstm_data['filtered_m'],
                        residual_m=lstm_data['residual_m'],
                        velocity_m=velocity_m,
                        p_walk=ai_data['p_walk'],
                        p_loiter=ai_data['p_loiter'],
                        p_attack=ai_data['p_attack'],
                        door_open=door_open_pending,
                    )
                    self.data_buffer.append(point)
                    self.frame_num += 1
                    lstm_data.clear()
                    ai_data.clear()
                    door_open_pending = False
            
            except Exception as e:
                pass
    
    def get_data(self) -> list[DataPoint]:
        """Return current buffered data"""
        return list(self.data_buffer)

    def get_door_events(self) -> list[float]:
        """Return door unlock event times (seconds)"""
        return list(self.door_event_times)


class RealtimeLSTMVisualizer:
    """Real-time 3-subplot visualization with academic styling"""
    
    def __init__(self, collector: SerialDataCollector, window_size: int = 200):
        self.collector = collector
        self.window_size = window_size  # Number of frames to display
        
        # Create figure with 3 stacked subplots
        plt.rcParams['font.family'] = 'serif'
        plt.rcParams['font.size'] = 10
        
        self.fig, (self.ax1, self.ax2, self.ax3, self.ax4) = plt.subplots(
            4, 1, figsize=(14, 12), sharex=True
        )
        
        # ===== SUBPLOT 1: Distance =====
        self.ax1.set_ylabel('Distance (m)', fontsize=12, fontweight='bold')
        self.ax1.grid(True, linestyle='--', alpha=0.5)
        self.ax1.set_ylim(-0.5, 5.0)
        
        # Raw distance (faint gray)
        self.line_raw, = self.ax1.plot([], [], color='gray', linewidth=0.8, 
                                        label='Raw UWB', alpha=0.6)
        # Filtered distance (thick blue)
        self.line_filtered, = self.ax1.plot([], [], color='#1f77b4', linewidth=2.0, 
                                             label='Kalman Filtered')
        # Unlock threshold (red dashed)
        self.ax1.axhline(y=2.0, color='red', linestyle='--', linewidth=1.5, 
                         label='Unlock Threshold (2.0m)', alpha=0.7)
        self.ax1.legend(loc='upper right', fontsize=10)
        
        # ===== SUBPLOT 2: Features =====
        self.ax2.set_ylabel('Normalized Value', fontsize=12, fontweight='bold')
        self.ax2.grid(True, linestyle='--', alpha=0.5)
        self.ax2.set_ylim(-2.0, 3.0)
        
        # Residual (red/orange)
        self.line_residual, = self.ax2.plot([], [], color='#ff7f0e', linewidth=1.5, 
                                            label='Kalman Residual')
        # Velocity (green)
        self.line_velocity, = self.ax2.plot([], [], color='#2ca02c', linewidth=1.5, 
                                            label='Velocity')
        self.ax2.legend(loc='upper right', fontsize=10)
        
        # ===== SUBPLOT 3: LSTM Probabilities =====
        self.ax3.set_xlabel('Time (s)', fontsize=12, fontweight='bold')
        self.ax3.set_ylabel('Probability', fontsize=12, fontweight='bold')
        self.ax3.grid(True, linestyle='--', alpha=0.5)
        self.ax3.set_ylim(-0.1, 1.1)
        
        # p_walk (green)
        self.line_walk, = self.ax3.plot([], [], color='#2ca02c', linewidth=2.0, 
                                        label=r'$P(\mathrm{Walk})$')
        # p_loiter (orange/yellow)
        self.line_loiter, = self.ax3.plot([], [], color='#ff7f0e', linewidth=2.0, 
                                          label=r'$P(\mathrm{Loiter})$')
        # p_attack (red)
        self.line_attack, = self.ax3.plot([], [], color='#d62728', linewidth=2.0, 
                                          label=r'$P(\mathrm{Attack})$')
        # Confidence threshold (black dashed)
        self.ax3.axhline(y=0.80, color='black', linestyle='--', linewidth=1.0, 
                         label='Confidence Threshold (0.8)', alpha=0.7)
        self.ax3.legend(loc='upper right', fontsize=10)

        # ===== SUBPLOT 4: Door State (Binary) =====
        self.ax4.set_xlabel('Time (s)', fontsize=12, fontweight='bold')
        self.ax4.set_ylabel('Door State', fontsize=12, fontweight='bold')
        self.ax4.grid(True, linestyle='--', alpha=0.5)
        self.ax4.set_ylim(-0.1, 1.1)
        self.ax4.set_yticks([0, 1])
        self.ax4.set_yticklabels(['Locked', 'Unlocked'])
        self.line_door_state, = self.ax4.plot([], [], color='#1f77b4', linewidth=2.0,
                              drawstyle='steps-post', label='Door State')
        self.ax4.legend(loc='upper right', fontsize=10)
        
        self.fig.tight_layout()
        self.door_open_events = []  # Track times when door opened for shading
        self.last_door_open_time: float | None = None
        self.door_open_hold_s = 2.0
        self.last_relay_trigger_time: float | None = None
        self.relay_hold_s = 1.5
        self.relay_text = self.fig.text(
            0.86,
            0.965,
            "Relay: IDLE",
            fontsize=11,
            fontweight='bold',
            color='#444444',
            ha='left',
            va='top',
            bbox=dict(facecolor='white', alpha=0.75, edgecolor='#999999', boxstyle='round,pad=0.25')
        )
    
    def update_plot(self, frame: int) -> list[Artist]:
        """Update all subplots with latest data"""
        data = self.collector.get_data()
        
        if not data:
            return []
        
        # Only show last window_size frames
        if len(data) > self.window_size:
            data = data[-self.window_size:]
        
        times = [p.time_s for p in data]
        raws = [p.raw_m for p in data]
        filtered = [p.filtered_m for p in data]
        residuals = [p.residual_m for p in data]
        velocities = [p.velocity_m for p in data]
        p_walks = [p.p_walk for p in data]
        p_loiters = [p.p_loiter for p in data]
        p_attacks = [p.p_attack for p in data]
        
        # ===== Update Subplot 1: Distance =====
        self.line_raw.set_data(times, raws)
        self.line_filtered.set_data(times, filtered)
        
        # ===== Update Subplot 2: Features =====
        self.line_residual.set_data(times, residuals)
        self.line_velocity.set_data(times, velocities)
        
        # ===== Update Subplot 3: Probabilities =====
        self.line_walk.set_data(times, p_walks)
        self.line_loiter.set_data(times, p_loiters)
        self.line_attack.set_data(times, p_attacks)
        
        # ===== Add shaded regions for door unlock events =====
        self.door_open_events = self.collector.get_door_events()
        
        # Add light blue vertical bands for each door open event
        for unlock_time in self.door_open_events:
            if times[0] <= unlock_time <= times[-1]:
                # Add semi-transparent vertical band
                for ax in [self.ax1, self.ax2, self.ax3]:
                    ax.axvspan(unlock_time, unlock_time + 1.0, alpha=0.15, 
                              color='blue', label='Door Unlock' if unlock_time == self.door_open_events[0] else '')

        # Update door state (latched for a short duration after unlock event)
        if self.door_open_events:
            latest_event_time = self.door_open_events[-1]
            if self.last_door_open_time is None or latest_event_time > self.last_door_open_time:
                self.last_door_open_time = latest_event_time
                self.last_relay_trigger_time = latest_event_time

        door_states = []
        last_open_time = None
        event_idx = 0
        for t in times:
            while event_idx < len(self.door_open_events) and self.door_open_events[event_idx] <= t:
                last_open_time = self.door_open_events[event_idx]
                event_idx += 1
            is_open = (
                last_open_time is not None and
                (t - last_open_time) <= self.door_open_hold_s
            )
            door_states.append(1.0 if is_open else 0.0)
        self.line_door_state.set_data(times, door_states)

        current_time = times[-1]

        relay_triggered = (
            self.last_relay_trigger_time is not None and
            (current_time - self.last_relay_trigger_time) <= self.relay_hold_s
        )
        if relay_triggered:
            self.relay_text.set_text("Relay: TRIGGERED")
            self.relay_text.set_color('#1f77b4')
        else:
            self.relay_text.set_text("Relay: IDLE")
            self.relay_text.set_color('#444444')
        
        # ===== Auto-scale X-axis =====
        if times:
            self.ax1.set_xlim(times[0], times[-1] + 1.0)
            self.ax3.set_xlim(times[0], times[-1] + 1.0)
            self.ax4.set_xlim(times[0], times[-1] + 1.0)

        return [
            self.line_raw,
            self.line_filtered,
            self.line_residual,
            self.line_velocity,
            self.line_walk,
            self.line_loiter,
            self.line_attack,
            self.line_door_state,
            self.relay_text,
        ]
    
    def show(self):
        """Start real-time animation"""
        print("[VIS] Starting real-time visualization...")
        print("[VIS] Close the window or press Ctrl+C to stop")
        
        anim = FuncAnimation(self.fig, self.update_plot, interval=100, 
                           blit=False, cache_frame_data=False)
        plt.show()
    
    def export_pdf(self, filename: str):
        """Export current plot to PDF"""
        self.fig.savefig(filename, format='pdf', dpi=300, bbox_inches='tight')
        print(f"[VIS] Exported to {filename}")
    
    def export_svg(self, filename: str):
        """Export current plot to SVG"""
        self.fig.savefig(filename, format='svg', dpi=300, bbox_inches='tight')
        print(f"[VIS] Exported to {filename}")


def default_port() -> str | None:
    ports = list(list_ports.comports())
    if not ports:
        return None
    if len(ports) == 1:
        return ports[0].device
    return None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Real-time LSTM + UWB visualization with stacked subplots'
    )
    parser.add_argument('--port', default=default_port(),
                       help='Serial port (e.g., COM9 or /dev/ttyUSB0). Auto-detect if not provided.')
    parser.add_argument('--baud', type=int, default=115200,
                       help='Serial baud rate (default: 115200)')
    parser.add_argument('--window', type=int, default=200,
                       help='Number of frames to display in plot window (default: 200)')
    parser.add_argument('--export-pdf', type=Path,
                       help='Export final plot to PDF file')
    parser.add_argument('--export-svg', type=Path,
                       help='Export final plot to SVG file')
    
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    
    if not args.port:
        print("[ERROR] No serial port specified and no single obvious port detected.", file=sys.stderr)
        return 2
    
    print(f"[VIS] Connecting to {args.port} @ {args.baud} baud...")
    
    collector = SerialDataCollector(args.port, args.baud, buffer_size=1000)
    if not collector.connect():
        return 1
    
    # Start data collection thread
    collector_thread = Thread(target=collector.run, daemon=True)
    collector_thread.start()
    
    # Create visualizer
    viz = RealtimeLSTMVisualizer(collector, window_size=args.window)
    
    try:
        viz.show()
    except KeyboardInterrupt:
        print("\n[VIS] Stopped by user")
    finally:
        collector.running = False
        collector.disconnect()
        
        # Export if requested
        if args.export_pdf:
            viz.export_pdf(str(args.export_pdf))
        if args.export_svg:
            viz.export_svg(str(args.export_svg))
    
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
