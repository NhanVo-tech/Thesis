# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

set(BLE_SRC_PATH ../../../../SDK_BSP/Nordic/SDK_17_1_0/components/ble)

set(BLE_SRC niq/ble_niq.c nrfx/qnis/ble_qnis.c nrfx/anis/ble_anis.c)
set(BLE_INC_PATH niq/ nrfx/qnis/ nrfx/anis/)

add_library(
  BLE
  nrfx/ble.c
  ${BLE_SRC}
  ${BLE_SRC_PATH}/peer_manager/auth_status_tracker.c
  ${BLE_SRC_PATH}/peer_manager/gatt_cache_manager.c
  ${BLE_SRC_PATH}/peer_manager/gatts_cache_manager.c
  ${BLE_SRC_PATH}/peer_manager/id_manager.c
  ${BLE_SRC_PATH}/peer_manager/peer_data_storage.c
  ${BLE_SRC_PATH}/peer_manager/peer_database.c
  ${BLE_SRC_PATH}/peer_manager/peer_id.c
  ${BLE_SRC_PATH}/peer_manager/peer_manager.c
  ${BLE_SRC_PATH}/peer_manager/peer_manager_handler.c
  ${BLE_SRC_PATH}/peer_manager/pm_buffer.c
  ${BLE_SRC_PATH}/peer_manager/security_dispatcher.c
  ${BLE_SRC_PATH}/peer_manager/security_manager.c
  ${BLE_SRC_PATH}/common/ble_advdata.c
  ${BLE_SRC_PATH}/common/ble_conn_params.c
  ${BLE_SRC_PATH}/common/ble_conn_state.c
  ${BLE_SRC_PATH}/common/ble_srv_common.c
  ${BLE_SRC_PATH}/ble_advertising/ble_advertising.c
  ${BLE_SRC_PATH}/nrf_ble_gatt/nrf_ble_gatt.c
  ${BLE_SRC_PATH}/nrf_ble_qwr/nrf_ble_qwr.c
  ${BLE_SRC_PATH}/ble_link_ctx_manager/ble_link_ctx_manager.c
  ${BLE_SRC_PATH}/ble_services/ble_bas/ble_bas.c
  ${BLE_SRC_PATH}/ble_services/ble_dis/ble_dis.c
  ${BLE_SRC_PATH}/ble_services/ble_hrs/ble_hrs.c
)

target_include_directories(
  BLE
  PUBLIC Inc ./ ${BLE_INC_PATH} ${BLE_SRC_PATH}/common ${BLE_SRC_PATH}/ble_services/ble_nus
         ${BLE_SRC_PATH}/ble_link_ctx_manager
  PRIVATE ${BLE_SRC_PATH}/ble_advertising
          ${BLE_SRC_PATH}/ble_dtm
          ${BLE_SRC_PATH}/ble_racp
          ${BLE_SRC_PATH}/ble_services/ble_ancs_c
          ${BLE_SRC_PATH}/ble_services/ble_ans_c
          ${BLE_SRC_PATH}/ble_services/ble_bas
          ${BLE_SRC_PATH}/ble_services/ble_bas_c
          ${BLE_SRC_PATH}/ble_services/ble_cscs
          ${BLE_SRC_PATH}/ble_services/ble_cts_c
          ${BLE_SRC_PATH}/ble_services/ble_dfu
          ${BLE_SRC_PATH}/ble_services/ble_dis
          ${BLE_SRC_PATH}/ble_services/ble_gls
          ${BLE_SRC_PATH}/ble_services/ble_hids
          ${BLE_SRC_PATH}/ble_services/ble_hrs
          ${BLE_SRC_PATH}/ble_services/ble_hrs_c
          ${BLE_SRC_PATH}/ble_services/ble_hts
          ${BLE_SRC_PATH}/ble_services/ble_ias
          ${BLE_SRC_PATH}/ble_services/ble_ias_c
          ${BLE_SRC_PATH}/ble_services/ble_lbs
          ${BLE_SRC_PATH}/ble_services/ble_lbs_c
          ${BLE_SRC_PATH}/ble_services/ble_lls
          ${BLE_SRC_PATH}/ble_services/ble_rscs
          ${BLE_SRC_PATH}/ble_services/ble_rscs_c
          ${BLE_SRC_PATH}/ble_services/ble_tps
          ${BLE_SRC_PATH}/common
          ${BLE_SRC_PATH}/nrf_ble_gatt
          ${BLE_SRC_PATH}/nrf_ble_qwr
          ${BLE_SRC_PATH}/peer_manager
)

target_link_libraries(BLE SDK Config uwbstack_core fira niq)
