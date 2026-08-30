/**
 * @file      freertos_tasks_c_additions.h
 *
 * @brief     FreeRTOS specific function
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

uint32_t uxTaskGetStackSize(TaskHandle_t xTask)
{
	if (xTask != NULL) {
		TCB_t *pxTCB = prvGetTCBFromHandle(xTask);
		/* This offset is because the first and the last entry of the stack are not counted
		 * here. */
		return (pxTCB->pxEndOfStack - pxTCB->pxStack + 2) * sizeof(StackType_t);
	}

	return 0;
}
