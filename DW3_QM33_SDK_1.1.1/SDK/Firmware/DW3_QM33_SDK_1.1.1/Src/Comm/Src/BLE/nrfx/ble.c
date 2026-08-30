/**
 * @file    ble.c
 *
 * @brief   Implementation of the Qorvo Apple Nearby Interaction example
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sdk_config.h"
#include "nordic_common.h"
#include "app_error.h"
#include "nrf.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_qnis.h"
#include "ble_anis.h"
#include "ble_conn_params.h"
#include "ble_conn_state.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_freertos.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_nvic.h"

#include "app_ble.h"
#include "boards.h"
#include "qlog.h"

#define APP_BLE_OBSERVER_PRIO          3 /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG           1 /**< A tag identifying the SoftDevice BLE configuration. */

#define APP_ADV_INTERVAL_FAST          320  /**< The advertising interval (in units of 0.625 ms. This value corresponds to 200 ms). */
#define APP_ADV_INTERVAL_SLOW          1600 /**< The advertising interval (in units of 0.625 ms. This value corresponds to 1s). */
#define APP_ADV_DURATION               0    /**< The advertising duration in units of 10 milliseconds. 0 is to never stop. */

#define MIN_CONN_INTERVAL              MSEC_TO_UNITS(25, UNIT_1_25_MS)  /**< Minimum acceptable connection interval (0.4 seconds). */
#define MAX_CONN_INTERVAL              MSEC_TO_UNITS(250, UNIT_1_25_MS) /**< Maximum acceptable connection interval (0.65 second). */
#define SLAVE_LATENCY                  6                                /**< Slave latency. */
#define CONN_SUP_TIMEOUT               MSEC_TO_UNITS(16000, UNIT_10_MS) /**< Connection supervisory time-out (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY 5000  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY  30000 /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT   2     /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                 1                    /**< Perform bonding. */
#define SEC_PARAM_MITM                 0                    /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                 0                    /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS             0                    /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES      BLE_GAP_IO_CAPS_NONE /**< No I/O capabilities. */
#define SEC_PARAM_OOB                  0                    /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE         7                    /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE         16                   /**< Maximum encryption key size. */

BLE_QNIS_DEF(m_qnis, NRF_SDH_BLE_TOTAL_LINK_COUNT);
BLE_ANIS_DEF(m_anis, NRF_SDH_BLE_TOTAL_LINK_COUNT);
NRF_BLE_GATT_DEF(m_gatt);                              /**< GATT module instance. */
NRF_BLE_QWRS_DEF(m_qwr, NRF_SDH_BLE_TOTAL_LINK_COUNT); /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                    /**< Advertising module instance. */

static ble_uuid_t m_adv_uuids[] = /**< Universally unique service identifiers. */
    {
        {BLE_UUID_QNIS_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN}};
static void advertising_start(void *parm);

/**
 * @brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const *p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            // advertising_start(false);
            break;

        default:
            break;
    }
}

/**
 * @brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
/**@snippet [Handling the data received over BLE] */
static void qnis_data_handler(ble_qnis_evt_t *p_evt)
{
    if (p_evt->type == BLE_QNIS_EVT_COMM_STARTED)
    {
        QLOGI("Notification is enabled");
    }

    if (p_evt->type == BLE_QNIS_EVT_COMM_STOPPED)
    {
        QLOGI("Notification is disabled");
    }

    if (p_evt->type == BLE_QNIS_EVT_RX_DATA)
    {
        const uint8_t *p_data;
        uint16_t data_len;
        p_data = p_evt->params.rx_data.p_data;
        data_len = p_evt->params.rx_data.length;

        handle_niq_data(p_evt->conn_handle, p_data, data_len);
    }
}

/**
 * @brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(char *gap_name)
{
    static ret_code_t err_code;
    ble_gap_conn_params_t gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)gap_name, strlen(gap_name));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_TAG);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**
 * @brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the QNIS and ANIS services.
 */
static void services_init(void)
{
    ret_code_t err_code;
    ble_qnis_init_t qnis_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    /* Initialize Queued Write Module. */
    qwr_init.error_handler = nrf_qwr_error_handler;

    for (uint32_t i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++)
    {
        err_code = nrf_ble_qwr_init(&m_qwr[i], &qwr_init);
        APP_ERROR_CHECK(err_code);
    }

    /* Initialize QNIS. */
    memset(&qnis_init, 0, sizeof(qnis_init));

    qnis_init.data_handler = qnis_data_handler;

    err_code = ble_qnis_init(&m_qnis, &qnis_init);
    APP_ERROR_CHECK(err_code);

    /* Initialize ANIS. */
    err_code = ble_anis_init(&m_anis);
    APP_ERROR_CHECK(err_code);
}

/**
 * @brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    /* Security parameters to be used for all security procedures. */
    sec_param.bond = SEC_PARAM_BOND;
    sec_param.mitm = SEC_PARAM_MITM;
    sec_param.lesc = SEC_PARAM_LESC;
    sec_param.keypress = SEC_PARAM_KEYPRESS;
    sec_param.io_caps = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob = SEC_PARAM_OOB;
    sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc = 1;
    sec_param.kdist_own.id = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}

/**
 * @brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in]   p_evt   Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t *p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(p_evt->conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**
 * @brief Function for handling a Connection Parameters error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**
 * @brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID; // m_hrs.hrm_handles.cccd_handle;
    cp_init.disconnect_on_fail = false;
    cp_init.evt_handler = on_conn_params_evt;
    cp_init.error_handler = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
}


/**
 * @brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            QLOGI("Fast advertising.");
            break;

        case BLE_ADV_EVT_SLOW:
            QLOGI("Slow advertising.");
            break;

        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            QLOGI("Idle advertising.");
            break;

        default:
            break;
    }
}


/**
 * @brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Uqnised.
 */
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{
    uint32_t err_code;

    /* For readability. */
    ble_gap_evt_t const *p_gap_evt = &p_ble_evt->evt.gap_evt;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            QLOGI("Connection 0x%x established.", p_gap_evt->conn_handle);

            ble_evt_connected_handler(p_gap_evt->conn_handle);

            /* Assign connection handle to available instance of QWR module. */
            for (uint32_t i = 0; i < NRF_SDH_BLE_PERIPHERAL_LINK_COUNT; i++)
            {
                if (m_qwr[i].conn_handle == BLE_CONN_HANDLE_INVALID)
                {
                    err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr[i], p_gap_evt->conn_handle);
                    APP_ERROR_CHECK(err_code);
                    break;
                }
            }

            if (ble_conn_state_peripheral_conn_count() < NRF_SDH_BLE_PERIPHERAL_LINK_COUNT)
            {
                advertising_start(NULL);
            }
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            QLOGI("Disconnected");
#if LEDS_NUMBER > 1
            /* If inside the secure bubble, keep red led, but if not, turn green off. */
            bsp_board_led_off(BSP_BOARD_LED_0);
#endif
            ble_evt_disconnected_handler(p_gap_evt->conn_handle);
            if (ble_conn_state_peripheral_conn_count() == (NRF_SDH_BLE_PERIPHERAL_LINK_COUNT - 1))
            {
                /* Advertising is not running when all connections are taken, and must therefore be started. */
                advertising_start(NULL);
            }
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            QLOGD("PHY update request.");
            ble_gap_phys_t const phys = {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_gap_evt->conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        }
        break;

        case BLE_GATTC_EVT_TIMEOUT:
            /* Disconnect on GATT Client timeout event. */
            QLOGD("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_gap_evt->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            /* Disconnect on GATT Server timeout event. */
            QLOGD("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_gap_evt->conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_ADV_SET_TERMINATED:
            /* Resetting the application when advertisement terminated */
            QLOGD("GATT Server Terminated.");
            sd_nvic_SystemReset();
            break;

        default:
            /* No implementation needed. */
            break;
    }
}

/**
 * @brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    nrf_sdh_disable_request();

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    /* Configure the BLE stack using the default settings. */
    /* Fetch the start address of the application RAM. */
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    /* Enable BLE stack. */
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    /* Register a handler for BLE events. */
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**
 * @brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type = BLE_ADVDATA_NO_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids = m_adv_uuids;

    init.srdata.name_type = BLE_ADVDATA_FULL_NAME;

    init.config.ble_adv_on_disconnect_disabled = true;
    init.config.ble_adv_fast_enabled = false;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL_FAST;
    init.config.ble_adv_fast_timeout = APP_ADV_DURATION;
    init.config.ble_adv_slow_enabled = true;
    init.config.ble_adv_slow_interval = APP_ADV_INTERVAL_SLOW;
    init.config.ble_adv_slow_timeout = APP_ADV_DURATION;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**
 * @brief Function for starting advertising.
 */
static void advertising_start(void *parm)
{
    ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_SLOW);
    APP_ERROR_CHECK(err_code);
}


void ble_init(char *gap_name)
{
    /* Configure and initialize the BLE stack. */
    ble_stack_init();

    /* Initialize modules. */
    gap_params_init(gap_name);
    gatt_init();
    services_init();
    peer_manager_init();
    advertising_init();
    conn_params_init();

    /* Create a FreeRTOS task for the BLE stack. */
    nrf_sdh_freertos_init(advertising_start, NULL);
}

void send_qnis_data(uint16_t conn_handle, uint8_t *buffer, uint16_t data_len)
{
    uint16_t length = data_len;
    uint32_t err_code;

    do
    {
        err_code = ble_qnis_data_send(&m_qnis, buffer, &length, conn_handle);
        if ((err_code != NRF_ERROR_INVALID_STATE) && (err_code != NRF_ERROR_RESOURCES) && (err_code != NRF_ERROR_NOT_FOUND))
        {
            APP_ERROR_CHECK(err_code);
        }

    } while (err_code == NRF_ERROR_RESOURCES);
}
