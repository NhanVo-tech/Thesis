/**
 * @file      thread_fn.c
 *
 * @brief     Thread function
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "thread_fn.h"

#include <inttypes.h>

#include "FreeRTOS.h"
#include "task.h"
#include "deca_dbg.h"
#include "qprofiling.h"

const char THREAD_FN_RET_OK[] = "ok\r\n";
const char THREAD_FN_RET_KO[] = "KO\r\n";

const char *thread_fn(void)
{
    const char *ret = THREAD_FN_RET_OK;
    const uint8_t max_task_num = 10;
    struct qstackstats task_stats[max_task_num];
    struct qmemstats memory_stats;

    qmemstat_get(&memory_stats);
    int task_number = qstackstat_get(task_stats, max_task_num);

    /* For each populated position in the task_stats array,
    format the raw data as human readable ASCII data. */
    diag_printf("%-16s\t%s\r\n", "THREAD NAME", "Stack usage");
    for (int i = 0; i < task_number; i++)
    {
        diag_printf("%-16s\t%" PRIu32 "/%" PRIu32 "\r\n", task_stats[i].thread_name, task_stats[i].stack_used, task_stats[i].stack_size);
    }

    diag_printf("%-16s\t%d\r\n", "Total HEAP", memory_stats.heap_size);
    diag_printf("%-16s\t%d\r\n", "Current HEAP used", memory_stats.heap_used);
    diag_printf("%-16s\t%d\r\n", "Max HEAP used", memory_stats.heap_peak);

    ret = THREAD_FN_RET_OK;
    return (ret);
}
