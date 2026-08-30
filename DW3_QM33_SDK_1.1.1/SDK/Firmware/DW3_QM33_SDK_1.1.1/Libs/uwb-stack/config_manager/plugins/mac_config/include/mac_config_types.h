/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * enum mac_session_scheduler_id - Session scheduler type identifier in the MAC.
 *
 * @MAC_SESSION_SCHEDULER_ID_STRICT: Strict priority scheduler.
 * @MAC_SESSION_SCHEDULER_ID_FAIR: Weight priority scheduler (Called fair too).
 * @MAC_SESSION_SCHEDULER_ID_NUM: Number of valid values in the enum.
 */
enum mac_session_scheduler_id {
	MAC_SESSION_SCHEDULER_ID_STRICT,
	MAC_SESSION_SCHEDULER_ID_FAIR,
	MAC_SESSION_SCHEDULER_ID_NUM,
};

#ifdef __cplusplus
}
#endif
