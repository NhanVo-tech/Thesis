/**
 * @file      cmd.c
 *
 * @brief     Command string parser
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "cmd.h"

#include <ctype.h>

#include "cJSON.h"
#include "cmd_fn.h"
#include "reporter.h"
#include "qmalloc.h"
#include <stdio.h>

/**
 * @brief Print error if parsing failed or command execution returned error.
 */
static void cmd_onERROR(const char *err)
{
    char *str = qmalloc(MAX_STR_SIZE);

    if (str)
    {
        strcpy(str, "\r\nerror \r\n");
        if (strlen(err) < (MAX_STR_SIZE - 7 - 3 - 1))
        {
            strcpy(&str[7], err);
            strcpy(&str[7 + strlen(err)], "\r\n");
        }
        reporter_instance.print((char *)str, strlen(str));

        qfree(str);
    }
}

/**
 * @brief checks if input "text" string in known "COMMAND" or "PARAMETER VALUE" format,
 * checks their execution permissions, a VALUE range if restrictions and
 * executes COMMAND or sets the PARAMETER to the VALUE.
 */
void command_parser(usb_data_e res, void *data)
{
    char *text = (char *)data;
    char *temp_str = text;
    command_e equal;
    int val;
    cJSON *json_root, *json_params;
    char cmd[20];
    const char *ret;
    uint8_t data_len = strlen(data);

    extern uint32_t __known_commands_start;
    extern uint32_t __known_commands_end;

    if (res != COMMAND_READY)
        return;

    known_commands = (command_t *)&__known_commands_start;

    while (*temp_str)
    {
        *temp_str = (char)toupper(*temp_str);
        temp_str++;
    }


    equal = _NO_COMMAND;
    json_params = NULL;
    json_root = NULL;
    cmd[0] = 0;

    if (*text == '{')
    {
        /* Probably a Json command. */
        json_root = cJSON_Parse(text);
        if (json_root != NULL)
        {
            /* Got valid Json command, get it's name. */
            temp_str = cJSON_GetObjectItem(json_root, CMD_NAME)->valuestring;
            if (temp_str != NULL)
            { /* Got right command name, get command params. */
                json_params = cJSON_GetObjectItem(json_root, CMD_PARAMS);
                if (json_params != NULL)
                {
                    /* We have a Json so we need to update command. */
                    sscanf(temp_str, "%" STR(MAX_CMD_LEN) "s", cmd);
                }
            }
        }
    }
    else
    {
        /* It is not a Json command. */
        sscanf(text, "%" STR(MAX_CMD_LEN) "s %d", cmd, &val);
    }

    /* Scan for known applications in the __known_command section. */
    for (known_commands = (command_t *)&__known_commands_start; known_commands < (command_t *)&__known_commands_end; known_commands++)
    {
        if (known_commands->name && strcmp(cmd, known_commands->name) == 0)
        {
            equal = _COMMAND_FOUND;

            /* Check the command mode to define the execution permission. */
            switch (known_commands->mode)
            {
                /* If it is an anytime command then launch it. */
                case mANY:
                    equal = _COMMAND_ALLOWED;
                    break;
                /* If it is an app then check the current running app mode. */
                case mIDLE:
                    if (known_commands->mode == AppGet()->app_mode)
                    {
                        equal = _COMMAND_ALLOWED;
                    }
                    break;
                /* If it is a subcommand then check is done later. */
                case mAPP:
                    break;

                default:
                    break;
            }
            /* At this stage the command is not allowed to execute
             * Check if the command is a sub command of the running application. */
            const struct subcommand_group_s *sub_cmd = AppGet()->subcommands;
            if (sub_cmd != NULL)
            {
                const command_t *cmd = sub_cmd->subcommands;
                for (int i = 0; i < sub_cmd->cnt; i++)
                {
                    if (cmd == known_commands)
                    {
                        /* The command is a subcommand of the running app. */
                        equal = _COMMAND_ALLOWED;
                        break;
                    }
                    cmd++;
                }
            }
            break;
        }
    }

    switch (equal)
    {
        case (_COMMAND_FOUND):
        {
            cmd_onERROR(" incompatible mode");
            break;
        }
        case (_NO_COMMAND):
        {
            const uint8_t max_msg_len = 50;
            char error_message[max_msg_len];
            snprintf(error_message, max_msg_len, " unknown command: \"%s\"", cmd);
            cmd_onERROR(error_message);
            break;
        }
        case (_COMMAND_ALLOWED):
        {
            /* Execute corresponding fn(). */
            ret = known_commands->fn(text, val, json_params);

            if (ret)
            {
                reporter_instance.print((char *)ret, strlen(ret));
            }
            else
            {
                cmd_onERROR(" function");
            }
            break;
        }
        default:
            break;
    }
    if (json_root != NULL)
    {
        cJSON_Delete(json_root);
    }
    memset(data, 0, data_len);
}
