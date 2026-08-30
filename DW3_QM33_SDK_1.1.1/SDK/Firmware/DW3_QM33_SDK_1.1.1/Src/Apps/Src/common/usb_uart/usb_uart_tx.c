/**
 * @file      usb_uart_tx.c
 *
 * @brief     Puts message to circular buffer which will be transmitted by flushing thread
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include <stdio.h>
#include <string.h>
#include "usb_uart_tx.h"
#include "circular_buffer.h"
#include "HAL_lock.h"
#include "HAL_uart.h"
#ifdef USB_ENABLE
#include "HAL_usb.h"
#include "InterfUsb.h"
#endif
#include "HAL_error.h"
#include "app.h"
#include "flushTask.h"

#include "qmutex.h"

#include "minmax.h"
#include "driver_app_config.h"

/* The size is such long because of possible ACCUMULATORS sending. */
#ifdef TINY_BUILD
/* The size of report buffer, must be 1<<N, i.e. 0x800,0x1000 etc. */
#define USB_REPORT_BUFSIZE 0x800
#else
/* The size of report buffer, must be 1<<N, i.e. 0x800,0x1000 etc. */
#define USB_REPORT_BUFSIZE 0x8000
#endif
/* The max timeout to output of the whole report buffer on UART speed 115200, ms. */
#define USB_UART_TX_TIMEOUT_MS      ((USB_REPORT_BUFSIZE * 10) / 115)
#define CDC_DATA_FS_MAX_PACKET_SIZE 64

/* Mutex timeout in ms. */
#define USB_UART_MUTEX_TIMEMOUT_MS USB_UART_TX_TIMEOUT_MS

/* Linear buffer, to transmit next chunk of data. */
static uint8_t ubuf[CDC_DATA_FS_MAX_PACKET_SIZE];

static struct _txHandle
{
    dw_hal_lockTypeDef lock;
    /* Circular USB/UART Tx report buffer. */
    struct
    {
        uint16_t head;
        uint16_t tail;
        uint8_t buf[USB_REPORT_BUFSIZE];
    } Report;
} txHandle = {
    .lock = DW_HAL_NODE_UNLOCKED,
    .Report.head = 0,
    .Report.tail = 0};

/* Mutex for tx buffer access. */
static struct qmutex *tx_buffer_mutex = NULL;

error_e init_tx_msg(void)
{
    tx_buffer_mutex = qmutex_init();
    if (!tx_buffer_mutex)
    {
        return _ERR;
    }

    return _NO_ERR;
}

int reset_report_buf(void)
{
    if (qmutex_lock(tx_buffer_mutex, QOSAL_WAIT_FOREVER) != QERR_SUCCESS)
    {
        return _ERR;
    }

    txHandle.Report.head = txHandle.Report.tail = 0;

    qmutex_unlock(tx_buffer_mutex);

    return _NO_ERR;
}

/**
 * @brief Put message to circular report buffer it will be transmitted in background ASAP from flushing thread.
 *
 * @return HAL_BUSY - can not do it now, wait for release
 * HAL_ERROR - buffer overflow
 * HAL_OK - scheduled for transmission
 */
error_e copy_tx_msg(uint8_t *str, int len)
{
    error_e ret = _NO_ERR;
    uint16_t head, tail;
    const uint16_t size = sizeof(txHandle.Report.buf) / sizeof(txHandle.Report.buf[0]);

    /* Check if buffer available, wait if not .*/
    if (qmutex_lock(tx_buffer_mutex, USB_UART_MUTEX_TIMEMOUT_MS) != QERR_SUCCESS)
    {
        return _ERR;
    }

    /* Add TX msg to circular buffer and increase circular buffer length. */
    head = txHandle.Report.head;
    tail = txHandle.Report.tail;

    if (CIRC_SPACE(head, tail, size) > len)
    {
        while (len > 0)
        {
            txHandle.Report.buf[head] = *(str++);
            head = (head + 1) & (size - 1);
            len--;
        }

        txHandle.Report.head = head;
    }
    else
    {
        /* If packet can not fit, setup TX Buffer overflow ERROR and exit. */
        error_handler(0, _ERR_TxBuf_Overflow);
        ret = _ERR_TxBuf_Overflow;
    }

    qmutex_unlock(tx_buffer_mutex);
    return ret;
}

/**
 * @brief Wrap for copy_tx_msg. Puts message to circular report buffer.
 *
 * @return see copy_tx_msg()
 */
error_e port_tx_msg(uint8_t *str, int len)
{
    error_e ret = copy_tx_msg(str, len);
    NotifyFlushTask();
    return (ret);
}

/**
 * @brief FLUSH should have higher priority than reporter_instance.print().
 * This shall be called periodically from process, which can not be locked,
 * i.e. from independent high priority thread / timer etc.
 */
error_e flush_report_buf(void)
{
    int size = sizeof(txHandle.Report.buf) / sizeof(txHandle.Report.buf[0]);
    int chunk;
    error_e ret = _NO_ERR;

    /* Check if buffer available. */
    if (qmutex_lock(tx_buffer_mutex, QOSAL_WAIT_FOREVER) != QERR_SUCCESS)
    {
        return _ERR;
    }

    int head = txHandle.Report.head;
    int tail = txHandle.Report.tail;

    int len = CIRC_CNT(head, tail, size);

#ifdef USB_ENABLE
    int old_tail = txHandle.Report.tail;
#endif

    if (len > 0)
    {
#ifdef USB_ENABLE
        /* Check USB status - ready to TX. */
        if ((UsbGetState() == USB_CONFIGURED) && !Usb.isTxBufferEmpty())
        {
            /* USB did not send the buffer: no connection to the terminal. */
            ret = _ERR;
            goto exit;
        }
#endif

        /* Copy MAX allowed length from circular buffer to linear buffer. */
        chunk = MIN(sizeof(ubuf), len);

        for (int i = 0; i < chunk; i++)
        {
            ubuf[i] = txHandle.Report.buf[tail];
            tail = (tail + 1) & (size - 1);
        }

        len -= chunk;

        txHandle.Report.tail = tail;

        if (is_uart_allowed())
        {
            /* Setup UART DMA transfer. */
            if (deca_uart_transmit(ubuf, chunk))
            {
                /* Indicate UART transmit error. */
                error_handler(0, _ERR_UART_TX);
                ret = _ERR_UART_TX;
                goto exit;
            }
        }
        else
        {
#ifdef USB_ENABLE
            /* Setup USB IT transfer. */
            if (!Usb.transmit(ubuf, chunk))
            {
                /* Indicate USB transmit error. */
                error_handler(0, _ERR_Usb_Tx);
                txHandle.Report.tail = old_tail;
                ret = _ERR_Usb_Tx;
                goto exit;
            }
            else
            {
                old_tail = tail;
            }
#endif
        }
    }

exit:
    qmutex_unlock(tx_buffer_mutex);
    return ret;
}
