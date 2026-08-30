# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

set(MY_BOARD Type2AB_EVB)
set(MY_OS FreeRTOS)
set(MY_SAMPLE "QANI") # Sample name consitent with the sample directory name
set(MY_HAL nrfx)
set(MY_BSP Nordic)
set(MY_CPU NRF52840_XXAA)
set(MY_LD_FILE nRF52840.ld)
set(CMAKE_CUSTOM_C_FLAGS
    "-Werror \
    -DBOARD_CUSTOM \
    -DCONFIG_GPIO_AS_PINRESET \
    -D${MY_CPU} \
    -DUSE_NIQ \
    -DCONFIG_FOR_TYPE2AB_EVB_ONLY"
)

# Add Soft Device flags
string(
  CONCAT
    CMAKE_CUSTOM_C_FLAGS
    ${CMAKE_CUSTOM_C_FLAGS}
    " -DNRF_SD_BLE_API_VERSION=7 \
      -DS113 \
      -DSOFTDEVICE_PRESENT \
      -DINITIALIZE_USER_SECTIONS \
      -DNO_VTOR_CONFIG \
      -DUSE_RTT_PRINT=1"
)

# Add Soft Device debug flag
string(CONCAT CMAKE_CUSTOM_C_FLAGS ${CMAKE_CUSTOM_C_FLAGS} " -DDEBUG")

# Add Soft Device flags
string(CONCAT CMAKE_ASM_FLAGS ${CMAKE_ASM_FLAGS} " -DDBOARD_CUSTOM \
                                                   -DS113 \
                                                   -DSOFTDEVICE_PRESENT"
)

set(MY_TARGET ${MY_BOARD}-${MY_SAMPLE}-${MY_OS})

add_definitions(-Os)

# Processor related defines
set(PROJECT_ARCH "m4")
set(PROJECT_FP "hard")
set(PROJECT_FPU "fpv4-sp-d16")

set(USE_DRV_DW3000 0)
set(USE_DRV_DW3720 1)

set(USE_CRYPTO_BACKEND_MBEDTLS 1)
set(USE_CRYPTO_SPEED_OPTIMIZATION 0)

if(USE_DRV_DW3000 AND USE_DRV_DW3720)
  message(
    WARNING
      "Enabling DW3000 and DW3720 drivers at the same time is allowed for nRF52840, as the proper \
                     driver will be selected at the runtime. However this will increase the binary size."
  )
endif()
