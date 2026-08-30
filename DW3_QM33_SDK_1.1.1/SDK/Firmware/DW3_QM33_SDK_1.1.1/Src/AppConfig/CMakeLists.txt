# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

add_library(Config STATIC Src/config.c)

target_include_directories(Config PUBLIC Inc)

target_link_libraries(Config PUBLIC UWB)

install(TARGETS Config EXPORT Config-config)
install(EXPORT Config-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/Config/cmake)
