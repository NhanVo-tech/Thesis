/**
 * @file      cmd_fn.h
 *
 * @brief     Header file for macros, structures and prototypes cmd_fn.c
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "app.h"
#include "cJSON.h"

/* Module definitions. */
#define MAX_STR_SIZE 300

/*
 * All cmd_fn functions have unified input: (char *text, int val, cJSON *params)).
 *
 * REG_FN(fun) macro will create a function:
 *   const char *fun(char *text, int val, cJSON *params)
 */
#define REG_FN(x) const char *x(char *text, int val, cJSON *params)

/* Command table structure definition. */
struct command_s
{
    const char *name;  /* Command name string. */
    const mode_e mode; /* System mode of operation. */
    REG_FN((*fn));     /* Function to serve the command. */
    const char *cmnt;  /* Help string. */
};

typedef struct command_s command_t;
extern command_t *known_commands;
extern const char CMD_FN_RET_OK[];
extern const char CMD_FN_RET_KO[];
extern const char COMMENT_VERSION[];

void command_stop_received(void);

#ifdef __cplusplus
}
#endif
