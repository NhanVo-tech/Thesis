# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

if(PROJECT_FP STREQUAL "hard")
  set(STACK_FP "hfp")
elseif(PROJECT_FP STREQUAL "softfp")
  set(STACK_FP "sfp")
elseif(PROJECT_FP STREQUAL "soft")
  set(STACK_FP "sfp")
endif()

add_library(niq STATIC IMPORTED)
set_target_properties(
  niq
  PROPERTIES IMPORTED_CONFIGURATIONS "Debug;Release"
             IMPORTED_LOCATION
             "${CMAKE_CURRENT_LIST_DIR}/../../../${LIBS_PATH}/niq/libniq-m4-${STACK_FP}-2.1.0.0.a"
             INTERFACE_INCLUDE_DIRECTORIES
             "${CMAKE_CURRENT_LIST_DIR}/../../../${LIBS_PATH}/niq/Inc"
)
