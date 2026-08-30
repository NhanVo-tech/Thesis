# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

cmake_minimum_required(VERSION 3.13)

set(QOSAL_SOURCES_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../../${LIBS_PATH}/uwb-stack/libs/qosal")

add_subdirectory(${QOSAL_SOURCES_PATH} qosal)

target_link_libraries(qosal PUBLIC qhal)

if(CONFIG_LOG)
  target_link_libraries(qosal PUBLIC log_rtt)
endif()

target_compile_definitions(qosal PRIVATE CONFIG_QOSAL_MAX_MUTEX=8 CONFIG_QOSAL_PROFILING_STATS=1)

add_library(OSAL INTERFACE)

target_link_libraries(OSAL INTERFACE OS qosal)

target_compile_options(OSAL INTERFACE -Wl,--wrap=_malloc_r -Wl,--wrap=_free_r -Wl,--wrap=_calloc_r)

install(TARGETS OSAL EXPORT OSAL-config)
install(EXPORT OSAL-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/OSAL/cmake)
