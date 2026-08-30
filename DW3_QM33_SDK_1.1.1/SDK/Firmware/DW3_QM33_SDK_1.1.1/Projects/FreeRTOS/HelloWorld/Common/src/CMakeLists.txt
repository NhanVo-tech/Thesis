# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

add_library(helloWorld STATIC hello_world.c)

target_include_directories(helloWorld PUBLIC ./ ../include)

target_link_libraries(helloWorld PUBLIC UWB qplatform)
