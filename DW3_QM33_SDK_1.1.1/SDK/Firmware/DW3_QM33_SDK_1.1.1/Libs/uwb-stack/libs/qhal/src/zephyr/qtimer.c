/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include <qtimer.h>
#include <stddef.h>

struct qtimer {};

struct qtimer *qtimer_init(uint8_t qtimer_id, const struct qtimer_config *config, qtimer_cb handler,
			   void *arg)
{
	return NULL;
}

enum qerr qtimer_start(const struct qtimer *timer, uint32_t us, bool periodic)
{
	return QERR_ENOTSUP;
}

enum qerr qtimer_stop(const struct qtimer *timer)
{
	return QERR_ENOTSUP;
}

enum qerr qtimer_read(const struct qtimer *timer, uint32_t *us)
{
	return QERR_ENOTSUP;
}
