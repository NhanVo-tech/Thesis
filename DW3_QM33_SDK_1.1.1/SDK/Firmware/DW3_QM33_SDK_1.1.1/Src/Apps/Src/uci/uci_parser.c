/**
 * @file      uci_parser.c
 *
 * @brief     UCI parser implementation
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include <ctype.h>
#include <string.h>

#include "uci_parser.h"
#include "controlTask.h"

/**
 * @brief Nothing to be done, UCI task works directly on RxCircBuffer
 */
usb_data_e uci_on_rx(struct cc_buff *buf)
{
    return buf->cnt ? DATA_READY : NO_DATA;
}
