# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-Qorvo-2
# cmake-lint: disable=E1122

cmake_minimum_required(VERSION 3.13)

add_library(mac_config_itf INTERFACE)
target_include_directories(mac_config_itf INTERFACE include)

if(COMMAND qorvo_install_target)
  install(
    TARGETS mac_config_itf
    OPTIONAL
    EXPORT mac_config_itf-config
    ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/mac_config_itf
            FILE_SET HEADERS
            DESTINATION include/mac_config_itf
  )
  install(EXPORT mac_config_itf-config
          DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/mac_config_itf/cmake
  )

endif()

add_library(mac_config INTERFACE)
target_link_libraries(mac_config INTERFACE mac_config_itf)

if(COMMAND qorvo_install_target)
  install(
    TARGETS mac_config
    OPTIONAL
    EXPORT mac_config-config
    ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/mac_config
            FILE_SET HEADERS
            DESTINATION include/mac_config
  )

  install(EXPORT mac_config-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/mac_config/cmake)

endif()
