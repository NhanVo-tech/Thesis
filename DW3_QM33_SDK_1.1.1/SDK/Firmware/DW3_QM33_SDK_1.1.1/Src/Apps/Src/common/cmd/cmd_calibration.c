/**
 * @file      cmd_calibration.c
 *
 * @brief     Command to handle calibration
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>

#include "cmd_fn.h"
#include "deca_dbg.h"
#include "qplatform.h"
#include "l1_config.h"
#include "qmalloc.h"

#define L1_CONFIG_VAL_SIZE_MAX 128

const char COMMENT_CALKEY[] = {"Set or get a calibration Key. Usage: \r\n"
                               "To set a value: \"CALKEY <key> <value>\"\r\n"
                               "To get a value: \"CALKEY <key>\"\r\n"};
const char COMMENT_LISTCAL[] = {"List all available calibration keys\r\n"};

static int16_t float_to_q11(float f)
{
#define FIXED_POINT_POS 11
    int16_t fixed;
    if (f >= 0)
    {
        fixed = f * (1 << FIXED_POINT_POS);
    }
    else
    {
        fixed = (1 << 16) + (-f * (1 << FIXED_POINT_POS));
    }
    return fixed;
}

REG_FN(f_calibration_key)
{
    const char *ret = CMD_FN_RET_OK;
    char key[L1_CONFIG_KEY_NAME_MAX_LEN + 1] = {0};
    int value_len = 0;
    int value_int = 0;
    uint64_t value_uint64 = 0;
    int16_t *LUT = NULL;
    void *value;
    uint8_t value_raw[L1_CONFIG_VAL_SIZE_MAX] = {0};

    /* We can do that because the mac is not yet initialized and the calib not loaded. */
    if (qplatform_init())
        return CMD_FN_RET_KO;
    if (l1_config_init(NULL))
        return CMD_FN_RET_KO;

    /* Command looks like "CALKEY <key> <value>". */
    /* Get the Command and drop it. */
    char *pch = strtok(text, " ");
    /* Get the Key and put it lowercase. */
    pch = strtok(NULL, " ");
    strncpy(key, pch, L1_CONFIG_KEY_NAME_MAX_LEN);
    char *temp_str = key;
    while (*temp_str)
    {
        *temp_str = (char)tolower(*temp_str);
        temp_str++;
    }

    /* Check that the key exists in database and get the length of the value. */
    value_len = l1_config_read_key(key, value_raw, L1_CONFIG_VAL_SIZE_MAX);
    if (value_len < 0)
    {
        diag_printf("\r\nPlease enter a valid key: %s\r\n", key);
        ret = CMD_FN_RET_KO;
        goto end;
    }
    /* Get the value. */
    pch = strtok(NULL, " ");
    if (pch == NULL)
    {
        /* Did not receive any value, print the current one. */
        diag_printf("%s: 0x", key);
        for (int i = value_len - 1; i >= 0; i--)
        {
            diag_printf("%02x", value_raw[i]);
        }
        diag_printf(" (len: %d)\r\n", value_len);
        ret = CMD_FN_RET_OK;
        goto end;
    }

    if (value_len <= 4)
    {
        if (value_len <= 2)
        {
            /* Some of the calibration keys which fit into 8 or 16 bits can be negative. */
            value_int = strtol(pch, NULL, 0);
        }
        else
        {
            /* All the calibration keys which fit into 32 bits are positive,
             * so we need to convert them to unsigned long. */
            value_int = strtoul(pch, NULL, 0);
        }
        value = &value_int;
    }
    else if (value_len <= 8)
    {
        /* Value Len == 6, for pdoa segments. Possible to store them in a uint64_t. */
        value_uint64 = strtoull(pch, NULL, 0);
        value = &value_uint64;
    }
    else
    {
        /* Value len == 124 bytes, AoA LUTs. The LUT size is 31x s16 pairs.
         * It is provided in CLI as: calkey pdoa_lut0.data <pdoa0> <aoa0> ... <pdoa31> <aoa31>. */
        LUT = qmalloc(value_len);
        if (LUT == NULL)
        {
            diag_printf("\r\n%s: Cannot set LUT - Not enough memory -\r\n", key);
            ret = CMD_FN_RET_KO;
            goto end;
        }
        int i = 0;
        while (i < value_len / 2 && pch != NULL)
        {
            int16_t tmp;
            if (pch[0] == '0' && (pch[1] == 'x' || pch[1] == 'X'))
            {
                /* Hex value. The user provided fixed point data. */
                tmp = (int16_t)strtol(pch, NULL, 0);
            }
            else
            {
                /* Float value. The user provided floating value to be converted. */
                float f = strtof(pch, NULL);
                tmp = float_to_q11(f);
            }
            LUT[i] = tmp;
            pch = strtok(NULL, " ");
            i++;
        }

        if (i != value_len / 2)
        {
            /* Did not received the full LUT, do not process it. */
            diag_printf("\r\n%s error: Expected number of elements: %d, received: %d elements\r\n", key, value_len, i);
            ret = CMD_FN_RET_KO;
            goto end;
        }
        diag_printf("\r\n%s: LUT correctly parsed from CLI\r\n", key);
        value = LUT;
    }

    enum qerr r = l1_config_store_key(key, value, value_len);
    if (r != QERR_SUCCESS)
    {
        diag_printf("\r\nParameter value rejected: %d\r\n", *((int *)value));
        ret = CMD_FN_RET_KO;
    }
    else
    {
        diag_printf("%s: 0x", key);
        for (int i = value_len - 1; i >= 0; i--)
        {
            diag_printf("%02x", ((uint8_t *)value)[i]);
        }
        diag_printf(" (len: %d)\r\n", value_len);
        ret = CMD_FN_RET_OK;
    }

end:
    l1_config_deinit();
    qplatform_deinit();
    if (LUT)
        qfree(LUT);
    return ret;
}

REG_FN(f_list_calibration)
{
    char key[L1_CONFIG_KEY_NAME_MAX_LEN];
    uint16_t i = 0;
    int value_len = 0;
    uint8_t value_raw[L1_CONFIG_VAL_SIZE_MAX] = {0};

    if (qplatform_init())
        return CMD_FN_RET_KO;
    if (l1_config_init(NULL))
        return CMD_FN_RET_KO;

    while (l1_config_get_key_name(i, key) == QERR_SUCCESS)
    {
        diag_printf("%s: 0x", key);
        value_len = l1_config_read_key(key, value_raw, L1_CONFIG_VAL_SIZE_MAX);
        for (int i = value_len - 1; i >= 0; i--)
        {
            diag_printf("%02x", value_raw[i]);
        }
        diag_printf(" (len: %d)\r\n", value_len);

        i++;
    }

    l1_config_deinit();
    qplatform_deinit();
    return CMD_FN_RET_OK;
}

const struct command_s known_commands_set_calibration[] __attribute__((section(".known_commands_idle"))) = {
    {"CALKEY", mIDLE, f_calibration_key, COMMENT_CALKEY},
    {"LISTCAL", mIDLE, f_list_calibration, COMMENT_LISTCAL}};
