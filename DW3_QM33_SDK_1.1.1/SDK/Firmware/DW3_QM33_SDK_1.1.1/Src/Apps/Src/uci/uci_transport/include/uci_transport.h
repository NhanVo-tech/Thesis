/**
 * @file      uci_transport.h
 *
 * @brief     Interface for UCI transport
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "uci/uci.h"
#include "circular_buffer.h"

enum uci_if_e
{
    UCI_NONE,
    UCI_UART0,
    UCI_UART1,
};

struct uci_tp
{
    struct uci_transport tr;
    struct uci *uci;
    uint8_t uci_if;
};

void uci_tp_attach(struct uci_transport *tr, struct uci *uci);
void uci_tp_detach(struct uci_transport *tr);
void uci_tp_usb_packet_send_ready(struct uci_transport *tr);
int uci_tp_read(struct uci_tp *tr, struct cc_buff *buf);
void uci_tp_flush(struct uci_tp *tr);

#ifdef __cplusplus
}
#endif
