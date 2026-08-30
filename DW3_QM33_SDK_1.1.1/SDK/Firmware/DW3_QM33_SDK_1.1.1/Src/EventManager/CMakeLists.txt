# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

add_library(EventManager STATIC EventManager.c)

target_include_directories(EventManager PUBLIC ./)

target_link_libraries(EventManager PUBLIC app_common Board)

install(TARGETS EventManager EXPORT EventManager-config)
install(EXPORT EventManager-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/EventManager/cmake)
