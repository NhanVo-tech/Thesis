/**
 * @file      HAL_usb.c
 *
 * @brief     HAL functions for usb interface
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include "nrf_drv_usbd.h"
#include "app_error.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"
#include "boards.h"
#include "InterfUsb.h"
#include "HAL_usb.h"
#include "nrf_drv_clock.h"

#define LED_USB_RESUME   (BSP_BOARD_LED_0)
#define LED_CDC_ACM_OPEN (BSP_BOARD_LED_1)

/**
 * @brief Enable power USB detection
 *
 * Configure if example supports USB port connection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif

static volatile bool tx_pending = false;

static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const *p_inst, app_usbd_cdc_acm_user_event_t event);

#define CDC_ACM_COMM_INTERFACE 0
#define CDC_ACM_COMM_EPIN      NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE 1
#define CDC_ACM_DATA_EPIN      NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT     NRF_DRV_USBD_EPOUT1

/**
 * @brief CDC_ACM class instance
 */
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm, cdc_acm_user_ev_handler, CDC_ACM_COMM_INTERFACE, CDC_ACM_DATA_INTERFACE, CDC_ACM_COMM_EPIN, CDC_ACM_DATA_EPIN, CDC_ACM_DATA_EPOUT, APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

/* Size of single Rx of USB. */
#define USB_MAX_RX_SIZE 64
/* Size of USB Rx circular buffer. */
#define USB_RX_BUFF_SIZE (USB_MAX_RX_SIZE * 16)

static struct cc_buff usb_rx_buffer = {};
static uint8_t usb_buffer[USB_RX_BUFF_SIZE] = {};

static CommRxCallback usb_rx_callback = NULL;

/**
 * @brief User event handler @ref app_usbd_cdc_acm_user_ev_handler_t
 */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const *p_inst, app_usbd_cdc_acm_user_event_t event)
{
    app_usbd_cdc_acm_t const *p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);
    ret_code_t ret;

    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            tx_pending = false;
            bsp_board_led_on(LED_CDC_ACM_OPEN);

            /* Setup first transfer. */
            uint8_t *buff_ptr = cc_buff_get_write_ptr(&usb_rx_buffer, USB_MAX_RX_SIZE);
            assert(buff_ptr);
            app_usbd_cdc_acm_read_any(&m_app_cdc_acm, buff_ptr, USB_MAX_RX_SIZE);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            tx_pending = false;
            bsp_board_led_off(LED_CDC_ACM_OPEN);
            break;
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
            tx_pending = false;
            break;
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            do
            {
                /* Get the amount of data transferred. */
                size_t len = app_usbd_cdc_acm_rx_size(p_cdc_acm);
                cc_buff_commit_write(&usb_rx_buffer, len);

                /* Setup the next transfer. */
                uint8_t *buff_ptr = cc_buff_get_write_ptr(&usb_rx_buffer, USB_MAX_RX_SIZE);
                assert(buff_ptr);
                ret = app_usbd_cdc_acm_read_any(&m_app_cdc_acm, buff_ptr, USB_MAX_RX_SIZE);
            } while (ret == NRF_SUCCESS);

            usb_rx_callback(&usb_rx_buffer);
            break;
        }
        default:
            break;
    }
}

static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:
            bsp_board_led_off(LED_USB_RESUME);
            break;
        case APP_USBD_EVT_DRV_RESUME:
            bsp_board_led_on(LED_USB_RESUME);
            break;
        case APP_USBD_EVT_STARTED:
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            bsp_board_leds_off();
            break;
        case APP_USBD_EVT_POWER_DETECTED:

            if (!nrf_drv_usbd_is_enabled())
            {
#if defined(USE_USB_ENUM_WORKAROUND)
                /* Workaround for nRF SDK issue. When HFCLK is turned on, then USB hangs on
                 * initialization for ~14 seconds. To workaround this issue, HFCLK is disabled before
                 * enabling USB.

                 * Warning: Plugging USB cable while ranging is ongoing can affect measurement
                 * performance for a short moment of time.
                 * If HDK is battery supplied, the flag USB_ENUM_WORKAROUND should be turned off! */
                if (nrf_drv_clock_hfclk_is_running())
                {
                    nrf_drv_clock_hfclk_release();
                    while (nrf_drv_clock_hfclk_is_running())
                        ;
                }
#endif
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            app_usbd_stop();
#if defined(USE_USB_ENUM_WORKAROUND)
            /* Workaround for nRF SDK issue. HFCLK cannot be turned on all the time. It needs to be
             * explicitly turned on after USB has stopped (e.g. cable plugged off). */
            nrf_drv_clock_hfclk_request(NULL);
#endif
            break;
        case APP_USBD_EVT_POWER_READY:
            app_usbd_start();
            break;
        case APP_USBD_EVT_STATE_CHANGED:
            if (APP_USBD_STATE_Configured == app_usbd_core_state_get())
            {
                UsbSetState(USB_CONFIGURED);
            }
            break;
        default:
            break;
    }
}

static bool deca_usb_transmit(unsigned char *tx_buffer, int size)
{
    ret_code_t ret;

    for (int i = 0; i < 1; i++)
    {
        tx_pending = true;
        ret = app_usbd_cdc_acm_write(&m_app_cdc_acm, tx_buffer, size);
        if (ret != NRF_SUCCESS)
        {
            break;
        }
    }
    return ret == NRF_SUCCESS;
}

static void deca_usb_init(CommRxCallback callback)
{
    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler};

    assert(callback);
    usb_rx_callback = callback;
    cc_buff_init(&usb_rx_buffer, usb_buffer, USB_RX_BUFF_SIZE, USB_MAX_RX_SIZE);
    app_usbd_serial_num_generate();

    tx_pending = false;
    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);

    app_usbd_class_inst_t const *class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    if (USBD_POWER_DETECTION)
    {
#ifndef SOFTDEVICE_PRESENT
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
#endif
    }
    else
    {
        app_usbd_enable();
        app_usbd_start();
    }
}

static bool isTxBufferEmpty(void)
{
    return tx_pending == false;
}

static void InterfaceUsbUpdate(void)
{
}


/** @brief HAL USB API structure. */
const struct hal_usb_s Usb = {
    .init = &deca_usb_init,
    .deinit = NULL,
    .transmit = &deca_usb_transmit,
    .receive = NULL,
    .update = &InterfaceUsbUpdate,
    .isTxBufferEmpty = &isTxBufferEmpty};
