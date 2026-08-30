/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include <pm/pm.h>
#include <pm/state.h>
#include <stdbool.h>
#include <zephyr.h>

/* Zephyr version file. */
#include "qpm.h"
#include "qpwr.h"

#include <version.h>

#if (KERNEL_VERSION_MAJOR >= 3) && (KERNEL_VERSION_MINOR >= 1)
#include <pm/policy.h>
#endif

static inline enum pm_state qpm_state_to_zephyr(enum qpm_sleep_state state)
{
	switch (state) {
	case QPM_STATE_S0:
		return PM_STATE_ACTIVE;
	case QPM_STATE_S0ix:
		return PM_STATE_RUNTIME_IDLE;
	case QPM_STATE_S1:
		return PM_STATE_SUSPEND_TO_IDLE;
	case QPM_STATE_S2:
		return PM_STATE_STANDBY;
	case QPM_STATE_S3:
		return PM_STATE_SUSPEND_TO_RAM;
	case QPM_STATE_S4:
		return PM_STATE_SUSPEND_TO_DISK;
	case QPM_STATE_S5:
		return PM_STATE_SOFT_OFF;
	default:
		return PM_STATE_ACTIVE;
	}
}

#if (KERNEL_VERSION_MAJOR >= 3) && (KERNEL_VERSION_MINOR >= 1)

#define DT_PM_SUBSTATES(node_id)                                    \
	{                                                           \
		.state = PM_STATE_DT_INIT(node_id),                 \
		.substate_id = DT_PROP_OR(node_id, substate_id, 0), \
	},

static struct {
	enum pm_state state;
	uint8_t substate_id;
} substate_lock_t[] = { DT_FOREACH_STATUS_OKAY(zephyr_power_state, DT_PM_SUBSTATES) };

void qpm_sleep_state_lock(enum qpm_sleep_state state, uint8_t substate_id)
{
	uint32_t i;
	enum pm_state zephyr_state = qpm_state_to_zephyr(state);

	/* Handle power state given by `state`. */
	if (!substate_id || (substate_id == PM_ALL_SUBSTATES)) {
		/* Lock all substates of current state. */
		pm_policy_state_lock_get(zephyr_state, PM_ALL_SUBSTATES);
	} else {
		/* Lock only substates which are greater or equal to the
		 * provided substate_id. */
		for (i = 0; i < ARRAY_SIZE(substate_lock_t); i++) {
			if ((substate_lock_t[i].state == zephyr_state) &&
			    (substate_lock_t[i].substate_id >= substate_id)) {
				pm_policy_state_lock_get(zephyr_state,
							 substate_lock_t[i].substate_id);
			}
		}
	}
	/* Lock all substates for higher power modes. */
	for (i = state + 1; i <= QPM_STATE_S5; i++) {
		pm_policy_state_lock_get(qpm_state_to_zephyr(i), PM_ALL_SUBSTATES);
	}
}

void qpm_sleep_state_unlock(enum qpm_sleep_state state, uint8_t substate_id)
{
	uint32_t i;
	enum pm_state zephyr_state = qpm_state_to_zephyr(state);

	/* Handle power state given by `state`. */
	if (!substate_id || (substate_id == PM_ALL_SUBSTATES)) {
		/* Unlock all substates of current state. */
		pm_policy_state_lock_put(zephyr_state, PM_ALL_SUBSTATES);
	} else {
		/* Unlock only substates which are greater or equal to the
		 * provided substate_id. */
		for (i = 0; i < ARRAY_SIZE(substate_lock_t); i++) {
			if ((substate_lock_t[i].state == zephyr_state) &&
			    (substate_lock_t[i].substate_id >= substate_id)) {
				pm_policy_state_lock_put(zephyr_state,
							 substate_lock_t[i].substate_id);
			}
		}
	}
	/* Lock all substates for higher power modes. */
	for (i = state + 1; i <= QPM_STATE_S5; i++) {
		pm_policy_state_lock_put(qpm_state_to_zephyr(i), PM_ALL_SUBSTATES);
	}
}

bool qpm_sleep_state_is_active(enum qpm_sleep_state state, uint8_t substate_id)
{
	return !pm_policy_state_lock_is_active(qpm_state_to_zephyr(state), substate_id);
}

#else /* Zephyr version < 3.1. */

void qpm_sleep_state_lock(enum qpm_sleep_state state, uint8_t substate_id)
{
#ifdef CONFIG_PM
	int i;

	/* Since substate is not handled by Zephyr 2.7, when substate_id is
	 * not 0, we lock only from the next state.
	 * For example, if we ask to lock S3b, S3a state should not be locked,
	 * so we lock state S4 and higher. */
	if (substate_id && (substate_id != QPM_ALL_SUBSTATES))
		state++;
	for (i = state; i <= QPM_STATE_S5; i++) {
		pm_constraint_set(qpm_state_to_zephyr(i));
	}
#else
	(void)state;
	(void)substate_id;
#endif
}

void qpm_sleep_state_unlock(enum qpm_sleep_state state, uint8_t substate_id)
{
#ifdef CONFIG_PM
	int i;

	/* Since substate is not handled by Zephyr 2.7, when substate_id is
	 * not 0, we unlock only from the next state. */
	if (substate_id && (substate_id != QPM_ALL_SUBSTATES))
		state++;
	for (i = state; i <= QPM_STATE_S5; i++) {
		pm_constraint_release(qpm_state_to_zephyr(i));
	}
#else
	(void)state;
	(void)substate_id;
#endif
}

bool qpm_sleep_state_is_active(enum qpm_sleep_state state, uint8_t substate_id)
{
#ifdef CONFIG_PM
	return pm_constraint_get(qpm_state_to_zephyr(state));
#else
	return false;
#endif
}

#endif /* Zephyr version < 3.1. */

enum qerr qpm_set_low_power_mode(bool enable)
{
	/* Low power mode is enabled at boot on Zephyr.
	 * In this state, unlocking sleep states not locked would fail.
	 */
	static bool is_enabled = true;
	if (enable != is_enabled) {
		/* Grab and release all the power modes from S3 */
		if (!enable) {
			qpwr_disable_lpm();
			qpm_sleep_state_lock(QPM_STATE_S3, QPM_ALL_SUBSTATES);
		} else {
			qpwr_enable_lpm();
			qpm_sleep_state_unlock(QPM_STATE_S3, QPM_ALL_SUBSTATES);
		}
		is_enabled = enable;
	}
	return QERR_SUCCESS;
}

bool qpm_get_low_power_mode(void)
{
	return qpwr_is_lpm_enabled();
}

enum qerr qpm_get_min_inactivity_s4(uint32_t *time_ms)
{
	return qpwr_get_min_inactivity_s4(time_ms);
}

enum qerr qpm_set_min_inactivity_s4(uint32_t time_ms)
{
	return qpwr_set_min_inactivity_s4(time_ms);
}
