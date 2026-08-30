/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "config_manager_internal.h"
#include "l1_config.h"
#include "l1_config_internal.h"
#include "sha256.h"

#include <qerr.h>
#include <qflash.h>
#include <stdint.h>
#include <string.h>

#define LOG_TAG "l1_config_custom"
#include <qlog.h>

#ifdef CONFIG_L1_CONFIG_CUSTOM_DEFAULT_USE_TEXT_SECTION

static const struct l1_config default_ram_config __attribute__((aligned(8)));

/* flash write only support address aligned to 8 bytes. */
static const uint32_t default_ram_config_hash[SHA256_BLOCK_SIZE / sizeof(uint32_t)]
	__attribute__((aligned(8)));

const uint32_t *l1_config_get_persistent_memory(void)
{
	return (const uint32_t *)&default_ram_config;
}

const uint32_t *l1_config_get_persistent_memory_hash_addr(void)
{
	return (const uint32_t *)default_ram_config_hash;
}

#elif defined(CONFIG_L1_CONFIG_CUSTOM_DEFAULT_USE_DEDICATED_SECTION)

#ifndef CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SIZE
#error "CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SIZE must be defined"
#endif

_Static_assert(sizeof(struct l1_config) <= CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SIZE,
	       "The persistent storage region is smaller than the structure stored on it...");

#ifndef CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SHA256_SIZE
#error "CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SHA256_SIZE must be defined"
#endif

_Static_assert(
	SHA256_BLOCK_SIZE <= CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SHA256_SIZE,
	"The persistent storage sha256 section is smaller than the structure stored on it...");

__attribute__((
	section(".l1_config_persist_storage"))) static const struct l1_config internal_l1_config;

__attribute__((section(".l1_config_persist_storage_sha256"))) static const uint32_t
	l1_config_persist_storage_sha256;

const uint32_t *persistent_ram_config = (uint32_t *)&internal_l1_config;

const uint32_t *persistent_ram_config_hash = (uint32_t *)&l1_config_persist_storage_sha256;

const uint32_t *l1_config_get_persistent_memory(void)
{
	return persistent_ram_config;
}

const uint32_t *l1_config_get_persistent_memory_hash_addr(void)
{
	return persistent_ram_config_hash;
}

#endif /* CONFIG_L1_CONFIG_CUSTOM_DEFAULT_USE_TEXT_SECTION */

#ifndef SECURED_FW
#define SECURED_FW false
#endif

STATIC bool is_device_secured(void)
{
	return SECURED_FW == true;
}

STATIC void l1_config_compute_sha256(const uint8_t data[], size_t len,
				     uint8_t hash[SHA256_BLOCK_SIZE])
{
	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, data, len);
	sha256_final(&ctx, hash);
}

enum qerr l1_config_load_from_persistent_memory(struct l1_config *l1_config)
{
	uint8_t hash[SHA256_BLOCK_SIZE];
	uint8_t rram_hash[SHA256_BLOCK_SIZE];
	uintptr_t l1_config_addr = (uintptr_t)l1_config_get_persistent_memory();
	uintptr_t hash_addr = (uintptr_t)l1_config_get_persistent_memory_hash_addr();
	memcpy(l1_config, (void *)l1_config_addr, sizeof(struct l1_config));
	memcpy(rram_hash, (void *)hash_addr, SHA256_BLOCK_SIZE);

	/* Compute a SHA256 hash on the whole configuration, and compare with the hash stored in
	 * RRAM. */
	l1_config_compute_sha256((unsigned char *)l1_config_addr, l1_config->size, hash);
	if (memcmp(hash, rram_hash, SHA256_BLOCK_SIZE)) {
		QLOGE("%s: hash differs from the one stored", __func__);
		return QERR_EIO;
	};
	return QERR_SUCCESS;
}

enum qerr l1_config_store_to_persistent_memory(struct l1_config *l1_config)
{
	const uint32_t l1_config_addr = (uint32_t)(uintptr_t)l1_config_get_persistent_memory();
	const uint32_t hash_addr = (uint32_t)(uintptr_t)l1_config_get_persistent_memory_hash_addr();
	uint8_t hash[SHA256_BLOCK_SIZE];
	enum qerr r;

	/* Update configuration  in persistent memory only if the firmware is not in secured state.
	 */
	if (is_device_secured())
		return QERR_SUCCESS;

	/* Compute a SHA256 on full config structure. */
	l1_config_compute_sha256((unsigned char *)l1_config, l1_config->size, hash);

	/* First write the L1 config structure. */
	r = qflash_write(l1_config_addr, (void *)l1_config, l1_config->size);
	if (r)
		return r;

	/* Then write its hash. */
	return qflash_write(hash_addr, hash, SHA256_BLOCK_SIZE);
}
