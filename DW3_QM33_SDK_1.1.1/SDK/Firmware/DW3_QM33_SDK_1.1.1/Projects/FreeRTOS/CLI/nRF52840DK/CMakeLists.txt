# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

cmake_minimum_required(VERSION 3.19)

include(${MY_TARGET_SCRIPT})

project(${MY_TARGET})

message(
  (
  "[Projects/FreeRTOS/CLI/nRF52840DK/CMakeLists.txt] include ${PROJECT_COMMON}/cmakefiles/${MY_SAMPLE}-${MY_OS}.cmake"
  )
)

include(${PROJECT_COMMON}/cmakefiles/${MY_SAMPLE}-${MY_OS}.cmake)
