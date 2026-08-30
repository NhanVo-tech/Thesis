/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <qerr.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum length of configuration key string. */
#define L1_CONFIG_KEY_NAME_MAX_LEN 64

/**
 * l1_config_get_key_name - Get name of key specific index.
 * @key_idx: Index of the key to retrieve name of.
 * @key: Pointer to string where key name will be copied. The caller is
 * responsible to provide a valid and big enough memory zone, i.e. which size is
 * greater or equal to L1_CONFIG_KEY_NAME_MAX_LEN.
 *
 * Return: QERR_SUCCESS if key index if found, else QERR_ENOENT.
 */
enum qerr l1_config_get_key_name(uint16_t key_idx, char *key);

/**
 * l1_config_transaction_start - Start a transaction in order to update configuration value(s).
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr l1_config_transaction_start(void);

/**
 * l1_config_transaction_end - End the ongoing configuration transaction.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr l1_config_transaction_end(void);

/**
 * l1_config_store_key - Store a key value in L1 configuration.
 * @key: key string to store value of.
 * @value: value to store.
 * @length: length to store.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr l1_config_store_key(const char *key, void *value, size_t length);

/**
 * l1_config_read_key - Read a key value from L1 configuration.
 * @key: key string to read value of.
 * @value: address where to copy read data.
 * @length: maximum length that can be read.
 *
 * Return: A negative error code on failure, else length of field corresponding
 * to key string.
 *
 */
int l1_config_read_key(const char *key, void *value, size_t length);

/**
 * l1_config_reset_to_default - Reset values to default for all keys.
 *
 * Return: QERR_SUCCESS or error.
 *
 */
enum qerr l1_config_reset_to_default(void);

#ifdef __cplusplus
}
#endif
