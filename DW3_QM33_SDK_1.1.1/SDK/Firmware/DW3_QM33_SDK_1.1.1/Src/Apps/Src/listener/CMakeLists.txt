# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

add_library(listener STATIC listener.c task_listener.c listener_fn.c)

target_include_directories(
  listener PRIVATE ${PROJECT_BASE}/Src/UWB ./ ../../Inc ../common ${MY_OS}/
)

target_link_libraries(listener PUBLIC Config EventManager HAL app_common)
