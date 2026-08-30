# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

add_library(fira_qani STATIC fira_niq.c)

target_include_directories(fira_qani PUBLIC .)

target_link_libraries(fira_qani PUBLIC fira niq Helpers)
