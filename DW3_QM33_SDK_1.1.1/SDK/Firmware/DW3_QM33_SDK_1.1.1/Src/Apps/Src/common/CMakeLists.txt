# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

add_library(
  app_common STATIC
  config/driver_app_config.c
  controlTask/controlTask.c
  flushTask/flushTask.c
  usb_uart/usb_uart_tx.c
  usb_uart/usb_uart_rx.c
  app/common/app.c
  task/task_signal.c
)

if(USE_CLI)
  target_sources(
    app_common PRIVATE cmd/thread_fn.c cmd/cmd.c cmd/cmd_fn.c defaultTask/defaultTask.c
                       cmd/cmd_dw3000.c cmd/cmd_calibration.c
  )
endif()

target_include_directories(
  app_common
  PUBLIC ./cmd
         ./config
         ./controlTask
         ./defaultTask
         ./flushTask
         ./task
         ./usb_uart
         ../../Inc
         ../fira/Inc
         ./app/common
)

target_link_libraries(app_common PUBLIC qplatform Config HAL OSAL Helpers)

if(USE_CLI OR USE_UCI)
  target_link_libraries(app_common PUBLIC reporter)
endif()

if(USE_CLI)
  target_link_libraries(app_common PUBLIC EventManager)
endif()

if(USE_LISTENER)
  target_compile_definitions(app_common PUBLIC LISTENER_ENABLED=1)
endif()

if(CONFIG_SOC_PROTECT)
  target_compile_definitions(app_common PUBLIC CONFIG_SOC_PROTECT)
endif()

add_definitions(-DPROJECT_NAME="${MY_SAMPLE}")

# Prevent optimization of symbols related to the CLI commands
target_link_options(
  app_common
  PUBLIC
  -Wl,--undefined=known_app_fira
  -Wl,--undefined=known_app_listener
  -Wl,--undefined=known_commands_app_start
  -Wl,--undefined=known_commands_anytime_all
  -Wl,--undefined=known_commands_dw3xxx
  -Wl,--undefined=known_commands_idle_uart
  -Wl,--undefined=known_subcommands_listener
  -Wl,--undefined=known_commands_service_all
  -Wl,--undefined=known_commands_set_calibration
)

install(TARGETS app_common EXPORT app_common-config)
install(EXPORT app_common-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/app_common/cmake)
