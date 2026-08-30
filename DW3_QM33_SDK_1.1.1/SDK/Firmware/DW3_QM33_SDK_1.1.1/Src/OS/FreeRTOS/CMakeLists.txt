# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

set(OS_SRC_PATH ../../../SDK_BSP/Nordic/SDK_17_1_0/external/freertos/source)

add_library(
  OS
  ${OS_SRC_PATH}/croutine.c
  ${OS_SRC_PATH}/event_groups.c
  ${OS_SRC_PATH}/list.c
  ${OS_SRC_PATH}/queue.c
  ${OS_SRC_PATH}/stream_buffer.c
  ${OS_SRC_PATH}/tasks.c
  ${OS_SRC_PATH}/timers.c
  ${OS_SRC_PATH}/../portable/GCC/nrf52/port.c
  ${OS_SRC_PATH}/../portable/CMSIS/nrf52/port_cmsis.c
  ${OS_SRC_PATH}/../portable/CMSIS/nrf52/port_cmsis_systick.c
  ${OS_SRC_PATH}/portable/MemMang/heap_4.c
)

target_include_directories(
  OS PUBLIC ${OS_SRC_PATH}/include ${OS_PUBLIC_INC} ${OS_SRC_PATH}/../portable/GCC/nrf52
            ${OS_SRC_PATH}/../portable/CMSIS/nrf52
)

target_link_libraries(OS SDK)

install(TARGETS OS EXPORT OS-config)
install(EXPORT OS-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/OS/cmake)
