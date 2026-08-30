/**
 * @file      cmd_fn.c
 *
 * @brief     Collection of executables functions from defined known_commands[]
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "cmd_fn.h"
#include "deca_dbg.h"
#include "driver_app_config.h"
#include "HAL_uart.h"
#include "qplatform.h"
#include "qotp.h"
#include "qmalloc.h"
#include "qirq.h"
#include "controlTask.h"
#include <stdio.h>

#define MAX_OTP_LENGTH_ADD 0x07F

REG_FN(f_get_otp)
{
    const char *ret = CMD_FN_RET_KO;
    enum qerr r;
    uint32_t *buf;

    r = qplatform_init();
    if (r != QERR_SUCCESS)
    {
        diag_printf("Failed to initialize qplatform.");
        return ret;
    }

    buf = qmalloc((MAX_OTP_LENGTH_ADD + 1) * sizeof(uint32_t));
    if (!buf)
    {
        diag_printf("Failed to initialize L1 config module.");
        goto exit_qplatform_deinit;
    }

    unsigned int lock = qirq_lock();
    r = qotp_read(0, buf, MAX_OTP_LENGTH_ADD + 1);
    qirq_unlock(lock);

    if (r != QERR_SUCCESS)
    {
        diag_printf("Error while reading OTP memory.");
        goto exit_free_buf;
    }

    diag_printf("OTP CONTENT: {\r\n");
    for (int i = 0; i <= MAX_OTP_LENGTH_ADD; i++)
    {
        diag_printf("\"0x%03x\":\"0x%08x\"", i, buf[i]);
        if (i < MAX_OTP_LENGTH_ADD)
            diag_printf(",\r\n");
    }
    diag_printf("\r\n}");

    ret = CMD_FN_RET_OK;
exit_free_buf:
    qfree(buf);
exit_qplatform_deinit:
    qplatform_deinit();
    return ret;
}


const char COMMENT_GETOTP[] = {"Usage: To get OTP memory values: \"GETOTP\""};

const struct command_s known_commands_dw3xxx[] __attribute__((section(".known_commands_service"))) = {
    {"GETOTP", mIDLE, f_get_otp, COMMENT_GETOTP},
};

#ifdef USB_ENABLE
REG_FN(f_uart)
{
    uint8_t n = 0;
    int param_val = 0;
    char cmd[9];

    bool uart_allowed = is_uart_allowed();

    n = sscanf(text, "%9s %d", cmd, &param_val);

    if (n != 2)
    {
        diag_printf("UART: %d \r\n", uart_allowed);
    }
    /* Support only 0 and 1 as input parameters. */
    else if (val == 0 || val == 1)
    {
        if (uart_allowed && !val)
        {
            deca_uart_close();
        }
        else if (!uart_allowed && val)
        {
            deca_uart_init(&NotifyControlTaskData);
        }
        set_uart_allowed(val == 1);
    }
    else
    {
        diag_printf("Status value %d is not supported.\r\n", val);
        return CMD_FN_RET_KO;
    }

    return (CMD_FN_RET_OK);
}

const char COMMENT_UART[] = {"Usage: To initialize selected UART: \"UART <DEC>\""};
const struct command_s known_commands_idle_uart[] __attribute__((section(".known_commands_idle"))) = {
    {"UART", mIDLE, f_uart, COMMENT_UART},
};
#endif // USB_ENABLE
