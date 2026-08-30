# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-Qorvo-2
# cmake-lint: disable=E1122

cmake_minimum_required(VERSION 3.13)
project(config_manager)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../tools/cmake)
include(helpers)

add_subdirectory(plugins)

# Declares the config manager interface alone.
add_library(config_manager_itf INTERFACE)
install(
  TARGETS config_manager_itf
  OPTIONAL
  EXPORT config_manager_itf-config
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/config_manager_itf
          FILE_SET HEADERS
          DESTINATION include/config_manager_itf
)

# Install the cmake file used to import the library
install(EXPORT config_manager_itf-config
        DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/config_manager_itf/cmake
)

if(CONFIG_L1_CONFIG_VOLATILE)
  add_library(config_manager INTERFACE)
  target_link_libraries(config_manager INTERFACE config_manager_itf)
else()
  add_library(config_manager STATIC)
  target_include_directories(config_manager PRIVATE src)
  target_sources(config_manager PRIVATE src/sha256.c)
  target_link_libraries(config_manager PRIVATE qhal)
  target_link_libraries(config_manager PUBLIC config_manager_itf)
endif()

set_property(TARGET config_manager PROPERTY EXCLUDE_FROM_ALL TRUE)

install(
  TARGETS config_manager
  OPTIONAL
  EXPORT config_manager-config
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/config_manager
          FILE_SET HEADERS
          DESTINATION include/config_manager
)

# Install the cmake file used to import the library
install(EXPORT config_manager-config
        DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/config_manager/cmake
)
