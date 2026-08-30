# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

add_library(logger)

if(CONFIG_LOG)
  target_sources(logger PRIVATE log_processing.c)
endif()

target_include_directories(logger PUBLIC .)
target_include_directories(logger PRIVATE ../Boards/Inc)

target_link_libraries(logger HAL log_rtt)
