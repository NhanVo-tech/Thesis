/**
 * @file      ble_anis.c
 *
 * @brief     Interface for ANIS (Apple NI Service).
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "sdk_config.h"
#include "ble.h"
#include "niq.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "ble_link_ctx_manager.h"
#include "app_ble.h"

/**@brief   Macro for defining a ble_anis instance.
 *
 * @param     _name            Name of the instance.
 * @param[in] _anis_max_clients Maximum number of ANIS clients connected at a time.
 * @hideinitializer
 */
#define BLE_ANIS_DEF(_name, _anis_max_clients)                                                                            \
    BLE_LINK_CTX_MANAGER_DEF(CONCAT_2(_name, _link_ctx_storage), (_anis_max_clients), sizeof(ble_anis_client_context_t)); \
    static ble_anis_t _name = {                                                                                           \
        .p_link_ctx_storage = &CONCAT_2(_name, _link_ctx_storage)};                                                       \
    NRF_SDH_BLE_OBSERVER(_name##_obs, BLE_ANIS_BLE_OBSERVER_PRIO, ble_anis_on_ble_evt, &_name)

/**@brief   Apple Nearby Interaction Service event types. */
typedef enum
{
    BLE_ANIS_EVT_RX_DATA,      /**< Data received. */
    BLE_ANIS_EVT_TX_RDY,       /**< Service is ready to accept new data to be transmitted. */
    BLE_ANIS_EVT_COMM_STARTED, /**< Notification has been enabled. */
    BLE_ANIS_EVT_COMM_STOPPED, /**< Notification has been disabled. */
} ble_anis_evt_type_t;


/* Forward declaration of the ble_anis_t type. */
typedef struct ble_anis_s ble_anis_t;


/**@brief   Apple Nearby Interaction Service @ref BLE_ANIS_EVT_RX_DATA event data.
 *
 * @details This structure is passed to an event when @ref BLE_ANIS_EVT_RX_DATA occurs.
 */
typedef struct
{
    uint8_t const *p_data; /**< A pointer to the buffer with received data. */
    uint16_t length;       /**< Length of received data. */
} ble_anis_evt_rx_data_t;


/**@brief Apple Nearby Interaction Service client context structure.
 *
 * @details This structure contains state context related to hosts.
 */
typedef struct
{
    bool is_notification_enabled; /**< Variable to indicate if the peer has enabled notification of the RX characteristic.*/
} ble_anis_client_context_t;


/**@brief   Apple Nearby Interaction Service event structure.
 *
 * @details This structure is passed to an event coming from service.
 */
typedef struct
{
    ble_anis_evt_type_t type;              /**< Event type. */
    ble_anis_t *p_anis;                    /**< A pointer to the instance. */
    uint16_t conn_handle;                  /**< Connection handle. */
    ble_anis_client_context_t *p_link_ctx; /**< A pointer to the link context. */
    union
    {
        ble_anis_evt_rx_data_t rx_data; /**< @ref BLE_ANIS_EVT_RX_DATA event data. */
    } params;
} ble_anis_evt_t;


/**@brief Apple Nearby Interaction Service event handler type. */
typedef void (*ble_anis_data_handler_t)(ble_anis_evt_t *p_evt);


/**@brief   Apple Nearby Interaction Service structure.
 *
 * @details This structure contains status information related to the service.
 */
struct ble_anis_s
{
    uint8_t uuid_type;                                 /**< UUID type for Apple Nearby Interaction Service Base UUID. */
    uint16_t service_handle;                           /**< Handle of Apple Nearby Interaction Service (as provided by the SoftDevice). */
    ble_gatts_char_handles_t tx_handles;               /**< Handles related to the TX characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t rx_handles;               /**< Handles related to the RX characteristic (as provided by the SoftDevice). */
    blcm_link_ctx_storage_t *const p_link_ctx_storage; /**< Pointer to link context storage with handles of all current connections and its context. */
    ble_anis_data_handler_t data_handler;              /**< Event handler to be called for handling received data. */
};


/**@brief   Function for initializing the Apple Nearby Interaction Service.
 *
 * @param[out] p_anis     Apple Nearby Interaction Service structure. This structure must be supplied
 *                        by the application. It is initialized by this function and will
 *                        later be used to identify this particular service instance.
 * @param[in] p_anis_init Information needed to initialize the service.
 *
 * @retval NRF_SUCCESS If the service was successfully initialized. Otherwise, an error code is returned.
 * @retval NRF_ERROR_NULL If either of the pointers p_anis or p_anis_init is NULL.
 */
uint32_t ble_anis_init(ble_anis_t *p_anis);


/**@brief   Function for handling the Apple Nearby Interaction Service's BLE events.
 *
 * @details The Apple Nearby Interaction Service expects the application to call this function each time an
 * event is received from the SoftDevice. This function processes the event if it
 * is relevant and calls the Apple Nearby Interaction Service event handler of the
 * application if necessary.
 *
 * @param[in] p_ble_evt     Event received from the SoftDevice.
 * @param[in] p_context     Apple Nearby Interaction Service structure.
 */
void ble_anis_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);


/**@brief   Function to set the Accessory Configuration Buffer with Accessory Configuration Data.
 *
 * @details The Apple Nearby Interaction Service will periodically check the ACD characteristic value, which
 * should match the Accessory Configuration Data value. This function should be called when the Accessory
 * Configuration Data is defined, using the niq library, right before it was sent to the iOS device.
 *
 * @param[in] curr_config   Pointer to the Accessory Configuration Data packet payload.
 */
void set_accessory_uwb_config_data(uint8_t *curr_config);
