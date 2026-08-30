/**
 * @file      usb_reporter.c
 *
 * @brief     USB reported module
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include <string.h>

#include "reporter.h"
#include "usb_uart_tx.h"


static error_e usb_print(char *buff, int len);
static void usb_init(void);

reporter_t reporter_instance = {
    .init = usb_init,
    .print = usb_print};

static error_e usb_print(char *buff, int len)
{
    return port_tx_msg((uint8_t *)buff, len);
}

static void usb_init(void)
{
    return;
}
