/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include "l1_config_keys.h"
#include "l1_config_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * struct l1_config_key_info - Key informations.
 * @offset: Offset of the current section of the key.
 * @length: Length.
 */
struct l1_config_key_info {
	uint16_t offset;
	uint16_t length;
};

/**
 * struct l1_config_key_itemized_info - Itemized key informations.
 * @nb_items: Number of items in the current section.
 * @next_sections_table_size: Size of the table containing next sections
 * available for current one.
 * @next_sections_table: Table containing next sections available for current
 * one.
 */
struct l1_config_key_itemized_info {
	uint8_t nb_items;
	uint8_t next_sections_table_size;
	const struct l1_config_key_section *next_sections_table;
};

/**
 * struct l1_config_key_section - Key section informations.
 * @key: Key string of the current section.
 * @info: Informations of the current section.
 * @itemized_info: Informations for an itemized section.
 */
struct l1_config_key_section {
	const char *const key;
	struct l1_config_key_info info;
	struct l1_config_key_itemized_info itemized_info;
};

#define NOT_ITEMIZED       \
	{                  \
		0, 0, NULL \
	}

#ifndef sizeof_field
#define sizeof_field(type, memb) sizeof((((type *)0)->memb))
#endif

#define KEY_OFFSET(m) offsetof(struct l1_config, params.m)
#define KEY_SIZE(m) sizeof_field(struct l1_config, params.m)
#define KEY_INFO(m)                                            \
	{                                                      \
		.offset = KEY_OFFSET(m), .length = KEY_SIZE(m) \
	}

#define IS_PARAM(m) (offset == KEY_OFFSET(m))

/**
 * struct l1_config - L1 configuration.
 * @version: L1 config version.
 * @size: Size of the l1_config structure stored in the NVME. Used to compute the hash at load.
 * @rfu: Reserved for Future Usage.
 * @params: L1 config parameters.
 */
struct l1_config {
	uint32_t version;
	uint32_t size;
	uint32_t rfu;
	struct l1_config_params params;
};

/**
 * l1_config_policy_check_bool - Policy check for booleans.
 * @value: Value to check.
 *
 * Return: QERR_SUCCESS if policy check succeeds, error otherwise.
 */
enum qerr l1_config_policy_check_bool(void *value);

/**
 * l1_config_policy_check - Policy check.
 * @offset: Offset of the current section of the key.
 * @value: Value to update.
 *
 * Return: QERR_SUCCESS if policy check succeeds, error otherwise.
 */
enum qerr l1_config_policy_check(uint16_t offset, void *value);

#ifndef CONFIG_L1_CONFIG_VOLATILE
/**
 * l1_config_load_from_persistent_memory - Load Configuration and calibration
 * data structure from persistent memory to RAM.
 * @l1_config: pointer to RAM Configuration and Calibration data.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr l1_config_load_from_persistent_memory(struct l1_config *l1_config);

/**
 * l1_config_store_to_persistent_memory - Store Configuration and calibration
 * data structure to persistent memory.
 * @l1_config: pointer to RAM Configuration and Calibration data.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr l1_config_store_to_persistent_memory(struct l1_config *l1_config);

/**
 * l1_config_get_persistent_memory - Get address of L1 configuration structure in persistent memory.
 *
 * Return: structure address in persistent memory.
 */
const uint32_t *l1_config_get_persistent_memory(void);

/**
 * l1_config_get_persistent_memory_hash_addr - Get address of L1 configuration HASH in persistent
 * memory.
 *
 * Return: HASH address in persistent memory.
 */
const uint32_t *l1_config_get_persistent_memory_hash_addr(void);

#endif /* CONFIG_L1_CONFIG_VOLATILE. */

#ifdef __cplusplus
}
#endif
