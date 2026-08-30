# Hello World Example

This example demonstrates a simple interaction with QM33 UWB transceiver chip using Qorvo libraries. The application initializes the UWB stack, retrieves device information, and transmits it over RTT.

## Requirements

The sample supports the following development kits:

| Board                              | Support                            |
|------------------------------------|------------------------------------|
| nRF52840DK with QM33 shield        | Yes                                |
| DWM3001CDK                         | Yes                                |

## Overview

The application performs the following steps:

1. Initializes the platform and UWB stack.
2. Retrieves device information.
3. Transmits initial message "Hello World" over RTT.
4. Transmits device information over RTT.
5. Repeats the device information transmission every second.

## Source Code

### hello_world.c

The source code includes the following key components:

- **Includes**: Necessary headers for Qorvo and Nordic libraries.
- **Macros**: Defines the stack size for the Hello World task.
- **External Declarations**: Declares external platform operations.
- **Functions**:
  - `uwb_stack_init`: Initializes the UWB stack.
  - `uwb_stack_deinit`: Deinitializes the UWB stack.
  - `hello_world_task`: Main task that initializes the UWB stack, retrieves device info, and transmits it over RTT.
  - `hello_world_init`: Initializes the Hello World task.

## Building and Running

1. Follow SDK documentation on building

2. **Monitor the RTT output**:
    Use a RTT_Viewer to connect to the RTT interface and observe the output.

## Sample Output
```
[00:00:00.013,696] <info> app: Hello World!
[00:00:01.033,903] <info> app: Qorvo Device ID = 0xdeca0302
[00:00:01.033,907] <info> app: Qorvo Lot ID = 0x00005056474d3932
[00:00:01.033,911] <info> app: Qorvo Part ID = 0x10e146ab
[00:00:01.033,915] <info> app: Qorvo SoC ID = 00005056474d393210e146ab
