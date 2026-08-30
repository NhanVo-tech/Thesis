/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#ifdef SOFTDEVICE_PRESENT
#include <nrf_sdh.h>
#endif
#include <nrfx_nvmc.h>
#include <qerr.h>
#include <qirq.h>

enum qerr qflash_write(uint32_t dst_addr, void *src_addr, uint32_t size)
{
	uint32_t size32 = size / sizeof(uint32_t);
	unsigned int key;

	/* Align on words */
	if (size32 % 2 != 0) {
		size32 += 1;
	}

	key = qirq_lock();

#ifdef SOFTDEVICE_PRESENT
	uint32_t page_size = NRF_FICR->CODEPAGESIZE;
	uint32_t page_number = dst_addr / page_size;

	if (nrf_sdh_is_enabled()) {
		sd_flash_page_erase(page_number);
		sd_flash_write((uint32_t *const)dst_addr, src_addr, size32);
	} else {
		nrfx_nvmc_page_erase(dst_addr);
		nrfx_nvmc_words_write(dst_addr, src_addr, size32);
	}
#else
	nrfx_nvmc_page_erase(dst_addr);
	nrfx_nvmc_words_write(dst_addr, src_addr, size32);
#endif

	qirq_unlock(key);

	return QERR_SUCCESS;
}
