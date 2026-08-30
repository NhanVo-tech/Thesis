# QANI Example

This example implements the **QANI** (Qorvo Nearby Interaction) to enable proximity interaction with nearby devices through **BLE** (Bluetooth Low Energy) and **UWB** (Ultra-Wide-Band) technologies.

## Requirements

The sample supports the following development kits:

| Board                              | Support                            |
|------------------------------------|------------------------------------|
| DWM3001CDK                         | Yes                                |
| nRF52840DK with QM33 shield        | Yes                                |
| Murata Type2AB                     | Yes                                |

An iPhone with U1 chip is required to interact with the device.
The Qorvo Nearby Integration application is available for download on the [App Store](https://apps.apple.com/ie/app/qorvo-nearby-interaction/id1615369084).

## Overview

On the UWB device, on power-on the application perform the following steps:

1. Initalizes the Nearby Interaction.
2. Configures and initializes the BLE stack.

Once the device and the iPhone are paired, the application automatically performs the following steps:

1. Initializes UWB stack and and configures the FiRa session.
3. Starts a TWR with the Apple device.

## Source Code

The QANI example is organized into several key source files, each with a specific responsibility. Below is a breakdown of the major components of the project and their roles in the application.

### 1. `main.c`

- **Purpose**: The main entry point for the QANI application.
- **Key Responsibilities**:
  - Initializes the platform.
  - Initializes the Nearby Interaction Qorvo module.
  - Configures and Initializes the BLE stack.
  - Starts the FreeRTOS scheduler.
- **Functions**:
  - `main()`: The main application function that initializes the necessary tasks and starts the scheduler.

### 2. `ble.c`

- **Purpose**: Handles the BLE stack.
- **Key Responsibilities**:
  - Initializes the BLE stack and configure the device as a Peripheral.
- **Functions**:
  - `ble_init()`: Initializes the BLE stack and configures the device as a Peripheral.

### 3. `fira_niq.c`

- **Purpose**: Initializes the UWB stack and starts the ranging session.
- **Key Responsibilities**:
  - Configures session parameters for FiRa, including device type, role, and communication mode.
- **Functions**:
  - `fira_niq_helper()`: Configures and starts the FiRa session .
  - `fira_niq_terminate()`: Stops the FiRa session.

## Building and Running

### 1. Build and Flash the Firmware

To build and flash the firmware for your device, refer to the **UT-Tag SDK documentation** or the [../../../../README.md](../../../../README.md) for detailed instructions on both processes. Ensure that all dependencies are met and the build environment is properly configured. After building the project, follow the provided instructions to flash the firmware onto your device.

### 2. How to run QANI

To run this sample, start the Qorvo Nearby Interaction application on the iPhone. Power on the development kit flashed with the QANI example.

1. Qorvo Nearby Interaction scans for all accessories with BLE technology and displays detected devices.
2. Press "Connect" for the UWB device to start the pairing.
3. Once paired, TWR between the iPhone and the device is started. A 3D arrow pointing toward the device is displayed. Distance, azimuth and elevation are monitored in real time. Qorvo Nearby Interaction application is able to handle the ranging between one iPhone and multiple QANI devices.

### 3. **Monitor the RTT output**:

> **Note**: By default, the Real-Time Transfer (RTT) interface is disabled in the QANI example to reduce power consumption. To enable RTT logs, set the `CONFIG_LOG` variable to `ON` in the [Common/cmakefiles/Examples-FreeRTOS.cmake](Common/cmakefiles/Examples-FreeRTOS.cmake) file.

To monitor the runtime behavior of the QANI example, use a Real-Time Transfer (RTT) viewer to connect to the RTT interface and observe the output.
- [J-Link RTT Viewer](https://www.segger.com/products/debug-probes/j-link/tools/rtt-viewer/) application can be used for this purpose.

## Sample Output
```
Application: Qorvo Nearby Interaction
BOARD: DWM3001CDK
OS: FreeRTOS
Version: 1.0.0
DW3XXX Device Driver Version 08.07.04
MAC: R12.7.0-332-gb2d95d9ba-dirty
[00:00:00.291,039] <debug> nrf_sdh_freertos: Creating a SoftDevice task.
[00:00:00.292,914] <debug> nrf_sdh_freertos: Enter softdevice_task.
[00:00:00.297,366] <info> app: Slow advertising.
[00:00:02.303,928] <info> app: Connection 0x0 established.
[00:00:02.451,872] <debug> app: PHY update request.
[00:00:02.811,802] <debug> nrf_ble_gatt: Peer on connection 0x0 requested a data length of 251 bytes.
[00:00:02.811,809] <debug> nrf_ble_gatt: Updating data length to 27 on connection 0x0.
[00:00:02.841,760] <debug> nrf_ble_gatt: Data length updated to 27 on connection 0x0.
[00:00:02.841,772] <debug> nrf_ble_gatt: max_rx_octets: 27
[00:00:02.841,779] <debug> nrf_ble_gatt: max_tx_octets: 27
[00:00:02.841,785] <debug> nrf_ble_gatt: max_rx_time: 2120
[00:00:02.841,791] <debug> nrf_ble_gatt: max_tx_time: 2120
[00:00:02.931,537] <debug> nrf_ble_gatt: Peer on connection 0x0 requested an ATT MTU of 293 bytes.
[00:00:02.931,672] <debug> nrf_ble_gatt: Updating ATT MTU to 100 bytes (desired: 100) on connection 0x0.
[00:00:03.471,795] <info> app: Notification is enabled
[00:00:03.531,783] <info> app: App requests accessory config data
[00:00:03.592,255] <info> app: App requests config and start
[00:00:03.592,271] <info> app: Started
{"Block":0, "results":[{"Addr":"0xdc4c","Status":"Err"}]}
[00:00:03.831,174] <info> app: Run calcbandwidthadj for pg_count 178: pg_delay 53
{"Block":0, "results":[{"Addr":"0xdc4c","Status":"Ok","D_cm":39}]}
{"Block":1, "results":[{"Addr":"0xdc4c","Status":"Ok","D_cm":42}]}
{"Block":2, "results":[{"Addr":"0xdc4c","Status":"Ok","D_cm":41}]}
{"Block":3, "results":[{"Addr":"0xdc4c","Status":"Ok","D_cm":41}]}
{"Block":4, "results":[{"Addr":"0xdc4c","Status":"Ok","D_cm":40}]}
```
