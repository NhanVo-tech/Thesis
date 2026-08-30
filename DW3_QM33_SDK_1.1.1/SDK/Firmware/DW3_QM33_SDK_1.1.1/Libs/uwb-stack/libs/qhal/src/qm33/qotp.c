/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include <deca_device_api.h>
#include <qerr.h>
#include <qirq.h>
#include <qotp.h>
#include <qpwr.h>

#define MAX_OTP_ADDR 0x7FU

enum qerr qotp_read(uint32_t address, uint32_t *data, uint8_t length)
{
	unsigned int key;
	enum qerr ret = QERR_SUCCESS;
	bool uwb_is_sleeping;

	if (length == 0) {
		return QERR_EINVAL;
	}
	if ((address > MAX_OTP_ADDR) || ((address + length) > MAX_OTP_ADDR + 1)) {
		return QERR_EFAULT;
	}

	key = qirq_lock();
	uwb_is_sleeping = qpwr_uwb_is_sleeping();
	if (uwb_is_sleeping) {
		ret = qpwr_uwb_wakeup();
		if (ret != QERR_SUCCESS) {
			qirq_unlock(key);
			return ret;
		}
	}
	dwt_otpread((uint16_t)address, data, length);

	if (uwb_is_sleeping) {
		ret = qpwr_uwb_sleep();
	}
	qirq_unlock(key);
	return ret;
}

enum qerr qotp_write(uint32_t value, uint32_t address)
{
	int error;
	unsigned int key;
	enum qerr ret = QERR_SUCCESS;
	bool uwb_is_sleeping;

	if (address > MAX_OTP_ADDR) {
		return QERR_EFAULT;
	}

	key = qirq_lock();
	uwb_is_sleeping = qpwr_uwb_is_sleeping();
	if (uwb_is_sleeping) {
		ret = qpwr_uwb_wakeup();
		if (ret != QERR_SUCCESS) {
			qirq_unlock(key);
			return ret;
		}
	}

	error = dwt_otpwrite(value, (uint16_t)address);

	if (uwb_is_sleeping) {
		ret = qpwr_uwb_sleep();
	}
	qirq_unlock(key);
	return (error != 0 ? QERR_EFAULT : ret);
}
