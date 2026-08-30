# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

set(UWBSTACK_DIR "${CMAKE_CURRENT_LIST_DIR}/../../${LIBS_PATH}/uwb-stack/")
set(CONFIG_MANAGER_SOURCES_PATH "${UWBSTACK_DIR}/config_manager")
set(QMATH_PATH "${UWBSTACK_DIR}/libs/qmath")

add_subdirectory("${QMATH_PATH}" qmath)

add_library(UWB STATIC uwb_translate.c uwb_utils.c mcps_crypto.c)
target_include_directories(UWB PUBLIC Inc)
add_compile_definitions(CONFIG_UWB_MCPS_STACK_SIZE=2048)

target_link_libraries(
  UWB
  PUBLIC qosal
         uwbdriver
         ProjectDefinition
         qhal
         uwbstack_core
)

target_link_options(UWB PUBLIC -Wl,--undefined=crypto_init)

install(TARGETS UWB EXPORT UWB-config)
install(EXPORT UWB-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/UWB/cmake)
