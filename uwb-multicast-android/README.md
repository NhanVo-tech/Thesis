## UWB Ranging and Trilateration with Android and Multiple Anchors
This repository shows how to do FiRa multicast two-way ranging between a plain UWB-enabled Android phone and multiple DWM3001CDK boards as anchors.
It also includes code to trilaterate noisy distance measurements into an estimated position. The result is that you
can set up multiple boards at fixed locations, and accurately track the position of your phone.

To keep this example minimal, it does not use out-of-band (Bluetooth) for configuration, but relies on hard-coded parameters and addresses.

### Setup
1. Get the [DW3 QM33 SDK 1.0.2](https://www.qorvo.com/products/d/da009780), build the CLI firmware (see below), and flash the boards.
2. Issue the CLI commands below on the boards (use a unique ADDR for each board).
3. Place your boards, measure their position as accurately as possible, and update the `uwbAnchors` list in `Uwb.kt`.
4. Power up your boards.
5. Start the Android app to see ranging results and current position of your phone.

### DWM3001CDK Setup
I set up the boards with the CLI firmware from the DW3 QM33 SDK 1.0.2. There's a newer version available, which I would assume works just as well, but I haven't tried it. Consult the DWM3001CDK Developers Manual for more details on how to work with the boards, firmware, and CLI.

#### Firmware Modification
I made two changes to the CLI firmware.

1. In the 1.0.2 version of the CLI, there's an issue when the board is plugged into a USB wall adapter, which makes ranging very slow and less accurate. See [this post](https://forum.qorvo.com/t/tutorial-how-to-use-dw3-qm33-sdk-to-operate-a-battery-operated-dk-to-take-ranging-measurements-for-evaluation/21845) for details on how to fix that. (Comment out two lines in `DWM3001CDK.c`.)
2. Whenever ranging stops on the Android side, it sends a termination signal to the responders, which causes them to exit responder mode, after which they need to be power cycled to start ranging again. This is of course very inconvenient, so as a workaround, I simply added the code below to the function `fira_session_status_ntf_cb` in `fira_app.c` to restart the session if it's stopped. (Yes, this is **extremely hacky**, but it works. A better solution is welcome!)

```C
if (status->state == QUWBS_FBS_SESSION_STATE_IDLE &&
    status->reason_code == QUWBS_FBS_REASON_CODE_SESSION_STOPPED_DUE_TO_INBAND_SIGNAL) {
    fira_helper_stop_session(&fira_ctx, session_handle);
    fira_helper_start_session(&fira_ctx, session_handle);
}
```

#### CLI Setup
Below are the CLI commands issued on the boards.

_Tell the board to automatically start in responder mode after a power cycle._

`SETAPP RESPF`

_Configure FiRa / Android compatible multicast responder mode. ADDR is the (unique) address of your board. PADDR is the Android side address. You can use any addresses, but they need to match what you set up on the Android side. In my setup, I used addresses 0, 1, and 2 for three boards, and 1729 for the Android side._

`RESPF -CHAN=9 -PRFSET=BPRF4 -PCODE=9 -HOP -MULTI -SLOT=2400 -BLOCK=120 -ROUND=20 -RRU=DSTWR -ID=42 -VUPPER=01:02:03:04:05:06:07:08 -ADDR=0 -PADDR=1729`

_Stop ranging._

`STOP`

_Save parameters._

`SAVE`

### Implementation Details
This demo uses the Ranging module introduced in Android 16 (`android.ranging`). Android also has an older UWB library (`androidx.core.uwb`), but to my knowledge it does not support specifying a local address, which means that it requires reconfiguring the boards (manually or OOB) for each ranging session.

This demo illustrates 2D trilateration, with the assumption that anchors and phone are all at the same height. The code can be extended to 3D, but may lead to reduced accuracy if done naively.

In addition to the distance, the library also provides the angle of arrival (azimuth), but this value is very unreliable in my testing.
