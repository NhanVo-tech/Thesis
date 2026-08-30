# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-Qorvo-2
# cmake-lint: disable=E1122

cmake_minimum_required(VERSION 3.13)

set(CONFIG_MANAGER_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
list(APPEND CMAKE_MODULE_PATH ${CONFIG_MANAGER_ROOT}/../tools/cmake)

set(CONFIG_GENERATOR_PATH ${CONFIG_MANAGER_ROOT}/tools/config_plugin_gen)
set(GENERATED_FILES_DIR ${CMAKE_BINARY_DIR}/generated)

set(MODEL_FILE_PATH ${CONFIG_MANAGER_ROOT}/data/l1_config/l1_config.yml)

set(POST_TX_TONE "$<BOOL:${CONFIG_UWB_MCPS_POST_TX_TONE}>")
set(CHIP_QM33 "$<BOOL:${CONFIG_UWB_CHIP_API_DW3000}>")

set(L1_CONFIG_HEADERS ${GENERATED_FILES_DIR}/include/l1_config_getter.h
                      ${GENERATED_FILES_DIR}/include/l1_config_types.h
)
set(L1_CONFIG_SOURCES ${GENERATED_FILES_DIR}/src/l1_config_keys.c
                      ${GENERATED_FILES_DIR}/src/l1_config_keys.h
)

set(GEN_DEPENDENCY_FILES
    ${MODEL_FILE_PATH}
    ${CONFIG_GENERATOR_PATH}/config_plugin_gen/models.py
    ${CONFIG_GENERATOR_PATH}/config_plugin_gen/generator.py
    ${CONFIG_GENERATOR_PATH}/config_plugin_gen/templates/macros.h.j2
    ${CONFIG_GENERATOR_PATH}/config_plugin_gen/templates/l1_config_getter.h.j2
    ${CONFIG_GENERATOR_PATH}/config_plugin_gen/templates/l1_config_types.h.j2
    ${CONFIG_GENERATOR_PATH}/config_plugin_gen/templates/l1_config_keys.c.j2
    ${CONFIG_GENERATOR_PATH}/config_plugin_gen/templates/l1_config_keys.h.j2
)

if(CONFIG_L1_CONFIG_OLD_ANTENNA_MANAGEMENT_MODEL)
  set(CONFIG_L1_CONFIG_ANT_PAIR_NUM 0)
endif()

if(DEFINED CONFIG_L1_CONFIG_ANT_NUM) # May be false when first entered in utest build
  add_library(l1_config_itf INTERFACE ${L1_CONFIG_HEADERS} ${L1_CONFIG_SOURCES})
  set_target_properties(
    l1_config_itf
    PROPERTIES ANT_NUM ${CONFIG_L1_CONFIG_ANT_NUM}
               ANT_SET_NUM ${CONFIG_L1_CONFIG_ANT_SET_NUM}
               PDOA_LUT_NUM ${CONFIG_L1_CONFIG_PDOA_LUT_NUM}
               ANT_PAIR_NUM ${CONFIG_L1_CONFIG_ANT_PAIR_NUM}
  )
endif()

qorvo_run_poetry_based_generator(
  NAME
  config-plugin-gen
  PATH
  ${CONFIG_GENERATOR_PATH}
  DEPENDS
  ${GEN_DEPENDENCY_FILES}
  OUTPUT
  ${L1_CONFIG_HEADERS}
  ${L1_CONFIG_SOURCES}
  COMMENT
  "Generate the l1_config plugin based on its model"
  ARGS
  ${MODEL_FILE_PATH}
  ${GENERATED_FILES_DIR}
  -DCONFIG_L1_CONFIG_ANT_NUM=$<TARGET_PROPERTY:l1_config_itf,ANT_NUM>
  -DCONFIG_L1_CONFIG_ANT_SET_NUM=$<TARGET_PROPERTY:l1_config_itf,ANT_SET_NUM>
  -DCONFIG_L1_CONFIG_PDOA_LUT_NUM=$<TARGET_PROPERTY:l1_config_itf,PDOA_LUT_NUM>
  -DCONFIG_L1_CONFIG_ANT_PAIR_NUM=$<TARGET_PROPERTY:l1_config_itf,ANT_PAIR_NUM>
  $<${POST_TX_TONE}:-DCONFIG_UWB_MCPS_POST_TX_TONE=1>
  $<${CHIP_QM33}:-DCONFIG_UWB_CHIP_API_DW3000=1>
)

target_compile_definitions(
  l1_config_itf
  INTERFACE CONFIG_L1_CONFIG_ANT_NUM=${CONFIG_L1_CONFIG_ANT_NUM}
            CONFIG_L1_CONFIG_ANT_SET_NUM=${CONFIG_L1_CONFIG_ANT_SET_NUM}
            CONFIG_L1_CONFIG_PDOA_LUT_NUM=${CONFIG_L1_CONFIG_PDOA_LUT_NUM}
            CONFIG_L1_CONFIG_ANT_PAIR_NUM=${CONFIG_L1_CONFIG_ANT_PAIR_NUM}
)

# Include generated headers to the generated installed library
target_sources(
  l1_config_itf
  INTERFACE FILE_SET
            HEADERS
            TYPE
            HEADERS
            BASE_DIRS
            ${GENERATED_FILES_DIR}/include
            FILES
            ${L1_CONFIG_HEADERS}
)

# Include headers to the generated installed library
target_sources(
  l1_config_itf
  INTERFACE FILE_SET
            HEADERS
            TYPE
            HEADERS
            BASE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            FILES
            include/l1_config_key_access.h
            include/l1_config.h
)

# Set specifically the build interface for the build, and the install for the installed library
target_include_directories(
  l1_config_itf
  INTERFACE # Relative path are forbidden in BUILD_INTEFACE generator
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${GENERATED_FILES_DIR}/include>
            $<INSTALL_INTERFACE:include/l1_config_itf>
)

# Install the library and the headers in the destination directory
install(
  TARGETS l1_config_itf
  OPTIONAL
  EXPORT l1_config_itf-config
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/l1_config_itf
          FILE_SET HEADERS
          DESTINATION include/l1_config_itf
)

# Install the cmake file used to import the library
install(EXPORT l1_config_itf-config
        DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/l1_config_itf/cmake
)

add_library(l1_config STATIC)
set_property(TARGET l1_config PROPERTY EXCLUDE_FROM_ALL TRUE)

target_sources(l1_config PRIVATE src/l1_config.c ${GENERATED_FILES_DIR}/src/l1_config_keys.c)

if(NOT CONFIG_L1_CONFIG_VOLATILE)
  target_sources(l1_config PRIVATE src/l1_config_custom.c)
endif()

target_include_directories(
  l1_config PRIVATE src ${GENERATED_FILES_DIR}/src
                    ${CONFIG_MANAGER_ROOT}/src # for config_manager_internal.h
)

target_compile_definitions(
  l1_config
  PRIVATE "CONFIG_L1_CONFIG_CUSTOM_DEFAULT_USE_DEDICATED_SECTION=\
          ${CONFIG_L1_CONFIG_CUSTOM_DEFAULT_USE_DEDICATED_SECTION}"
          "CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SHA256_SIZE=\
          ${CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SHA256_SIZE}"
          "CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SIZE=\
          ${CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SIZE}"
)

if(CONFIG_UWB_CHIP_API_QM35)
  target_link_libraries(l1_config PUBLIC $<$<BOOL:${CONFIG_UWB_DRIVER_QM35}>:peg_driver_itf>)
  if(CONFIG_L1_CONFIG_CUSTOM_DEFAULT_USE_DEDICATED_SECTION)
    qlinker_sources(SECTIONS src/linker/l1_config_fragment.ld)
  endif()
endif()

if(CONFIG_L1_CONFIG_CUSTOM_DEFAULT OR NOT CONFIG_L1_CONFIG_VOLATILE)
  target_link_libraries(l1_config PRIVATE qhal)
endif()
target_link_libraries(l1_config PUBLIC l1_config_itf config_manager qosal qtils)

install(
  TARGETS l1_config
  OPTIONAL
  EXPORT l1_config-config
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/l1_config
          FILE_SET HEADERS
          DESTINATION include/l1_config
)

install(EXPORT l1_config-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/l1_config/cmake)
