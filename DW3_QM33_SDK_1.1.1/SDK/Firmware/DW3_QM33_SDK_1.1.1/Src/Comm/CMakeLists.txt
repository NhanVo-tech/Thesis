# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

add_library(Interface Src/InterfUsb.c)

target_include_directories(Interface PUBLIC Inc)

install(TARGETS Interface EXPORT Interface-config)
install(EXPORT Interface-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/Interface/cmake)
