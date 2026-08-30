/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include <persistent_time.h>
#include <qrtc.h>

inline int64_t qrtc_get_us(void)
{
	return persistent_time_get_rtc_us();
}

inline void qrtc_resync_rtc_systime(int64_t *rtc_us, uint32_t *systime)
{
	persistent_time_resync_rtc_systime(rtc_us, systime);
}

void qrtc_update_rtc_systime(int64_t updated_rtc_us, uint32_t updated_systime)
{
	persistent_time_update_rtc_systime(updated_rtc_us, updated_systime);
}
