# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-Qorvo-2
# cmake-lint: disable=E1122

cmake_minimum_required(VERSION 3.13)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../tools/cmake)
include(helpers)

if(CONFIG_QPLATFORM_IMPL_QM33_QHAL_ZEPHYR OR CONFIG_QPLATFORM_IMPL_QM33_QHAL_NON_ZEPHYR)

  add_library(qplatform STATIC)

  target_include_directories(qplatform PUBLIC include)
  target_link_libraries(qplatform PUBLIC qosal)

  target_sources(qplatform PRIVATE qm33_qhal_common/src/qplatform.c)
  target_include_directories(qplatform PRIVATE qm33_qhal_common/include)

  if(USE_DRV_DW3000)
    set(CONFIG_DW3000 ${USE_DRV_DW3000})
    target_compile_definitions(qplatform PRIVATE CONFIG_DW3000=${USE_DRV_DW3000})
  endif()

  if(USE_DRV_DW3720)
    set(CONFIG_DW3720 ${USE_DRV_DW3720})
    target_compile_definitions(qplatform PRIVATE CONFIG_DW3720=${USE_DRV_DW3720})
  endif()

  if(CONFIG_QPLATFORM_IMPL_QM33_QHAL_ZEPHYR)
    target_sources(qplatform PRIVATE qm33_qhal_zephyr/src/qplatform.c)
  else()
    target_sources(qplatform PRIVATE qm33_qhal_non_zephyr/src/qplatform.c)
  endif()

  if(NOT BUILD_TESTING AND CONFIG_UWB_DRIVER_DW3000)
    target_link_libraries(qplatform PUBLIC dwt_driver)
  endif()

  # Isolate qplatform_deca_impl.c as it is required by dwt_driver.
  add_library(
    qplatform_deca_compat STATIC EXCLUDE_FROM_ALL qm33_qhal_common/src/qplatform_deca_impl.c
  )
  target_include_directories(qplatform_deca_compat PRIVATE qm33_qhal_common/include)
  target_link_libraries(
    qplatform_deca_compat
    PUBLIC qosal
    PRIVATE qplatform
  )
  install(
    TARGETS qplatform_deca_compat
    OPTIONAL
    EXPORT qplatform_deca_compat-config
    ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qplatform_deca_compat
            FILE_SET HEADERS
            DESTINATION include/qplatform_deca_compat
  )
  install(EXPORT qplatform_deca_compat-config
          DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qplatform_deca_compat/cmake
  )
  if(NOT CONFIG_UWB_DRIVER_DW3000)
    # That link is necessary when building drivers from outside of uwb-stack.
    target_link_libraries(qplatform PUBLIC qplatform_deca_compat)
  endif()

else()
  add_library(qplatform INTERFACE)
  target_include_directories(qplatform INTERFACE include)
  target_link_libraries(qplatform INTERFACE qosal)
endif()

install(
  TARGETS qplatform
  OPTIONAL
  EXPORT qplatform-config
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qplatform
          FILE_SET HEADERS
          DESTINATION include/qplatform
)
install(EXPORT qplatform-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qplatform/cmake)
