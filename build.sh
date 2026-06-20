#!/usr/bin/env bash
set -e

echo "== Espresso ESP32 setup =="

. ./esp-idf/export.sh

# 1. check ESP-IDF
if [ -z "$IDF_PATH" ]; then
    echo "[INFO] ESP-IDF not sourced. Trying default path..."

    if [ -d "$HOME/esp/esp-idf" ]; then
        export IDF_PATH="$HOME/esp/esp-idf"
    else
        echo "[ERROR] ESP-IDF not found!"
        echo "Install it first: https://docs.espressif.com/projects/esp-idf/"
        exit 1
    fi
fi

echo "[OK] IDF_PATH = $IDF_PATH"

# 2. export environment
echo "[INFO] Sourcing ESP-IDF export.sh..."
. "$IDF_PATH/export.sh"

# 3. set target
echo "[INFO] Setting target to esp32..."
idf.py set-target esp32

# 4. build
echo "[INFO] Building project..."
idf.py -DCMAKE_EXPORT_COMPILE_COMMANDS=ON build

echo "== Build finished =="

# 5. optional flash + monitor
read -p "Flash now? (y/n): " FLASH

if [ "$FLASH" = "y" ]; then
    read -p "Serial port (e.g. /dev/ttyUSB0): " PORT
    echo "[INFO] Flashing..."
    idf.py -p "$PORT" flash

    echo "[INFO] Starting monitor..."
    idf.py -p "$PORT" monitor
fi
