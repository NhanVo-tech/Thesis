/**
 * @file      deca_debug.h
 *
 * @brief     Implementation of debug functionalities
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "deca_dbg.h"

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "reporter.h"

#define DIAG_BUF_LEN 0x8
#ifdef CONFIG_CIR_READ
/* (16*6+2 = CIR samples line = worst case). */
#define DIAG_STR_LEN (2 + 6 * CONFIG_CIR_WINDOW)
#else
#define DIAG_STR_LEN (64)
#endif

/**
 * @note (DIAG_BUF_LEN*(DIAG_STR_LEN+4)) shall be < HTTP_PAGE_MALLOC_SIZE-16
 * <br>=4
 */
typedef struct
{
    uint8_t buf[DIAG_BUF_LEN][DIAG_STR_LEN];
    int head;
} gDiagPrintFStr_t;

gDiagPrintFStr_t gDiagPrintFStr;

void diag_printf(char *s, ...)
{
    va_list args;
    va_start(args, s);
    vsnprintf((char *)(&gDiagPrintFStr.buf[gDiagPrintFStr.head][0]), DIAG_STR_LEN, s, args);
    reporter_instance.print((char *)&gDiagPrintFStr.buf[gDiagPrintFStr.head][0], strlen((char *)(&gDiagPrintFStr.buf[gDiagPrintFStr.head][0])));
    gDiagPrintFStr.head = (gDiagPrintFStr.head + 1) & (DIAG_BUF_LEN - 1);
    va_end(args);
}
