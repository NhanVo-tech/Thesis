# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2
add_library(fira STATIC)

target_sources(fira PRIVATE fira_dw3000.c common_fira.c)

if(NOT USE_NIQ)
  target_sources(fira PRIVATE fira_app.c fira_fn.c)
  target_link_libraries(fira PUBLIC reporter Config)
endif()

target_include_directories(fira PUBLIC ./ ../../Inc ../common Inc)

target_link_libraries(fira PUBLIC HAL UWB uwbstack_core Helpers)
