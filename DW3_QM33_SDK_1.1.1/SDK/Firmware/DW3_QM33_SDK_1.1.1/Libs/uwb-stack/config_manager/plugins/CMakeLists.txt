# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-Qorvo-2

cmake_minimum_required(VERSION 3.13)

add_library(plugins_itf INTERFACE)

add_subdirectory(mac_config)
target_link_libraries(plugins_itf INTERFACE mac_config_itf)

add_library(plugins INTERFACE)
target_link_libraries(plugins INTERFACE mac_config)

if(CONFIG_UWB_MCPS_LLHW)
  add_subdirectory(l1_config)
  target_link_libraries(plugins_itf INTERFACE l1_config_itf)
  target_link_libraries(plugins INTERFACE l1_config)
endif()

if(COMMAND qorvo_install_target)
  qorvo_install_target(plugins_itf)
endif()
