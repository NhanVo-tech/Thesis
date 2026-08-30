/**
 * @file      InterfUsb.h
 *
 * @brief     USB interface definition
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

enum usbState
{
    USB_DISCONNECTED,
    USB_PLUGGED,
    USB_CONNECTED,
    USB_CONFIGURED,
    USB_UNPLUGGED
};

enum usbCtrl
{
    USB_OFF,
    USB_LINE_CODING_RECEIVED,
    USB_LINE_CODING_REQUESTED,
    USB_LINE_SET_CTRL_LINE_STATE
};

void UsbSetState(enum usbState _state);
enum usbState UsbGetState(void);
