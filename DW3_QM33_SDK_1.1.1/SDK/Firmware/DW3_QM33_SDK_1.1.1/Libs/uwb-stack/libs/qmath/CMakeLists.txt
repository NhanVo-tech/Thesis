# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-Qorvo-2
# cmake-lint: disable=E1122

cmake_minimum_required(VERSION 3.13)

add_library(qmath STATIC src/qmath.c)
target_include_directories(qmath PUBLIC include)

install(
  TARGETS qmath
  OPTIONAL
  EXPORT qmath-config
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qmath
          FILE_SET HEADERS
          DESTINATION include/qmath
)

install(EXPORT qmath-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/qmath/cmake)
