/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <qatomic_impl.h>

#ifndef QATOMIC

#include <stdatomic.h>

/**
 * qatomic_int - See atomic_int in GCC documentation.
 */
#define qatomic_int atomic_int
/**
 * qatomic_bool - See atomic_bool in GCC documentation.
 */
#define qatomic_bool atomic_bool

/**
 * qatomic_init() - See atomic_init in GCC documentation.
 * @x: The pointer to the qatomic_* variable to initialize.
 * @value: The value to initialize it to.
 */
#define qatomic_init(x, value) atomic_init(x, value)
/**
 * qatomic_load() - See atomic_load in GCC documentation.
 * @x: The pointer to the qatomic_* variable to load.
 * Returns: The value of the variable.
 */
#define qatomic_load(x) atomic_load(x)
/**
 * qatomic_store() - See atomic_store in GCC documentation.
 * @x: The pointer to the qatomic_* variable to load.
 * @value: The value to store.
 */
#define qatomic_store(x, value) atomic_store(x, value)
/**
 * qatomic_exchange() - See atomic_exchange in GCC documentation.
 * @x: The pointer to the qatomic_* variable to exchange.
 * @value: The value to exchange it with.
 * Returns: The old value.
 */
#define qatomic_exchange(x, value) atomic_exchange(x, value)
/**
 * qatomic_fetch_add() - See atomic_fetch_add in GCC documentation.
 * @x: The pointer to the qatomic_* variable to add to.
 * @value: The value to add.
 * Returns: The old value.
 */
#define qatomic_fetch_add(x, value) atomic_fetch_add(x, value)
/**
 * qatomic_fetch_sub() - See atomic_fetch_sub in GCC documentation.
 * @x: The pointer to the qatomic_* variable to substract to.
 * @value: The value to add.
 * Returns: The old value.
 */
#define qatomic_fetch_sub(x, value) atomic_fetch_sub(x, value)

#endif /* QATOMIC */
