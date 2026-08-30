/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "l1_config.h"

#include "config_manager_internal.h"
#include "l1_config_internal.h"
#include "qassert.h"

#ifdef CONFIG_L1_CONFIG_OTP_PA_OFFSET
/* TODO: This include is needed because of dwt_get_pa_offset_otp definition, until
 * OTP storage manager is ready.
 */
#include "uwb_device_api.h"
#endif

#include <stdlib.h>
#include <string.h>

#define LOG_TAG "l1_config"
#include "qlog.h"

struct l1_config l1_config;
STATIC struct l1_config_platform_ops platform_ops;

/* Check that expected defines exist. */
#ifndef CONFIG_L1_CONFIG_ANT_SET_NUM
#error "CONFIG_L1_CONFIG_ANT_SET_NUM define is missing. It should correspond to \
	maximum number of antenna sets needed for the project."
#elif CONFIG_L1_CONFIG_ANT_SET_NUM > 8
#error "CONFIG_L1_CONFIG_ANT_SET_NUM maximum value supported is 8."
#endif

#ifndef CONFIG_L1_CONFIG_ANT_NUM
#error "CONFIG_L1_CONFIG_ANT_NUM define is missing. It should correspond to \
	maximum number of antennas on the board."
#elif CONFIG_L1_CONFIG_ANT_NUM > 8
#error "CONFIG_L1_CONFIG_ANT_NUM maximum value supported is 8."
#endif

#ifndef CONFIG_L1_CONFIG_OLD_ANTENNA_MANAGEMENT_MODEL
#ifndef CONFIG_L1_CONFIG_ANT_PAIR_NUM
#error "CONFIG_L1_CONFIG_ANT_PAIR_NUM define is missing. It should correspond \
	to maximum number of antennas on the board."
#elif CONFIG_L1_CONFIG_ANT_PAIR_NUM > 8
#error "CONFIG_L1_CONFIG_ANT_PAIR_NUM maximum value supported is 8."
#endif
#endif

/* Default config params is defined in the generated file. */
extern const struct l1_config default_config;

/**
 * l1_config_search_key_recursive - Search offset and length of the parameter corresponding to the
 * key name provided as argument.
 * @key: Key name for which informations must be retrieved.
 * @offset: Returned offset in the Configuration and Calibration structure.
 * @key_section_table: Current parsed table containing key section informations.
 * @key_section_table_size: Size of the current parsed section table.
 *
 * Return: length of field corresponding to key string if key has been found, or QERR_ENOENT if not
 * found.
 *
 * Note: that function is recursive: it checks if one of the strings contained in the current
 * key_section table corresponds to the key to find. If it does:
 *  - If that section key is itemized, it calls itself recursively in order to look for the next
 *    part of the key.
 *  - Otherwise, it considers the key has been found entirely, and returns the length of the value
 *    corresponding to the found key.
 */
STATIC int l1_config_search_key_recursive(const char *key, uint16_t *offset,
					  const struct l1_config_key_section *key_section_table,
					  int key_section_table_size)
{
	int length = QERR_ENOENT;
	int str_offset = 0;

	for (int i = 0; i < key_section_table_size; i++) {
		const struct l1_config_key_section *key_section = &key_section_table[i];
		if (strncmp(key_section->key, key, strlen(key_section->key)) == 0) {
			const struct l1_config_key_itemized_info *itemized_info =
				&key_section->itemized_info;

			if (!itemized_info->nb_items) {
				/* Current key section is not itemized.
				 * So we reached the last section of the key.
				 */

				/* Check if we found the exact same key, and not
				 * a key starting similarly. */
				if (strlen(key_section->key) != strlen(key))
					continue;

				/* Retrieve info and exit. */
				*offset = key_section->info.offset;
				length = key_section->info.length;
				break;
			} else {
				/* Current key section is itemized. */
				uint8_t idx;
				char c = *(key + strlen(key_section->key));

				/* Check if the next character in the key is a number, otherwise, it
				 * is not the good key.*/
				if ((c < '0') || (c > '9')) {
#ifdef CONFIG_L1_CONFIG_OLD_ANTENNA_MANAGEMENT_MODEL
					/* Workaround for axis x,y,z => convert to idx 0,1,2. */
					if (strncmp(key, ".pdoa.axis", strlen(".pdoa.axis")) == 0) {
						char c = *(key + strlen(key_section->key));
						idx = c - 'x';
						if (idx > 2)
							return QERR_ENOENT;
					} else
#endif
					{
						continue;
					}
				} else {
					idx = atoi(key + strlen(key_section->key));
				}

				/* LCOV_EXCL_START. */
				QASSERT(key_section->itemized_info.next_sections_table != NULL);
				QASSERT(key_section->itemized_info.next_sections_table_size);
				/* LCOV_EXCL_STOP. */

				/* Workaround for ch5/9 => convert to idx 0 & 1.
				 */
				if (!strncmp(key, "ch", strlen("ch")) ||
				    !strncmp(key, ".ch", strlen(".ch"))) {
					if (idx == 5)
						idx = 0;
					else if (idx == 9)
						idx = 1;
					else
						return QERR_ENOENT;
				}

				/* Check index of the itemized key. */
				if (idx >= key_section->itemized_info.nb_items)
					return QERR_ENOENT;

				/* Increment string index to the beginning of
				 * the section. */
				str_offset += strlen(key_section->key) + 1;

				/* Call l1_config_search_key_recursive recursively, until the last
				 * section is found. */
				length = l1_config_search_key_recursive(
					key + str_offset, offset,
					itemized_info->next_sections_table,
					itemized_info->next_sections_table_size);
				if (length > 0) {
					/* Add offset corresponding to the item
					 * number "index". */
					*offset += (key_section->info.length * idx);
					/* Add offset corresponding to the
					 * offset in the current section. */
					*offset += key_section->info.offset;
					break;
				}
			}
		}
	}
	return length;
}

extern const struct l1_config_key_section root_keys[NB_ROOT_KEY_SECTIONS];

STATIC int l1_config_parse_key(const char *key, uint16_t *offset)
{
	*offset = 0;
	return l1_config_search_key_recursive(key, offset, root_keys, NB_ROOT_KEY_SECTIONS);
}

/**
 * l1_config_get_key_name_recursive - Search the key name corresponding to the index
 * provided as argument.
 * @key: Key name to fill-in.
 * @key_section_table: Current parsed table containing key section informations.
 * @key_section_table_size: Size of the current parsed section table.
 * @cur_idx: Index of the current key parsed.
 * @key_idx: Index of the key to get name of.
 *
 * Return: true if key has been found, false otherwise.
 */
STATIC bool l1_config_get_key_name_recursive(char *key,
					     const struct l1_config_key_section *key_section_table,
					     int key_section_table_size, int *cur_idx, int key_idx)
{
	bool found = false;
	for (int i = 0; i < key_section_table_size; i++) {
		const struct l1_config_key_section *key_section = &key_section_table[i];
		const struct l1_config_key_itemized_info *itemized_info =
			&key_section->itemized_info;
		if (!itemized_info->nb_items) {
			if (*cur_idx == key_idx) {
				strcat(key, key_section->key);
				return true;
			} else {
				(*cur_idx)++;
				continue;
			}
		}
		for (int j = 0; j < itemized_info->nb_items; j++) {
			char str_idx[] = "0";

			/* Workaround for ch5/9 => convert idx 0 & 1. */
			if (!strcmp(key_section->key, "ch") || !strcmp(key_section->key, ".ch")) {
				if (j == 0)
					str_idx[0] = '5';
				else
					str_idx[0] = '9';
			}
#ifdef CONFIG_L1_CONFIG_OLD_ANTENNA_MANAGEMENT_MODEL
			/* Workaround for axis x,y,z => convert to idx 0,1,2. */
			else if (strcmp(key_section->key, ".pdoa.axis") == 0) {
				str_idx[0] = 'x' + j;
			}
#endif
			else {
				str_idx[0] += j;
			}

			found = l1_config_get_key_name_recursive(
				key, itemized_info->next_sections_table,
				itemized_info->next_sections_table_size, cur_idx, key_idx);
			if (found) {
				/* Append prefix at the beginning of the key
				 * string. */
				char tmp[L1_CONFIG_KEY_NAME_MAX_LEN + 1] = { '\0' };
				strcpy(tmp, key);
				strcpy(key, key_section->key);
				strcat(key, str_idx);
				strcat(key, tmp);
				return found;
			}
		}
	}
	return found;
}

enum qerr l1_config_get_key_name(uint16_t key_idx, char *key)
{
	bool found;
	int cur_idx = 0;

	key[0] = 0;
	found = l1_config_get_key_name_recursive(key, root_keys, NB_ROOT_KEY_SECTIONS, &cur_idx,
						 key_idx);
	if (!found)
		return QERR_ENOENT;
	return QERR_SUCCESS;
}

#ifndef CONFIG_L1_CONFIG_VOLATILE

#ifndef DEVICE_STATE_RRAM
#define DEVICE_STATE_RRAM ((uint8_t *)&protected_uwb_config)
static uint8_t protected_uwb_config = false;
#endif

STATIC bool l1_config_is_uwb_config_protected(void)
{
	return *DEVICE_STATE_RRAM == true;
}

#else /* CONFIG_L1_CONFIG_VOLATILE. */
STATIC bool l1_config_is_uwb_config_protected(void)
{
	return false;
}
#endif /* CONFIG_L1_CONFIG_VOLATILE. */

#ifdef CONFIG_L1_CONFIG_OTP_PA_OFFSET

#define foreach_ant_port_chan_id(port, ch_idx)      \
	for (port = 0; port < ANT_PORT_MAX; port++) \
		for (ch_idx = 0; ch_idx < CHANNELS_NUM; ch_idx++)

/**
 * l1_config_check_update_otp_pa - Update the value of the pa_offset_gain with the OTP one
 * if it's present and if the input value is PA_GAIN_OFFSET_INVALID.
 *
 * @offset: Offset of the current section of the key.
 * @value: Value to update.
 * @len: Length of the value to update.
 *
 * Return: QERR_SUCCESS if the value has been updated with the one from OTP,
 * QERR_EINVAL otherwise.
 */
STATIC enum qerr l1_config_check_update_otp_pa(uint16_t offset, void *value, size_t len)
{
	uint8_t ant_port;
	uint8_t ch_idx;
	int8_t *v = (int8_t *)value;

	if (*v != PA_GAIN_OFFSET_INVALID)
		return QERR_EINVAL;

	foreach_ant_port_chan_id(ant_port, ch_idx)
	{
		if (offset == KEY_OFFSET(ant[ant_port].ch[ch_idx].pa_gain_offset)) {
			uint8_t chan_number = ch_idx == CHANNELS_9 ? 9 : 5;
			int r;

			r = dwt_get_pa_offset_otp(1 + ant_port, chan_number, v);
			if (r)
				return QERR_EINVAL;

			/*
			 * PA_GAIN_OFFSET_INVALID is a valid value in OTP but
			 * not to be used within the firmware, so force it to 0.
			 */
			if (*v == PA_GAIN_OFFSET_INVALID)
				*v = 0;

			memcpy((uint8_t *)&l1_config + offset, value, len);
			return QERR_SUCCESS;
		}
	}
	return QERR_EINVAL;
}

/**
 * l1_config_check_update_otp_common - Check if the value of the key is specific and
 * require an overwrite from OTP value. If it's the case update the value.
 *
 * @offset: Offset of the current section of the key.
 * @value: Value to update.
 * @len: Length of the value to update.
 *
 * Return: QERR_SUCCESS if the value has been updated with the one from OTP,
 * error otherwise.
 */
STATIC enum qerr l1_config_check_update_otp_common(uint16_t offset, void *value, size_t len)
{
	/* pa_gain_offset keys. */
	if (IS_ANT_CH_PARAM(pa_gain_offset)) {
		return l1_config_check_update_otp_pa(offset, value, len);
	}

	return QERR_EINVAL;
}
#endif /* CONFIG_L1_CONFIG_OTP_PA_OFFSET */

enum qerr l1_config_policy_check_bool(void *value)
{
	uint8_t v = *(uint8_t *)value;
	if (v > 1)
		return QERR_ERANGE;
	else
		return QERR_SUCCESS;
}

/**
 * l1_config_policy_check_manual - Additionnal "manual" policy checks.
 * @offset: Offset of the current section of the key.
 * @value: Value to update.
 *
 * That function allows to add policy checks for specific keys, that could not be automatically
 * generated via IDL, because too specific.
 *
 * Return: QERR_SUCCESS if policy check suceeds, error otherwise.
 */
enum qerr l1_config_policy_check_manual(uint16_t offset, void *value)
{
	if (IS_REF_FRAME_PARAM(phy_cfg)) {
		/* Additional checks for reference frame PHY config. */
		struct ref_frame_phy_cfg ref_frame_cfg = *(struct ref_frame_phy_cfg *)value;

		/* Reference frame #0 must always be BPRF. */
		if ((offset == KEY_OFFSET(ref_frame[0].phy_cfg)) &&
		    (ref_frame_cfg.prf == REF_FRAME_PRF_HPRF))
			return QERR_ERANGE;

		return QERR_SUCCESS;
	}

#ifdef CONFIG_L1_CONFIG_OLD_ANTENNA_MANAGEMENT_MODEL
	if (IS_ANT_GRP_PARAM(tx_power_control)) {
#else
	if (IS_ANT_SET_PARAM(tx_power_control)) {
#endif
		struct tx_power_control tx_power_control = *(struct tx_power_control *)value;
		if (tx_power_control.rfu)
			return QERR_ERANGE;
		else if ((tx_power_control.frame || tx_power_control.post_tx) &&
			 (tx_power_control.debug))
			return QERR_ERANGE;
		else
			return QERR_SUCCESS;
	}

	return QERR_SUCCESS;
}

/* Temporary workaround to avoid starting a transaction for each call to config_manager_set_value
 * from platform specific reset_to_default ops.
 * [TODO UWB-12568]: remove that workaround once we do have the real transactional system. */
STATIC bool must_commit_transaction = true;

STATIC bool transaction_started = false;
enum qerr l1_config_transaction_start(void)
{
	if (transaction_started)
		return QERR_EBUSY;

	transaction_started = true;
	must_commit_transaction = false;

	return QERR_SUCCESS;
}

enum qerr l1_config_transaction_end(void)
{
	if (!transaction_started)
		return QERR_ENOENT;

	must_commit_transaction = true;
	transaction_started = false;

	return l1_config_store_to_persistent_memory(&l1_config);
}

enum qerr l1_config_store_key(const char *key, void *value, size_t length)
{
	uint16_t offset;
	int len;
	int r = 0;

	/* Sanity checks. */
	if (!key || !value || !length) {
		QLOGE("%s: invalid param", __func__);
		return QERR_EINVAL;
	}

	if (l1_config_is_uwb_config_protected()) {
		return QERR_SUCCESS;
	}

	/* Search parameter. */
	len = l1_config_parse_key(key, &offset);
	if (len < 0) {
		QLOGE("%s: l1_config_parse_key %s error %d", __func__, key, len);
		return len;
	}
	if (len != (int)length) {
		QLOGE("%s %s: provided buffer length %zu, expected %d", __func__, key, length, len);
		return QERR_EINVAL;
	}

	/* LCOV_EXCL_START. */
	QASSERT(offset >= offsetof(struct l1_config, params));
	QASSERT(offset < sizeof(struct l1_config));
	QASSERT(offset + (uint16_t)len <= sizeof(struct l1_config));
	/* LCOV_EXCL_STOP. */

	/* Check if value is valid using generated checkers. */
	r = l1_config_policy_check(offset, value);
	if (r) {
		/* For Ref Frame PHY cfg, check if value is the special unset value (valid). */
		if (IS_REF_FRAME_PARAM(phy_cfg)) {
			uint32_t ref_frame_cfg_invalid = REF_FRAME_PHY_CFG_INVALID;
			if (!memcmp(value, &ref_frame_cfg_invalid, REF_FRAME_PHY_CFG_SIZE))
				r = QERR_SUCCESS;
		}
	}

	if (r) {
		QLOGE("%s %s: l1_config_policy_check failed %d\n", __func__, key, r);
		return r;
	}

	/* Some policy check are added manually because too specific to be generated. */
	r = l1_config_policy_check_manual(offset, value);
	if (r) {
		QLOGE("%s %s: l1_config_policy_check_manual failed %d\n", __func__, key, r);
		return r;
	}

#ifdef CONFIG_L1_CONFIG_OTP_PA_OFFSET
	/* Update config from OTP if needed for this parameter. */
	r = l1_config_check_update_otp_common(offset, value, len);

	/* Update config in SRAM. */
	if (r)
		memcpy((uint8_t *)&l1_config + offset, value, len);
#else
	memcpy((uint8_t *)&l1_config + offset, value, len);
#endif /* CONFIG_L1_CONFIG_OTP_PA_OFFSET */

#ifdef CONFIG_L1_CONFIG_VOLATILE
	return QERR_SUCCESS;
#else
	if (must_commit_transaction)
		return l1_config_store_to_persistent_memory(&l1_config);
	else
		return QERR_SUCCESS;
#endif
}

int l1_config_read_key(const char *key, void *value, size_t length)
{
	uint16_t offset;
	int len;

	if (!key || !value)
		return QERR_EINVAL;

	/* Search key offset and length. */
	len = l1_config_parse_key(key, &offset);
	if (len < 0) {
		QLOGE("l1_config_parse_key error %d", len);
		return len;
	}

	if ((uint16_t)len > length) {
		/* Provided buffer size isn't enough, return an error. */
		return QERR_ENOSPC;
	}

	/* LCOV_EXCL_START. */
	QASSERT(offset >= offsetof(struct l1_config, params));
	QASSERT(offset < sizeof(struct l1_config));
	QASSERT(offset + (uint16_t)len <= sizeof(struct l1_config));
	/* LCOV_EXCL_STOP. */

	memcpy(value, (uint8_t *)&l1_config + offset, len);

	return len;
}

#ifdef CONFIG_L1_CONFIG_OLD_ANTENNA_MANAGEMENT_MODEL
STATIC INLINE void l1_config_add_all_default_antennas(void)
{
	for (int i = 0; i < ANT_PORT_MAX; i++) {
		l1_config.params.ant[i].config.port = 1 + i;
	}
}

#else /* CONFIG_L1_CONFIG_OLD_ANTENNA_MANAGEMENT_MODEL. */
STATIC INLINE void l1_config_add_default_antenna(uint8_t ant_idx, enum transceiver transceiver,
						 uint8_t port, uint8_t rx_lna)
{
	struct antenna_path *def = &l1_config.params.ant[ant_idx];
	def->transceiver = transceiver;
	def->port = port;
	def->ext_sw_cfg = 0;
	def->lna = rx_lna;
}

#define DEFAULT_ANT_TX_IDX 0
#define DEFAULT_ANT_RX_IDX 1

STATIC INLINE void l1_config_add_all_default_antennas(void)
{
	struct antenna_set *def;

	/* Create default TX Antenna: uses TX_ANT1. */
	l1_config_add_default_antenna(DEFAULT_ANT_TX_IDX, TRANSCEIVER_TX, 1, 0);
	/* Create default RX Antenna: uses RXA_ANT3. LNA is off. */
	l1_config_add_default_antenna(DEFAULT_ANT_RX_IDX, TRANSCEIVER_RXA, 3, 0);
	/* Create Antenna Set 0 containing those two antennas. */
	def = &l1_config.params.ant_set[0];
	def->tx_ant_path = DEFAULT_ANT_TX_IDX;
	def->rx_ants[0] = DEFAULT_ANT_RX_IDX;
	def->nb_rx_ants = 1;
	def->rx_ants_are_pairs = false;
	/* No default Antenna Pair, since no PDoA by default. */
}
#endif /* CONFIG_L1_CONFIG_OLD_ANTENNA_MANAGEMENT_MODEL. */

#ifdef CONFIG_L1_CONFIG_OTP_PA_OFFSET
/**
 * l1_config_add_all_default_pa_gain_offset - Apply the OTP value for the pa_gain_offset
 * parameter if found in OTP. Otherwise PA_GAIN_OFFSET_INVALID value is set.
 */
STATIC INLINE void l1_config_add_all_default_pa_gain_offset(void)
{
	int r;
	uint8_t ant_port;
	uint8_t ch_idx;
	int8_t pa_gain_offset;

	foreach_ant_port_chan_id(ant_port, ch_idx)
	{
		uint8_t chan_number = ch_idx == CHANNELS_9 ? 9 : 5;
		r = dwt_get_pa_offset_otp(1 + ant_port, chan_number, &pa_gain_offset);

		if (r)
			l1_config.params.ant[ant_port].ch[ch_idx].pa_gain_offset =
				PA_GAIN_OFFSET_INVALID;
		else {
			/*
			 * PA_GAIN_OFFSET_INVALID is a valid value in OTP but
			 * not to be used within the firmware, so force it to 0.
			 */
			if (pa_gain_offset == PA_GAIN_OFFSET_INVALID)
				pa_gain_offset = 0;

			l1_config.params.ant[ant_port].ch[ch_idx].pa_gain_offset = pa_gain_offset;
		}
	}
}
#endif /* CONFIG_L1_CONFIG_OTP_PA_OFFSET */

/* Default Ref Frame #0. */
static const struct ref_frame ref_frame0 = { .phy_cfg = { .prf = REF_FRAME_PRF_BPRF,
							  .sfd_type = REF_FRAME_SFD_IEEE_4Z_8,
							  .psr = REF_FRAME_PSR_64,
							  .data_rate = REF_FRAME_DATA_RATE_6M8,
							  .phr_rate = REF_FRAME_BPRF_PHR_RATE_STD,
							  .seg_num = 1,
							  .seg_len = REF_FRAME_SEG_LEN_64 },
					     .payload_size = 127 };

STATIC INLINE void l1_config_add_all_default_ref_frames(void)
{
	memcpy(&l1_config.params.ref_frame[0], &ref_frame0, sizeof(ref_frame0));
}

enum qerr l1_config_reset_to_default(void)
{
	enum qerr r = QERR_SUCCESS;

	memcpy(&l1_config, &default_config, sizeof(struct l1_config));

	/* Reset default configs which depend on index. */
	l1_config_add_all_default_antennas();

	/* Reset reference frames which depend on index. */
	l1_config_add_all_default_ref_frames();

#ifdef CONFIG_L1_CONFIG_OTP_PA_OFFSET
	/*
	 * Apply the OTP value for the pa_gain_offset parameter if found in OTP.
	 * Otherwise the invalid value is set.
	 */
	l1_config_add_all_default_pa_gain_offset();
#endif /* CONFIG_L1_CONFIG_OTP_PA_OFFSET */

	/* Temporary workaround to avoid starting a transaction for each call to
	 * config_manager_set_value that would be called from platform specific reset_to_default
	 * ops.
	 * [TODO UWB-12568]: remove that workaround once we do have the real transactional system.
	 */
	must_commit_transaction = false;

	if (platform_ops.reset_to_default)
		r = platform_ops.reset_to_default();

	must_commit_transaction = true;

#ifndef CONFIG_L1_CONFIG_VOLATILE
	r = l1_config_store_to_persistent_memory(&l1_config);
	if (r)
		return r;
#endif

	return r;
}

/* First version of the L1 Config layout containing the new header format, which include size. */
#define L1_CONFIG_FIRST_VERSION_WITH_NEW_HEADER 0x2
/* NMV controller migth set one's instead of zeros when erasing flhas sector. */
#define L1_CONFIG_INVALID_HEADER 0xFFFFFFFF

STATIC enum qerr l1_config_load(void)
{
	enum qerr r = QERR_SUCCESS;

#ifndef CONFIG_L1_CONFIG_VOLATILE
	struct l1_config *flash_config;
	bool to_reset = false;

	flash_config = (struct l1_config *)l1_config_get_persistent_memory();

	/* Check if config header contains the size. Otherwise, this is a legacy version, so always
	 * reset to default values, because the hash cannot be checked on those versions.
	 */
	if (flash_config->version < L1_CONFIG_FIRST_VERSION_WITH_NEW_HEADER ||
	    flash_config->version == L1_CONFIG_INVALID_HEADER) {
		QLOGE("Old l1 config version %d not supported", flash_config->version);
		to_reset = true;
#ifdef CONFIG_L1_CONFIG_CUSTOM_DEFAULT_USE_DEDICATED_SECTION
	} else if (flash_config->size > CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SIZE) {
		/* Also check validity of the size in the NVM header. Should not exceed max config
		 * size. */
		QLOGE("Invalid header size %d, should be <= %d", flash_config->size,
		      CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SIZE);
		to_reset = true;
#endif
	} else {
		if (flash_config->version != L1_CONFIG_VERSION) {
			/* Note: the processing of inconsistent version will be specified soon.
			 * For now, just print a warning. */
			QLOGW("%s: the version stored in NVME (%d) mismatch with the current one (%d). "
			      "Config should be reset to work properly.",
			      __func__, flash_config->version, L1_CONFIG_VERSION);
		}

		r = l1_config_load_from_persistent_memory(&l1_config);
		if (r != QERR_SUCCESS) {
			QLOGE("%s: load failed", __func__);
			to_reset = true;
		}
	}
	if (to_reset) {
		enum qerr ret;

		QLOGE("Reset all to default");
		/* Use another ret variable to avoid overwriting previous returned state. */
		ret = l1_config_reset_to_default();
		if (ret) {
			QLOGE("Reset failed (%d)", ret);
			return ret;
		}
	}
#else
	/* Always load default values. */
	r = l1_config_reset_to_default();
#endif
	return r;
}

enum qerr l1_config_init(struct l1_config_platform_ops *ops)
{
	/* Register ops, if any. */
	if (ops)
		memcpy(&platform_ops, ops, sizeof(struct l1_config_platform_ops));

	/* Load config from persistent memory is not volatile, or from defaut values otherwise. */
	return l1_config_load();
}

void l1_config_deinit(void)
{
	memset(&platform_ops, 0, sizeof(struct l1_config_platform_ops));
}
