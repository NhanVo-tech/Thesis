/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#ifndef QPRIVATE_H
#define QPRIVATE_H

#include <stdbool.h>

/**
 * qprivate_is_in_isr() - Indicate if execution is in IRQ or not.
 *
 * Return: true if execution is in IRQ.
 */
bool qprivate_is_in_isr(void);

#endif /* QPRIVATE_H */
