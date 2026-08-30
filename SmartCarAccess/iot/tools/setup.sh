#!/usr/bin/env bash
# Quick setup and test script for visualization tools

echo "========================================"
echo "UWB + LSTM Visualization Suite Setup"
echo "========================================"
echo ""

# Check Python
if ! command -v python3 &> /dev/null; then
    echo "[ERROR] Python 3 not found. Install Python 3.8+"
    exit 1
fi

echo "[✓] Python 3 found: $(python3 --version)"
echo ""

# Install dependencies
echo "[*] Installing Python dependencies..."
pip install -r requirements.txt

if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to install dependencies"
    exit 1
fi

echo "[✓] Dependencies installed"
echo ""

# Show available tools
echo "========================================"
echo "Available Tools:"
echo "========================================"
echo ""

echo "1. Data Collection (CSV logging):"
echo "   python serial_csv_logger.py --label 0 --run 1 --port COM9"
echo ""

echo "2. Real-Time Visualization:"
echo "   python realtime_lstm_visualizer.py --port COM9"
echo ""

echo "3. Export Results:"
echo "   python realtime_lstm_visualizer.py --port COM9 --export-pdf scenario.pdf"
echo ""

echo "========================================"
echo "Next Steps:"
echo "========================================"
echo "1. Connect ESP32 via USB"
echo "2. Verify firmware is running (check serial monitor)"
echo "3. Run one of the tools above"
echo ""
echo "For detailed documentation:"
echo "   cat README_VISUALIZATION.md"
echo ""
