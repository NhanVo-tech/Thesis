# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

add_library(
  uci task_uci/task_uci.c uci_transport/src/uci_transport.c uci_parser.c
      uwbmac_helper/src/uwbmac_helper_dw3000.c
)

target_include_directories(
  uci
  PRIVATE ./
          ../../Inc
          ../common
          ./task_uci
          ./uci_transport/include
          ./uwbmac_helper/include
          ./embedded/include/
)

target_link_libraries(uci PUBLIC Config HAL UWB uwbstack_core uwbstack_uci)
