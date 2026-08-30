/**
 * @file      usb_uart_rx.c
 *
 * @brief     Module receiving data from USB/UART
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "usb_uart_rx.h"

#include <string.h>
#include <stdint.h>

#include "app.h"
#include "driver_app_config.h"
#include "controlTask.h"
#include "cmd.h"
#include "usb_uart_tx.h"
#if defined(USB_ENABLE)
#include "HAL_usb.h"
#include "InterfUsb.h"
#endif
#include "qlog.h"
#include "qirq.h"

extern char cmdBuf[];

/* Receiving command type status. */
typedef enum
{
    cmdREGULAR = 0,
    cmdJSON,
    cmdUNKNOWN_TYPE
} command_type_e;

/**
 * @brief Waits only commands from incoming stream.
 *
 * @return COMMAND_READY : the data for future processing can be found in app.local_buff : app.local_buff_len
 * NO_DATA : no command yet
 */
usb_data_e waitForCommand(struct cc_buff *buf)
{
    usb_data_e ret = NO_DATA;
    static uint16_t cmdLen = 0;
    static command_type_e command_type = cmdUNKNOWN_TYPE;
    static uint8_t brackets_cnt;
    uint8_t data;
    unsigned lock;

    lock = qirq_lock();
    while (buf->cnt)
    {
        if (!cc_buff_get_byte(buf, &data))
        {
            qirq_unlock(lock);
            return NO_DATA;
        }

        /* Erase of a char in the terminal. */
        if (data == '\b' || data == 0x7F)
        {
            port_tx_msg((uint8_t *)"\b\x20\b", 3);
            if (cmdLen)
                cmdLen--;
        }
        else
        {
            port_tx_msg(&data, 1);
            if (data == '\n' || data == '\r')
            {
                /* Checks if need to handle regular command. */
                if ((cmdLen != 0) && (command_type == cmdREGULAR))
                {
                    cmdBuf[cmdLen] = '\n';
                    cmdLen = 0;
                    command_type = cmdUNKNOWN_TYPE;
                    ret = COMMAND_READY;
                }
            }
            else if (command_type == cmdUNKNOWN_TYPE)
            {
                /* Find out if getting regular command or JSON. */
                cmdBuf[cmdLen] = data;
                if (data == '{')
                {
                    /* Start Json command. */
                    command_type = cmdJSON;
                    brackets_cnt = 1;
                }
                else
                {
                    /* Start regular command. */
                    command_type = cmdREGULAR;
                }
                cmdLen++;
            }
            else if (command_type == cmdREGULAR)
            {
                /* Regular command. */
                cmdBuf[cmdLen] = data;
                cmdLen++;
            }
            else
            {
                /* Json command. */
                cmdBuf[cmdLen] = data;
                cmdLen++;
                if (data == '{')
                {
                    brackets_cnt++;
                }
                else if (data == '}')
                {
                    brackets_cnt--;
                    /* Got a full Json command. Update the app commands buffer. */
                    if (brackets_cnt == 0)
                    {
                        cmdBuf[cmdLen] = '\n';
                        cmdLen = 0;
                        command_type = cmdUNKNOWN_TYPE;
                        ret = COMMAND_READY;
                    }
                }
            }
        }
        /* Checks if command too long and we need to reset it. */
        if (cmdLen >= MAX_CMD_LENGTH)
        {
            cmdLen = 0;
            command_type = cmdUNKNOWN_TYPE;
        }
    }
    qirq_unlock(lock);
    return ret;
}

usb_data_e usb_uart_rx(struct cc_buff *buf)
{
    usb_data_e ret = NO_DATA;

    if (buf->cnt > 0)
        ret = AppGet()->on_rx(buf);

    return ret;
}
