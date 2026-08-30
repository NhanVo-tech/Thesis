/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include <drivers/flash.h>
#include <qerr.h>
#include <zephyr.h>

#if defined(CONFIG_FLASH) && DT_NUM_INST_STATUS_OKAY(qorvo_flash_qm35)
static const struct device *flash_dev = DEVICE_DT_GET(DT_INST(0, qorvo_flash_qm35));

enum qerr qflash_write(uint32_t dst_addr, void *src_addr, uint32_t size)
{
	if (flash_write(flash_dev, dst_addr, src_addr, size))
		return QERR_EINVAL;

	return QERR_SUCCESS;
}
#else /* CONFIG_FLASH. */
enum qerr qflash_write(uint32_t dst_addr, void *src_addr, uint32_t size)
{
	return QERR_ENOTSUP;
}
#endif /* CONFIG_FLASH. */
