# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

set(MY_BOARD Type2AB_EVB)
set(MY_OS FreeRTOS)
set(MY_SAMPLE "CLI") # Sample name consitent with the sample directory name
set(MY_HAL nrfx)
set(MY_BSP Nordic)
set(MY_CPU NRF52840_XXAA)
set(MY_LD_FILE nRF52840.ld)

set(CMAKE_CUSTOM_C_FLAGS
    "-Werror \
    -DBOARD_CUSTOM \
    -DUSB_ENABLE \
    -DCONFIG_GPIO_AS_PINRESET \
    -DCONFIG_FOR_TYPE2AB_EVB_ONLY"
)

set(MY_TARGET ${MY_BOARD}-${MY_SAMPLE}-${MY_OS})

add_definitions(-Os)

# Enable USE_USB_ENUM_WORKAROUND to workaround long USB enumeration. If HDK is battery supplied, the
# flag USB_ENUM_WORKAROUND should be removed.
add_definitions(-DUSE_USB_ENUM_WORKAROUND)

# Processor related defines
set(PROJECT_ARCH "m4")
set(PROJECT_FP "hard")
set(PROJECT_FPU "fpv4-sp-d16")

# Used Dw Driver
set(USE_DRV_DW3000 0)
set(USE_DRV_DW3720 1)

set(USE_CRYPTO_BACKEND_MBEDTLS 1)
set(USE_CRYPTO_SPEED_OPTIMIZATION 0)
