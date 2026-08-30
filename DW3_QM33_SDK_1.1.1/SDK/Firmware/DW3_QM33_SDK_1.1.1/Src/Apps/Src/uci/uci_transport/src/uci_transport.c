/**
 * @file      uci_transport.c
 *
 * @brief     uci transport functions
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include <stdlib.h>

#include "reporter.h"
#include "uci_transport.h"
#include "qlog.h"
#include "qrtc.h"
#include "qirq.h"
#include "nrf_delay.h"

/*
 * RS485 auto-direction switching delay.
 * After the nRF receives a UCI command, the ESP32's RS485 transceiver needs
 * time to switch from TX to RX mode. This delay ensures the response is not
 * sent before the ESP32 is ready to receive it.
 * Set to 0 to disable, or override via compiler flag -DUCI_TX_DELAY_MS=<N>.
 */
#ifndef UCI_TX_DELAY_MS
#define UCI_TX_DELAY_MS 20
#endif

#ifdef NRF_LOG_MODULE_NAME
#undef NRF_LOG_MODULE_NAME
#endif

#define NRF_LOG_MODULE_NAME uci_transport

#include "nrf_log.h"


#define UCI_GARBAGE_TIMEOUT_MS 100

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - __builtin_offsetof(type, member)))

NRF_LOG_MODULE_REGISTER();

static struct uci_blk *rx = NULL;
static uint16_t rx_offset = 0;

void uci_tp_attach(struct uci_transport *tr, struct uci *uci)
{
    struct uci_tp *s = container_of(tr, struct uci_tp, tr);

    s->uci = uci;
}

void uci_tp_detach(struct uci_transport *tr)
{
    struct uci_tp *s = container_of(tr, struct uci_tp, tr);

    if (rx)
    {
        bool found = false;
        if (s->uci)
        {
            struct uci_blk *block = s->uci->rx;
            struct uci_blk *next;
            while (block)
            {
                next = block->next;
                if (rx == block)
                {
                    found = true;
                    break;
                }
                block = next;
            }
        }

        if (!found)
        {
            uci_blk_free_all(s->uci, rx);
        }
        rx = NULL;
    }

    rx_offset = 0;

    s->uci = NULL;
}

void uci_tp_usb_packet_send_ready(struct uci_transport *tr)
{
    struct uci_tp *s = container_of(tr, struct uci_tp, tr);
    struct uci_blk *p;

    /* Delay to let the ESP32 RS485 transceiver switch from TX to RX mode */
    if (UCI_TX_DELAY_MS > 0 && (s->uci_if == UCI_UART0 || s->uci_if == UCI_UART1))
    {
        nrf_delay_ms(UCI_TX_DELAY_MS);
    }

    while ((p = uci_packet_send_get_ready(s->uci)))
    {
        struct uci_blk *current = p;

        while (current)
        {
            if (current->len >= UCI_PACKET_HEADER_SIZE)
            {
                const uint8_t mt = (current->data[0] >> 5) & 0x07U;
                const uint8_t gid = current->data[0] & 0x0FU;
                const uint8_t oid = current->data[1] & 0x3FU;
                NRF_LOG_INFO("UCI TX mt=%d gid=0x%02x oid=0x%02x len=%d", mt, gid, oid, current->data[3]);
            }
            reporter_instance.print((char *)current->data, current->len);
            current = current->next;
        }

        uci_packet_send_done(s->uci, p, 0);
    }
}

/**
 * @brief Platform agnostic read entry point for the UCI transport read.
 * This function process the buffer provided in entry and
 * check if it contains a valid UCI packet.
 * If it does -> call UCI-CORE
 * If it does not -> return NO_PACKET
 * @param tr Transport structure containing buffer to read from, buffer len and uci structure.
 * @return Number of byte read from the input buffer.
 */

int uci_tp_read(struct uci_tp *tr, struct cc_buff *buf)
{
    uint16_t n = 0;
    uint16_t data_size = 0;
    uint16_t available = buf->cnt;
    static bool msg_pending = false;
    static int64_t start_timestamp_us = 0;

    while (available > 0)
    {
        if (!rx)
        {
            rx = uci_blk_alloc(tr->uci, UCI_MAX_PACKET_SIZE, 0);
            if (!rx)
            {
                QLOGE("UCI: Packet allocation failed");
                return QERR_ENOMEM;
            }
        }

        if (rx_offset < UCI_PACKET_HEADER_SIZE)
            n = MIN(UCI_PACKET_HEADER_SIZE - rx_offset, available);
        else
            n = MIN(UCI_PACKET_HEADER_SIZE + rx->data[3] - rx_offset, available);


        unsigned lock = qirq_lock();
        available -= cc_buff_copy_n(buf, rx->data + rx_offset, n);
        qirq_unlock(lock);

        rx_offset += n;
        /* If received data has same len as declared in header then assume
         * it's a complete UCI message and pass it to the stack. */
        if (rx_offset == (UCI_PACKET_HEADER_SIZE + rx->data[3]))
        {
            const uint8_t mt = (rx->data[0] >> 5) & 0x07U;
            const uint8_t gid = rx->data[0] & 0x0FU;
            const uint8_t oid = rx->data[1] & 0x3FU;
            NRF_LOG_INFO("UCI RX mt=%d gid=0x%02x oid=0x%02x len=%d", mt, gid, oid, rx->data[3]);
            rx->len = rx_offset;
            uci_packet_recv(tr->uci, rx);
            rx = NULL;
            rx_offset = 0;
            msg_pending = false;
        }
        /* Not a complete message. */
        else if (rx_offset > 0)
        {
            if (msg_pending)
            {
                int64_t current_timestamp_us = qrtc_get_us();
                /* If message is pending completion for too long, flush the buffer. */
                if (current_timestamp_us - start_timestamp_us > (UCI_GARBAGE_TIMEOUT_MS * 1000))
                {
                    QLOGW("UCI: Timeout, flushing UCI buffer");
                    uci_tp_flush(tr);
                    msg_pending = false;
                    return QERR_EINVAL;
                }
            }
            else
            {
                start_timestamp_us = qrtc_get_us();
            }
            msg_pending = true;
        }
    }

    return data_size;
}

void uci_tp_flush(struct uci_tp *tr)
{
    uci_blk_free_all(tr->uci, rx);
    rx = NULL;
    rx_offset = 0;
}
