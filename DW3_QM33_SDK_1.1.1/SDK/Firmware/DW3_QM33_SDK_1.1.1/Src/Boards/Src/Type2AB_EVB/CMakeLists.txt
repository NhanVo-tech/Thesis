# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

add_library(platform_l1)

target_sources(
  platform_l1 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/platform_l1_config.c
                     ${CMAKE_CURRENT_LIST_DIR}/../Common/helper_platform_l1_config.c
)

target_include_directories(
  platform_l1 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../Common_OTP ${CMAKE_CURRENT_LIST_DIR}/../Common
)

target_link_libraries(platform_l1 PUBLIC Helpers uwbstack_core)

install(TARGETS platform_l1 EXPORT platform_l1-config)
install(EXPORT platform_l1-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/platform_l1/cmake)

if(USE_BOARD_LIB)
  add_library(Board ${CMAKE_CURRENT_LIST_DIR}/${MY_OS}/Type2AB_EVB.c)

  target_include_directories(
    Board PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Common ${CMAKE_CURRENT_LIST_DIR}/../../Inc
  )

  target_link_libraries(Board app_common platform_l1 HAL)

  install(TARGETS Board EXPORT Board-config)
  install(EXPORT Board-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/Board/cmake)
endif()
