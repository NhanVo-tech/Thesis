/**
 * @file      app.h
 *
 * @brief     Interface for applications
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "deca_error.h"
#include "circular_buffer.h"

/* System mode of operation. Used to indicate in which mode of operation system is running. */
typedef enum
{
    mANY = 0,  /* Used only for Commands: indicates the command can be executed in any modes below. */
    mIDLE = 1, /* Used only in IDLE state (no application is running). */
    mAPP = 2,  /* Used for application's subcommand (when application is running). */
} mode_e;

typedef enum
{
    NO_DATA = 0,
    DATA_READY,
    COMMAND_READY,
    DATA_SEND,
    DATA_FLUSH,
    DATA_STOP,
    DATA_SAVE,
    DATA_ERROR
} usb_data_e;

struct command_s;

struct subcommand_group_s
{
    const char *name;                    /* Text printed above group of subcommands in CLI help. */
    const struct command_s *subcommands; /* Array of subcommands. */
    const uint8_t cnt;                   /* Number of subcommands in group. */
};

struct app_definition_s
{
    char *app_name;
    mode_e app_mode;
    void (*helper)(void const *argument);
    void (*terminate)(void);
    usb_data_e (*on_rx)(struct cc_buff *buf);
    void (*command_parser)(usb_data_e res, void *text);
    const struct subcommand_group_s *subcommands;
};
typedef struct app_definition_s app_definition_t;
extern app_definition_t *known_apps;

const app_definition_t *AppGet(void);
void AppSet(const app_definition_t *app);
const app_definition_t *AppGetDefaultEvent(void);
error_e AppSetDefaultEvent(const app_definition_t *app);
void AppConfigInit(void);
void app_apptimer_stop(void);
