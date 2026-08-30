# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-Qorvo-2
# cmake-lint: disable=E1122

cmake_minimum_required(VERSION 3.13)

if(CONFIG_QHAL_IMPL_ZEPHYR OR CONFIG_QHAL_IMPL_NRFX)
  add_library(qhal STATIC)

  target_include_directories(qhal PUBLIC include)
  target_link_libraries(qhal PUBLIC qosal)

  target_sources(qhal PRIVATE src/qpwr.c src/qrtc.c)

  if(CONFIG_QHAL_IMPL_NRFX) # QHAL for NRFX
    target_include_directories(qhal PUBLIC src/nrfx)
    target_sources(
      qhal
      PRIVATE src/nrfx/qgpio.c
              src/nrfx/qspi.c
              src/nrfx/qtimer.c
              src/nrfx/qrtc_share.c
              src/nrfx/persistent_time.c
              src/nrfx/qflash.c
              src/qm33/qpwr.c
              src/qm33/qotp.c
    )
  elseif(CONFIG_QHAL_IMPL_ZEPHYR)
    target_sources(qhal PRIVATE src/zephyr/qgpio.c src/zephyr/qspi.c src/zephyr/qtimer.c)

    target_sources(qhal PRIVATE src/zephyr/qflash.c)
    target_link_libraries(qhal PUBLIC zephyr_interface)

    target_sources(qhal PRIVATE src/zephyr/qspi.c src/qm33/qpwr.c src/qm33/qotp.c)
  endif()

else() # not CONFIG_QHAL
  add_library(qhal INTERFACE)
  target_include_directories(qhal INTERFACE include include/dummy)
  target_link_libraries(qhal INTERFACE qosal)
endif()

install(
  TARGETS qhal
  EXPORT qhal-config
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qhal
          FILE_SET HEADERS
          DESTINATION include/qhal
)
install(EXPORT qhal-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qhal/cmake)
