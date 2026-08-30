# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

set(NRFX_DIR ../nrfx)

add_library(
  HAL ${NRFX_DIR}/HAL_crypto.c ${NRFX_DIR}/HAL_watchdog.c ${NRFX_DIR}/HAL_uart.c
      ${NRFX_DIR}/HAL_error.c ${NRFX_DIR}/HAL_usb.c ${NRFX_DIR}/HAL_cpu.c
)

target_include_directories(HAL PUBLIC ../../Inc)
target_include_directories(HAL PUBLIC ../../../Boards/Src/${MY_BOARD}/Common)

target_link_libraries(HAL UWB SDK OSAL Helpers Interface)

# Add used dw driver
if(USE_DRV_DW3000 EQUAL 1)
  target_compile_definitions(HAL PUBLIC USE_DRV_DW3000)
endif()

if(USE_DRV_DW3720 EQUAL 1)
  target_compile_definitions(HAL PUBLIC USE_DRV_DW3720)
endif()

math(EXPR NB_DW_DRIVERS "${USE_DRV_DW3000} + ${USE_DRV_DW3720}")

if(NB_DW_DRIVERS EQUAL 0)
  message(FATAL_ERROR "At least one dw driver should be defined.")
else()
  math(EXPR NB_DW_DRIVERS "${USE_DRV_DW3000}+${USE_DRV_DW3720}")
  add_definitions(-DNB_DW_DRIVERS=${NB_DW_DRIVERS})
endif()

install(
  TARGETS HAL
  OPTIONAL
  EXPORT HAL-config
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/HAL
          FILE_SET HEADERS
          DESTINATION include/HAL
)

install(EXPORT HAL-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/HAL/cmake)
