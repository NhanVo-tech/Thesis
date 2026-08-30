# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

if(PROJECT_FP STREQUAL "hard")
  set(STACK_FP "hfp")
elseif(PROJECT_FP STREQUAL "softfp")
  set(STACK_FP "sfp")
  set(PROJECT_FP "soft")
elseif(PROJECT_FP STREQUAL "soft")
  set(STACK_FP "sfp")
endif()

set(QORVO_DISABLE_KCONFIG ON)
# Check if it is UCI or CLI build
if(USE_UCI)
  set(CONFIG_UCI
      ON
      CACHE BOOL "activate UCI" FORCE
  )
  set(CONFIG_UCI_CORE
      ON
      CACHE BOOL "activate UCI Core" FORCE
  )
  set(CONFIG_UCI_BACKENDS_DEPS
      ON
      CACHE BOOL "activate UCI backends deps" FORCE
  )
  set(CONFIG_UCI_BACKEND_CORE
      ON
      CACHE BOOL "activate UCI CORE backend" FORCE
  )
  set(CONFIG_UCI_BACKEND_CONFIG_MANAGER
      ON
      CACHE BOOL "activate UCI backend config manager" FORCE
  )
else()
  set(CONFIG_UCI
      OFF
      CACHE BOOL "disable UCI" FORCE
  )
  set(CONFIG_UCI_CORE
      OFF
      CACHE BOOL "disable UCI Core" FORCE
  )
  set(CONFIG_UCI_BACKENDS_DEPS
      OFF
      CACHE BOOL "disable UCI backends deps" FORCE
  )
  set(CONFIG_UCI_BACKEND_CORE
      OFF
      CACHE BOOL "disable UCI CORE backend" FORCE
  )
  set(CONFIG_UCI_BACKEND_CONFIG_MANAGER
      OFF
      CACHE BOOL "disable UCI backend config manager" FORCE
  )
endif()

if(CONFIG_LOG)
  add_definitions(-DCONFIG_LOG=1)
endif()

set(CMAKE_LIBRARY_ARCHITECTURE "arm-cortex-${PROJECT_ARCH}-${PROJECT_FP}_floating")
# Check if we need fira only
if(USE_UCI OR USE_PCTT)
  message("Using full uwbstack")
  set(UWBSTACK_LIBS_PATH
      "${CMAKE_CURRENT_LIST_DIR}/../../../${LIBS_PATH}/uwbstack_libs/delivery/full/Release"
  )
else()
  message("Using Fira only uwbstack")
  set(UWBSTACK_LIBS_PATH
      "${CMAKE_CURRENT_LIST_DIR}/../../../${LIBS_PATH}/uwbstack_libs/delivery/fira/Release"
  )
endif()

# Define the path where find_package should look for libs
set(CMAKE_PREFIX_PATH "${UWBSTACK_LIBS_PATH}")

# List packages, order is important for some
find_package(uwbstack_bundle REQUIRED)
add_library(uwbstack_core ALIAS uwbstack_bundle)

if(USE_UCI)
  find_package(uci_bundle QUIET)
  add_library(uwbstack_uci ALIAS uci_bundle)
endif()
