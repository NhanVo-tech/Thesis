/**
 * @file      InterfUsb.c
 *
 * @brief     Usb interface to set and get the state
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "InterfUsb.h"

static enum usbState state = USB_DISCONNECTED;

void UsbSetState(enum usbState _state)
{
    state = _state;
}

enum usbState UsbGetState(void)
{
    return state;
}
