# UWB Test Guide: DWM3001CDK / nRF52840DK + Android (multicast DS-TWR)

This guide collects all commands used to test the UWB ranging system:
- **Anchor (responder)**: DWM3001CDK / nRF52840DK + DWM3000EVB boards, driven by Python scripts (UCI).
- **Controller (initiator)**: Android 16 phone running the `uwb-multicast-android` app.

---

## 1. Architecture Overview

The Android app and the Python SDK are **not wired together**. They only "meet" over the UWB radio (FiRa) when the over-the-air (OTA) parameters match:

| Parameter | Value | Notes |
|---|---|---|
| Session id | 42 | |
| Channel / Preamble | 9 / 9 | |
| Mode | Multicast DS-TWR | |
| Phone address (controller) | 1729 (0x06C1) | `LOCAL_ADDRESS` in `Uwb.kt` |
| 3 anchor addresses | 0, 1, 2 | `uwbAnchors` in `Uwb.kt` |
| STS (static) | vendor 0x0708 + IV | matches by default between app and script |

---

## 2. Environment Setup

Tools already available on this machine:

| Tool | Path |
|---|---|
| Python 3.10 | `C:\Users\Legion\AppData\Local\Programs\Python\Python310\python.exe` |
| adb | `C:\Users\Legion\AppData\Local\Android\Sdk\platform-tools\adb.exe` |
| JDK (Android Studio jbr) | `C:\Program Files\Android\Android Studio\jbr` |
| ARM GCC 10.3 | `C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin` |
| CMake | on PATH |
| mingw32-make | `C:\mingw64\bin\mingw32-make.exe` |

> Note: the SDK Python scripts require **Python 3.9–3.10** (Python 3.14 fails to import `uci`).

### Install dependencies for the Python scripts (one-time)

```powershell
& "C:\Users\Legion\AppData\Local\Programs\Python\Python310\python.exe" -m pip install pyserial colorama
```

---

## 3. Running the Python Script (anchor / responder)

### 3.1. Identifying the COM port

```powershell
Get-CimInstance Win32_PnPEntity -Filter "PNPClass='Ports'" | Select-Object Name
```

- DWM3001CDK board: plug USB into **J20** (near the antenna) → shows up as `USB Serial Device (COMx)`.
  - **J9** (far from antenna) = flashing, **J20** = UCI communication.
- nRF52840DK: plug USB into **J3** (long side) to communicate; **J2** (short side) to flash.

### 3.2. Run 3 anchors with a single script (one terminal) — final approach

Use the custom script **`run_fira_multianchor.py`** (same directory as `run_fira_twr.py`):
opens **3 COM ports in a single process**, talks UART/USB directly to each anchor,
collects the 3 distances and computes **(x, y) via trilateration right in the terminal**.

```powershell
# Set PYTHONPATH (required to import the uci / uqt_utils libraries)
$env:PYTHONPATH = "C:\DATN\DW3_QM33_SDK_1.1.1\SDK\Tools\uwb-qorvo-tools\lib\uwb-uci;C:\DATN\DW3_QM33_SDK_1.1.1\SDK\Tools\uwb-qorvo-tools\lib\uqt-utils"

# Single terminal for all 3 anchors (with each anchor's (x,y) position in metres)
& "C:\Users\Legion\AppData\Local\Programs\Python\Python310\python.exe" run_fira_multianchor.py -p COM11 COM19 COM12 --anchor 0,0 5,0 0,5 -t -1
```

Sample output (updates ~every 100 ms, matching the radio rate):
```
[22:10:01]  d0=0.24m  d1=0.35m  d2=0.30m  =>  (x=1.23, y=2.34) m
```

Parameter reference:

| Parameter | Value | Meaning |
|---|---|---|
| `-p` | `COM11 COM19 COM12` | list of COM ports for the 3 anchors |
| `--macs` | `0 1 2` (default) | address of each anchor (must match the app) |
| `--anchor` | `0,0 5,0 0,5` | (x,y) metres of each anchor — **must match the real board positions** |
| `--dest-mac` | `0x06C1` (default) | phone (controller) address = 1729 |
| `--preamble-idx` | `9` (default) | must match the app |
| `-t` | `-1` | run forever (Ctrl+C to stop) |

> On success: prints 3 `ranging started` lines, then continuously `d0/d1/d2 => (x,y)`.
> Trilateration uses Gauss-Newton (3+ anchors) / geometric intersection (2 anchors) — ported from `Trilateration.kt` in the Android app.

### 3.3. (Optional) Run each anchor separately with `run_fira_twr.py`

If you only need raw distance per anchor (no trilateration), run one terminal per anchor:

```powershell
$env:PYTHONPATH = "C:\DATN\DW3_QM33_SDK_1.1.1\SDK\Tools\uwb-qorvo-tools\lib\uwb-uci;C:\DATN\DW3_QM33_SDK_1.1.1\SDK\Tools\uwb-qorvo-tools\lib\uqt-utils"

& "C:\Users\Legion\AppData\Local\Programs\Python\Python310\python.exe" run_fira_twr.py -p COM11 --controlee --node onetomany --mac 0x0 --dest-mac 0x06C1 --preamble-idx 9 -t -1
```

To dump results to JSON (diagnostics + statistics), add `--stats --en-diag --diag_dump`.

> On success: terminal shows `Device -> Ready`, `Session -> Active`, and `# Ranging Data` messages with `distance`.

---

## 4. Build & Run the Android App (controller)

Project directory: `C:\DATN\uwb-multicast-android`

### 4.1. Configuration (one-time)

- Create `local.properties`:
  ```
  sdk.dir=C:/Users/Legion/AppData/Local/Android/Sdk
  ```
- In `app/build.gradle.kts`: change `compileSdkVersion = "android-Baklava"` → `compileSdk = 36`
  (because only the `android-36` platform is installed, not the `android-Baklava` codename).

### 4.2. Build the APK

```powershell
cd C:\DATN\uwb-multicast-android
$env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"
.\gradlew.bat assembleDebug
```

APK output: `app\build\outputs\apk\debug\app-debug.apk`

### 4.3. Install + launch the app

```powershell
$adb = "C:\Users\Legion\AppData\Local\Android\Sdk\platform-tools\adb.exe"

# check device (must be "device", not "unauthorized")
& $adb devices

# install
& $adb install -r "app\build\outputs\apk\debug\app-debug.apk"

# launch
& $adb shell am start -n com.equinox.uwb/.MainActivity
```

On first launch, grant the **Nearby devices / Ranging** permission.

### 4.4. Requirements

- The phone must run **Android 16** (minSdk 36, uses the `android.ranging` API).
- **Turn off airplane mode** (if on, UWB is disabled → error `Cannot start ranging with tech UWB`).

```powershell
# turn off airplane mode (if needed)
& $adb shell cmd connectivity airplane-mode disable
# verify UWB is enabled
& $adb shell cmd uwb status   # "Uwb is enabled"
& $adb shell cmd uwb enable-uwb
```

---

## 5. Build the UCI Firmware from Source

Source directory: `C:\SDK_Final\SDK\Firmware`

### 5.1. Build for nRF52840DK

```powershell
# 1. Set PYTHONPATH (so CreateTarget.py can import the build library)
$env:PYTHONPATH = "C:\SDK_Final\SDK\Firmware\Projects\Common\scripts"

# 2. Create the target
cd C:\SDK_Final\SDK\Firmware\Projects\FreeRTOS\UCI\nRF52840DK
python CreateTarget.py -build Release

# 3. Build
cd C:\SDK_Final\SDK\Firmware\BuildOutput\UCI\FreeRTOS\nRF52840DK\Release
mingw32-make -j
```

Hex output: `...\BuildOutput\UCI\FreeRTOS\nRF52840DK\Release\nRF52840DK-UCI-FreeRTOS.hex`

### 5.2. Build for DWM3001CDK

```powershell
$env:PYTHONPATH = "C:\SDK_Final\SDK\Firmware\Projects\Common\scripts"

cd C:\SDK_Final\SDK\Firmware\Projects\FreeRTOS\UCI\DWM3001CDK
python CreateTarget.py -build Release

cd C:\SDK_Final\SDK\Firmware\BuildOutput\UCI\FreeRTOS\DWM3001CDK\Release
mingw32-make -j
```

Hex output: `...\BuildOutput\UCI\FreeRTOS\DWM3001CDK\Release\DWM3001CDK-UCI-FreeRTOS.hex`

### 5.3. General form

```powershell
$env:PYTHONPATH = "C:\SDK_Final\SDK\Firmware\Projects\Common\scripts"
python CreateTarget.py -build Release   # run inside Projects\FreeRTOS\<example>\<board>
mingw32-make -j                          # run inside BuildOutput\<example>\FreeRTOS\<board>\Release
```

- `-build` can be `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`, `Custom`.
- Use `mingw32-make` (if you want `make`, copy `mingw32-make.exe` → `make.exe`).

### 5.4. Important: DW3000 vs DW3720

Each board corresponds to one UWB chip type:

| Board | Chip | `USE_DRV_DW3000` | `USE_DRV_DW3720` |
|---|---|---|---|
| DWM3001CDK | DW3000 | 1 | 0 |
| nRF52840DK + DWM3000EVB | DW3000 | **1** | **0** |
| nRF52840DK + QM33120WDK1 | DW3720 | 0 | 1 |

The SDK defaults `nRF52840DK` to DW3720. If using the **DWM3000EVB (DW3000)**, edit
`Projects\FreeRTOS\UCI\nRF52840DK\project_UCI.cmake`:

```cmake
set(USE_DRV_DW3000 1)
set(USE_DRV_DW3720 0)
```

then rebuild (flashing the DW3720 hex onto a DW3000 board causes UCI timeout; the UWB stack never comes up).

---

## 6. Flashing the Firmware

- **DWM3001CDK**: plug USB into **J9** (far from antenna) → drag-and-drop the `.hex` onto the `JLINK` drive, or use J-Link.
- **nRF52840DK**: plug USB into **J2** (short side) → drag-and-drop the `.hex` onto the `JLINK` drive.

After flashing: unplug, reconnect to the communication port (J20 / J3) to run the Python script.

---

## 7. Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| `ModuleNotFoundError: No module named 'uci'` | using Python 3.14 | use Python 3.10 + set `PYTHONPATH` |
| `UciComError.TimeoutError` | wrong firmware (DW3720 on a DW3000 board), or no UCI flashed, or wrong port | flash the correct UCI hex, check the port |
| App shows `Not started`, logcat `Cannot start ranging with tech UWB` | airplane mode on → UWB disabled | turn off airplane mode / enable UWB |
| `RangingRxTimeout (0x21)` / `distance 65535` | weak link, too close, antenna misaligned | keep 0.5–1 m, aim antennas, ensure 3 anchors |
| App updates distance slowly (10–15 s) | app has sensor fusion + AoA enabled | disable `setSensorFusionEnabled` + `setAngleOfArrivalNeeded` in `Uwb.kt` |
