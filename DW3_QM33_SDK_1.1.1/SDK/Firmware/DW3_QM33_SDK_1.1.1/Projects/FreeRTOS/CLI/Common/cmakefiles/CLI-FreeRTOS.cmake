# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

cmake_minimum_required(VERSION 3.19)

enable_language(C ASM)

set(EXECUTABLE ${PROJECT_NAME}.elf)

message("Target is ${MY_TARGET}")

add_executable(${EXECUTABLE} ${PROJECT_COMMON}/main.c ${PROJECT_COMMON}/hooks.c)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(USE_BOARD_LIB ON)
set(USE_DIAG_PRINTF ON)

set(CMAKE_C_FLAGS
    "-mcpu=cortex-${PROJECT_ARCH}\
        -mfpu=${PROJECT_FPU}\
        -mfloat-abi=${PROJECT_FP}\
        -mthumb\
        -specs=nano.specs\
        -specs=nosys.specs\
        -fdata-sections\
        -ffunction-sections\
        -std=c11\
        -Wall\
        -D${MY_CPU}\
        ${CMAKE_CUSTOM_C_FLAGS}"
)

set(CMAKE_ASM_FLAGS
    "-mcpu=cortex-${PROJECT_ARCH}\
        -mfpu=${PROJECT_FPU}\
        -mfloat-abi=${PROJECT_FP}\
        -mthumb\
        -fdata-sections\
        -ffunction-sections\
        -Wall\
        -D${MY_CPU}\
        -D__STACK_SIZE=2048\
        -D__HEAP_SIZE=0\
        ${CMAKE_CUSTOM_C_FLAGS}"
)

# CMAKE_BUILD_TYPE=Debug then add -g3 debug flags to CMAKE_C_FLAGS and CMAKE_ASM_FLAGS
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3")
  set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -g3")
endif()

set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -u _printf_float -Wl,--gc-sections,-T${CMAKE_SOURCE_DIR}/${MY_LD_FILE},-Map=${PROJECT_NAME}.map -Wl,--print-memory-usage"
)

# Flags for stack
set(CONFIG_QOSAL_IMPL_FREERTOS 1)

add_definitions(-DCLI_BUILD)

# define a CLI build
set(USE_CLI 1)

# Non-FiRa applications
set(USE_LISTENER 1)

# MAC regions
set(USE_FIRA 1)

set(CONFIG_QM33 1)

# Disable APPROTECT
set(CONFIG_SOC_PROTECT 0)

# Disable RTT logs for performance reasons
set(CONFIG_LOG OFF)

# 0=NONE 1=ERROR 2=WARNING 3=INFO 4=DEBUG
set(CONFIG_QLOG_LEVEL 3)

# Include external libraries
include(${COMMON_PATH}/uwbstack.cmake)
include(${COMMON_PATH}/dwt_uwbdriver.cmake)

# Include internal libraries
add_subdirectory(ProjectDefinition)
add_subdirectory(${PROJECT_BASE}/SDK_BSP/${MY_BSP} ${MY_BSP})
add_subdirectory(${PROJECT_BASE}/Src/OS/${MY_OS} ${MY_OS})
add_subdirectory(${PROJECT_BASE}/Src/EventManager EventManager)

add_subdirectory(${PROJECT_BASE}/Src/OSAL/Src/${MY_OS} OSAL)
list(APPEND LINK_LIB_LIST "OSAL")

add_subdirectory(${PROJECT_BASE}/Src/Boards/Src/${MY_BOARD} ${MY_BOARD})
list(APPEND LINK_LIB_LIST "Board")

add_subdirectory(${PROJECT_BASE}/Src/HAL/Src/${MY_HAL} HAL)
list(APPEND LINK_LIB_LIST "HAL")

# set l1 config needs (used on Src/UWB/CMakelists.txt)
set(CONFIG_L1_CONFIG_CUSTOM_DEFAULT_USE_DEDICATED_SECTION ON)
set(CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SHA256_SIZE 32)
set(CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SIZE 4096)
# Linker calib flags
set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} \
-Wl,--defsym=CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SHA256_SIZE=${CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SHA256_SIZE}"
)

set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} \
-Wl,--defsym=CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SIZE=${CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SIZE}"
)

# Set qhal needs
set(CONFIG_QHAL_IMPL_NRFX ON)
set(CONFIG_QPLATFORM_IMPL_QM33_QHAL_NON_ZEPHYR ON)
set(CONFIG_QHAL_MAX_GPIO_CALLBACKS 6)

list(APPEND LINK_LIB_LIST "qhal")
add_subdirectory(${PROJECT_BASE}/${LIBS_PATH}/uwb-stack/libs/qhal qhal)

list(APPEND LINK_LIB_LIST "qplatform")
add_subdirectory(${PROJECT_BASE}/${LIBS_PATH}/uwb-stack/libs/qplatform qplatform)

target_compile_definitions(
  qhal PRIVATE CONFIG_QHAL_MAX_GPIO_CALLBACKS=${CONFIG_QHAL_MAX_GPIO_CALLBACKS}
)

target_compile_definitions(qplatform PRIVATE CONFIG_QPLATFORM_IMPL_NRF)

target_link_libraries(qhal PUBLIC HAL)

if(CONFIG_LOG)
  add_subdirectory(${PROJECT_BASE}/Src/Logger logger)
  list(APPEND LINK_LIB_LIST "logger")
endif()

list(APPEND LINK_LIB_LIST "qosal")

add_subdirectory(${PROJECT_BASE}/Src/AppConfig AppConfig)

add_subdirectory(${PROJECT_BASE}/Src/Apps/Src/common AppCommon)
list(APPEND LINK_LIB_LIST "app_common")

add_subdirectory(${PROJECT_BASE}/Src/UWB UWB)
list(APPEND LINK_LIB_LIST "UWB")

add_subdirectory(${PROJECT_BASE}/Src/Apps/Src/reporter reporter)
list(APPEND LINK_LIB_LIST "reporter")

add_subdirectory(${PROJECT_BASE}/Src/Helpers Helpers)
list(APPEND LINK_LIB_LIST "Helpers")

add_subdirectory(${PROJECT_BASE}/Src/Comm Comm)
list(APPEND LINK_LIB_LIST "Interface")

if(USE_LISTENER EQUAL 1)
  add_subdirectory(${PROJECT_BASE}/Src/Apps/Src/listener AppListener)
  list(APPEND LINK_LIB_LIST "listener")
endif()

if(USE_FIRA EQUAL 1)
  add_subdirectory(${PROJECT_BASE}/Src/Apps/Src/fira AppFira)
  list(APPEND LINK_LIB_LIST "fira")
endif()

# List all libraries
list(APPEND STATIC_LINK_LIB_LIST "uwbstack_core")

foreach(lib ${LINK_LIB_LIST})
  target_link_libraries(${EXECUTABLE} ${lib})
endforeach(lib in)

foreach(lib ${STATIC_LINK_LIB_LIST})
  target_link_libraries(${EXECUTABLE} ${lib})
endforeach(lib in)

target_link_libraries(${EXECUTABLE} -Wl,--no-whole-archive c m nosys)

add_custom_command(
  TARGET ${PROJECT_NAME}.elf
  POST_BUILD
  # copy target image to other formats
  COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
  COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
)
