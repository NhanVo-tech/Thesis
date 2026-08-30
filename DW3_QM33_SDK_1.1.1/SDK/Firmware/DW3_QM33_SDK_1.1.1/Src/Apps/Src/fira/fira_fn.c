/**
 * @file      fira_fn.c
 *
 * @brief     Executables for FiRa commands
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "cmd_fn.h"
#include "app.h"
#include "common_fira.h"
#include "EventManager.h"
#include "fira_app.h"

#define INITF_OFFSET 0
#define RESPF_OFFSET 1

static const char INITF_CMD_COMMENT[] = {
    "INITF [Option1] [Option2] ...\r\n"
    "Options: (only default values are shown, check the SDK Manual to know more about the available configurations)\r\n"
    "  -CHAN=9       --> Channel number\r\n"
    "  -PRFSET=BPRF4 --> PRF set\r\n"
    "  -PCODE=10     --> Preamble code index\r\n"
    "  -SLOT=2400    --> Slot duration [RSTU]\r\n"
    "  -BLOCK=200    --> Block duration [ms]\r\n"
    "  -ROUND=25     --> Round duration [slots]\r\n"
    "  -RRU=DSTWR    --> Ranging round usage\r\n"
    "  -ID=42        --> Session ID\r\n"
    "  -VUPPER=01:02:03:04:05:06:07:08   --> vUpper64\r\n"
    "  -MULTI        --> Activate one-to-many mode\r\n"
    "  -HOP          --> Activate round hopping\r\n"
    "  -ADDR=0       --> Device own address (Initiator address)\r\n"
    "  -PADDR=1      --> 1st Responder address\r\n"
    "     or to set multiple responders: \r\n"
    "  -PADDR=[1,2,.,.,n]  --> to set n Responder addresses (for one-to-many)\r\n"};
static const char RESPF_CMD_COMMENT[] = {
    "RESPF [Option1] [Option2] ...\r\n"
    "Options: (only default values are shown, check the SDK Manual to know more about the available configurations)\r\n"
    "  -CHAN=9       --> Channel number\r\n"
    "  -PRFSET=BPRF4 --> PRF set\r\n"
    "  -PCODE=10     --> Preamble code index\r\n"
    "  -SLOT=2400    --> Slot duration [RSTU]\r\n"
    "  -BLOCK=200    --> Block duration [ms]\r\n"
    "  -ROUND=25     --> Round duration [slots]\r\n"
    "  -RRU=DSTWR    --> Ranging round usage\r\n"
    "  -ID=42        --> Session ID\r\n"
    "  -VUPPER=01:02:03:04:05:06:07:08   --> vUpper64\r\n"
    "  -MULTI        --> Activate one-to-many mode\r\n"
    "  -HOP          --> Activate round hopping\r\n"
    "  -ADDR=1       --> Device own address (Responder address)\r\n"
    "  -PADDR=0      --> Peer address (Initiator address)\r\n"};

extern const app_definition_t helpers_app_fira[];

/* FiRa Two-Way Ranging Initiator and Responder. */
REG_FN(f_initiator_f)
{
    bool is_controller = true;
    const char *ret = CMD_FN_RET_KO;

    if (fira_set_user_params(text, is_controller))
    {
        fira_show_params();

        const app_definition_t *app_ptr = &helpers_app_fira[INITF_OFFSET];
        EventManagerRegisterApp(&app_ptr);

        ret = CMD_FN_RET_OK;
    }

    return (ret);
}

REG_FN(f_responder_f)
{
    bool is_controller = false;
    const char *ret = CMD_FN_RET_KO;

    if (fira_set_user_params(text, is_controller))
    {
        fira_show_params();

        const app_definition_t *app_ptr = &helpers_app_fira[RESPF_OFFSET];
        EventManagerRegisterApp(&app_ptr);

        ret = CMD_FN_RET_OK;
    }

    return (ret);
}

const struct command_s known_app_fira[] __attribute__((
    section(".known_commands_app")
))
= {
    {"RESPF", mIDLE, f_responder_f, RESPF_CMD_COMMENT},
    {"INITF", mIDLE, f_initiator_f, INITF_CMD_COMMENT},
};
