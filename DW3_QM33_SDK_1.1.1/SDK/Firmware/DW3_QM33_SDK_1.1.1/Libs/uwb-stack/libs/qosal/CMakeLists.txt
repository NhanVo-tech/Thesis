# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-Qorvo-2
# cmake-lint: disable=E1122

cmake_minimum_required(VERSION 3.13)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../tools/cmake)
include(helpers)

# Declares the interface alone.
add_library(qosal_itf INTERFACE)
target_include_directories(qosal_itf INTERFACE include)

if(CONFIG_QOSAL_IMPL_ZEPHYR)
  target_include_directories(qosal_itf INTERFACE include/zephyr)
  target_compile_definitions(qosal_itf INTERFACE QOSAL_ENABLED)

  add_library(
    qosal STATIC EXCLUDE_FROM_ALL
    src/qerr_common.c
    src/qmalloc.c
    src/zephyr/qerr.c
    src/zephyr/qirq.c
    src/zephyr/qlog.c
    src/zephyr/qmalloc.c
    src/zephyr/qprofiling.c
    src/zephyr/qmsg_queue.c
    src/zephyr/qmutex.c
    src/zephyr/qpm.c
    src/zephyr/qrand.c
    src/zephyr/qsemaphore.c
    src/zephyr/qsignal.c
    src/zephyr/qthread.c
    src/zephyr/qtime.c
    src/zephyr/qworkqueue.c
    src/zephyr/qtracing.c
  )
  target_link_libraries(qosal PUBLIC qosal_itf)
  target_link_libraries(
    qosal
    PUBLIC zephyr_interface
    # TODO kernel dependency is required as qosal is not exactly a zephyr library and must depends
    # on zephyr_generated_headers library for offsets.h
    PRIVATE qhal kernel
  )
  target_compile_options(qosal PRIVATE -Wno-error=type-limits)

elseif(CONFIG_QOSAL_IMPL_FREERTOS)
  target_include_directories(qosal_itf INTERFACE include/freertos)
  target_compile_definitions(qosal_itf INTERFACE QOSAL_ENABLED)

  add_library(
    qosal STATIC EXCLUDE_FROM_ALL
    src/qerr_common.c
    src/qmalloc.c
    src/freertos/qerr.c
    src/freertos/qirq.c
    src/freertos/qmalloc.c
    src/freertos/qprofiling.c
    src/freertos/qmsg_queue.c
    src/freertos/qmutex.c
    src/freertos/qpm.c
    src/freertos/qprivate.c
    src/freertos/qrand.c
    src/freertos/qsemaphore.c
    src/freertos/qsignal.c
    src/freertos/qthread.c
    src/freertos/qtime.c
    src/freertos/qworkqueue.c
    src/freertos/qtracing.c
    src/freertos/qos.c
  )
  target_link_libraries(qosal PUBLIC qosal_itf)
  target_include_directories(qosal PRIVATE src/freertos)

else()
  target_include_directories(qosal_itf INTERFACE include/dummy)

  add_library(qosal INTERFACE)
  target_link_libraries(qosal INTERFACE qosal_itf)
endif()

install(
  TARGETS qosal_itf
  EXPORT qosal_itf-config
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qosal_itf
          FILE_SET HEADERS
          DESTINATION include/qosal_itf
)

install(EXPORT qosal_itf-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qosal_itf/cmake)

install(
  TARGETS qosal
  EXPORT qosal-config
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qosal
          FILE_SET HEADERS
          DESTINATION include/qosal
)

install(EXPORT qosal-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qosal/cmake)
