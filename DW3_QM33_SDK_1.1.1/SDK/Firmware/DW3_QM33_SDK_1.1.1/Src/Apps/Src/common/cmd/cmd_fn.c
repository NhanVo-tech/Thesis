/**
 * @file      cmd_fn.c
 *
 * @brief     Collection of executables functions from defined known_commands[]
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "cmd_fn.h"

#include "fira_app_config.h"
#include "reporter.h"
#include "uwb_translate.h"
#include "rtls_version.h"
#include "appConfig.h"
#include "flushTask.h"
#include "deca_dbg.h"
#include "usb_uart_tx.h"
#include "EventManager.h"
#include "thread_fn.h"
#include "driver_app_config.h"
#include "fira_app.h"
#include "qplatform.h"
#include "llhw.h"
#include <stdio.h>
#include "qirq.h"

#define CMD_COLUMN_WIDTH      10
#define CMD_COLUMN_MAX        4
#define MAX_GROUP_COMMENT_LEN 36

#define MAX_XTAL_TRIM_VAL     XTAL_TRIM_BIT_MASK
#define MIN_XTAL_TRIM_VAL     0

#ifdef LISTENER_ENABLED
static const char COMMENT_LISTENER_CFG[] = {
    "LCFG [Option1] [Option2] ...\r\n"
    "Options: (only default values are shown, check the SDK Manual to know more about the available configurations)\r\n"
    "  -CHAN=9       --> Channel selection [5, 9]\r\n"
    "  -PAC=8        --> Preamble Acquisition Chunk size [4 or 8 when RX Preamble Length is <= 128, 16 for 256, 32 when higher]\r\n"
    "  -PCODE=10     --> Preamble Code [9, 10, 11, 12]\r\n"
    "  -SFDTYPE=3    --> Start Frame Delimiter [0 = 4A 8-bit, 1 = DW 8-bit, 2 = DW 16-bit, 3 = 4z 8-bit]\r\n"
    "  -DRATE=6810   --> PSDU Data Rate in kbps [850, 6810]\r\n"
    "  -PHRMODE=0    --> PHR Mode [0 - Standard, 1 - DW Extended]\r\n"
    "  -PHRRATE=0    --> PHR Data Rate [0 - Standard, 1 - PHR at data rate]\r\n"
    "  -STSMODE=0    --> Scrambled Time Sequence Mode [0 - off, 1 - STS mode 1, 2 - STS mode 2, 3 - STS with no data, 8 - STS mode SDC]\r\n"
    "  -STSLEN=0     --> STS Length (power of 2) [32, . ., 2048]\r\n"
    "  -PDOAMODE=1   --> PDoA Mode [0 - off, 1 - PDOA mode 1, 3 - PDOA mode 3]\r\n"
    "  -XTALTRIM=46  --> Crystal oscillator trimming value [0, . ., 63]\r\n"};
#endif
const char CMD_FN_RET_OK[] = "\r\nok\r\n";
const char CMD_FN_RET_KO[] = "\r\nKO\r\n";

const char COMMENT_ANYTIME_OPTIONS[] = {"Anytime commands"};
const char COMMENT_APPSELECTION[] = {"Application selection"};
const char COMMENT_SERVICE[] = {"Service commands"};
const char COMMENT_IDLETIME_OPTIONS[] = {"IDLE time commands"};

extern const app_definition_t idle_app[];
#ifdef LISTENER_ENABLED
extern const app_definition_t helpers_app_listener[];
#endif
extern const app_definition_t helpers_app_fira[];
extern struct l1_config_platform_ops l1_config_platform_ops;
static enum qerr uwb_stack_init(struct uwbmac_context **uwbmac_ctx)
{
    enum qerr r;

    r = qplatform_init();
    if (r != QERR_SUCCESS)
        return r;

    r = l1_config_init(&l1_config_platform_ops);
    if (r != QERR_SUCCESS)
        goto deinit_qplatform;

    r = llhw_init();
    if (r != QERR_SUCCESS)
        goto deinit_l1_config;

    r = uwbmac_init(uwbmac_ctx);

    /* Success. */
    if (r == QERR_SUCCESS)
        goto exit;

    llhw_deinit();
deinit_l1_config:
    l1_config_deinit();
deinit_qplatform:
    qplatform_deinit();
exit:
    return r;
}

static void uwb_stack_deinit(struct uwbmac_context *uwbmac_ctx)
{
    uwbmac_exit(uwbmac_ctx);
    llhw_deinit();
    l1_config_deinit();
    qplatform_deinit();
}

void command_stop_received(void)
{
    const app_definition_t *app_ptr = &idle_app[0];
    EventManagerRegisterApp(&app_ptr);
}

REG_FN(f_stop)
{
    FlushTask_reset();
    reporter_instance.print((char *)"\r\n", 2);
    const app_definition_t *app_ptr = &idle_app[0];
    EventManagerRegisterApp(&app_ptr);
    return (CMD_FN_RET_OK);
}

REG_FN(f_thread)
{
    return thread_fn();
}

REG_FN(f_diag)
{
    uint8_t n = 0;
    int param_val = 0;
    char cmd[9];
    fira_param_t *fira_params = get_fira_config();

    n = sscanf(text, "%9s %d", cmd, &param_val);

    if (n != 2)
    {
        diag_printf("DIAG: %d \r\n", fira_params->session.enable_diagnostics);
    }
    /* Support only 0 and 1 as input parameters. */
    else if (val == 0 || val == 1)
    {
        /* Set diagnostic status. */
        fira_params->session.enable_diagnostics = (bool)val;
        fira_params->session.report_rssi = (bool)val;
    }
    else
    {
        diag_printf("Status value %d is not supported.\r\n", val);
        return CMD_FN_RET_KO;
    }

    return (CMD_FN_RET_OK);
}

REG_FN(f_restore)
{
    unsigned int lock = qirq_lock();
    restore_bssConfig(0);
    qirq_unlock(lock);

    struct uwbmac_context *uwbmac_ctx = NULL;

    enum qerr r = uwb_stack_init(&uwbmac_ctx);
    if (r != QERR_SUCCESS)
        return CMD_FN_RET_KO;

    uwbmac_reset_calibration(uwbmac_ctx);

    uwb_stack_deinit(uwbmac_ctx);

    return CMD_FN_RET_OK;
}


REG_FN(f_decaid)
{
    struct uwbmac_context *uwbmac_ctx = NULL;
    struct uwbmac_device_info device_info;

    enum qerr r = uwb_stack_init(&uwbmac_ctx);
    if (r != QERR_SUCCESS)
        return CMD_FN_RET_KO;

    uwbmac_get_device_info(uwbmac_ctx, &device_info);

    diag_printf("Qorvo Device ID = 0x%08lx\r\n", device_info.dev_id);
    /* PRIx64 does not work properly, so print uint64_t using 2x uint32_t. */
    diag_printf("Qorvo Lot ID = 0x%08lx%08lx\r\n", (uint32_t)(device_info.lot_id >> 32), (uint32_t)device_info.lot_id);
    diag_printf("Qorvo Part ID = 0x%08lx\r\n", device_info.part_id);
    diag_printf("Qorvo SoC ID = %08lx%08lx%08lx\r\n", (uint32_t)(device_info.lot_id >> 32), (uint32_t)device_info.lot_id, device_info.part_id);

    uwb_stack_deinit(uwbmac_ctx);
    return CMD_FN_RET_OK;
}


#ifdef LISTENER_ENABLED
bool scan_listener_cfg(char *text)
{
    int sz;
    char err_msg[128];
    /* Get parameters from global configuration. */
    dwt_app_config_t *dwt_app_config = get_app_dwt_config();
    dwt_config_t *dwt_config = &dwt_app_config->dwt_config;

    /* Check '-' present (modular options). */
    if (strchr(text, '-') != NULL)
    {
        char *pch = strtok(text, " -");
        unsigned int tmp_val = 0;
        uint8_t cpt_arg = 0;

        while (pch != NULL)
        {
            if (sscanf(pch, "CHAN=%d", &tmp_val) == 1)
            {
                if (chan_to_deca(tmp_val) == -1)
                {
                    sz = sprintf(err_msg, "Incorrect Channel: %d\r\n", tmp_val);
                    goto error;
                }
                dwt_config->chan = chan_to_deca(tmp_val);
            }
            else if (sscanf(pch, "PAC=%d", &tmp_val) == 1)
            {
                if (pac_to_deca(tmp_val) == -1)
                {
                    sz = sprintf(err_msg, "Incorrect PAC: %d\r\n", tmp_val);
                    goto error;
                }
                dwt_config->rxPAC = pac_to_deca(tmp_val);
            }
            else if (sscanf(pch, "PCODE=%d", &tmp_val) == 1)
            {
                if (preamble_code_to_deca(tmp_val) == -1)
                {
                    sz = sprintf(err_msg, "Incorrect preamble code: %d\r\n", tmp_val);
                    goto error;
                }
                dwt_config->rxCode = preamble_code_to_deca(tmp_val);
            }
            else if (sscanf(pch, "SFDTYPE=%d", &tmp_val) == 1)
            {
                if (sfd_type_to_deca(tmp_val) == -1)
                {
                    sz = sprintf(err_msg, "Incorrect SFD type: %d\r\n", tmp_val);
                    goto error;
                }
                dwt_config->sfdType = tmp_val;
            }
            else if (sscanf(pch, "DRATE=%d", &tmp_val) == 1)
            {
                if (bitrate_to_deca(tmp_val) == -1)
                {
                    sz = sprintf(err_msg, "Incorrect data rate %d\r\n", tmp_val);
                    goto error;
                }
                dwt_config->dataRate = bitrate_to_deca(tmp_val);
            }
            else if (sscanf(pch, "PHRMODE=%d", &tmp_val) == 1)
            {
                if (phr_mode_to_deca(tmp_val) == -1)
                {
                    sz = sprintf(err_msg, "Incorrect PHR mode: %d\r\n", tmp_val);
                    goto error;
                }
                dwt_config->phrMode = phr_mode_to_deca(tmp_val);
            }
            else if (sscanf(pch, "PHRRATE=%d", &tmp_val) == 1)
            {
                if (phr_rate_to_deca(tmp_val) == -1)
                {
                    sz = sprintf(err_msg, "Incorrect PHR rate: %d\r\n", tmp_val);
                    goto error;
                }
                dwt_config->phrRate = phr_rate_to_deca(tmp_val);
            }
            else if (sscanf(pch, "STSMODE=%d", &tmp_val) == 1)
            {
                if (sts_mode_to_deca(tmp_val) == -1)
                {
                    sz = sprintf(err_msg, "Incorrect STS Mode: %d\r\n", tmp_val);
                    goto error;
                }
                dwt_config->stsMode = sts_mode_to_deca(tmp_val);
            }
            else if (sscanf(pch, "STSLEN=%d", &tmp_val) == 1)
            {
                if (sts_length_to_deca(tmp_val) == -1)
                {
                    sz = sprintf(err_msg, "Wrong STS Length: %d\r\n", tmp_val);
                    goto error;
                }
                dwt_config->stsLength = sts_length_to_deca(tmp_val);
            }
            else if (sscanf(pch, "PDOAMODE=%d", &tmp_val) == 1)
            {
                if (pdoa_mode_to_deca(tmp_val) == -1)
                {
                    sz = sprintf(err_msg, "Incorrect PDOA mode: %d\r\n", tmp_val);
                    goto error;
                }
                dwt_config->pdoaMode = pdoa_mode_to_deca(tmp_val);
            }
            else if (sscanf(pch, "XTALTRIM=%d", &tmp_val) == 1)
            {
                if (tmp_val < MIN_XTAL_TRIM_VAL || tmp_val > MAX_XTAL_TRIM_VAL)
                {
                    sz = sprintf(err_msg, "Incorrect XTALTRIM value: %d, allowed range: [%d, %d]\r\n", tmp_val, MIN_XTAL_TRIM_VAL, MAX_XTAL_TRIM_VAL);
                    goto error;
                }
                dwt_app_config->xtal_trim = (uint8_t)tmp_val;
            }
            else
            {
                if (cpt_arg != 0)
                {
                    sz = sprintf(err_msg, "Unknown argument: %s\r\n", pch);
                    goto error;
                }
            }
            pch = strtok(NULL, " -");
            cpt_arg++;
        }
        return true;
    }
    else
    {
        sz = sprintf(err_msg, "Incorrect command format: \"%s\".\r\nAccepted command format: LCFG -<opt>=VALUE. See help (`HELP LCFG`) for more details\r\n", text);
        goto error;
    }

error:
    reporter_instance.print(err_msg, sz);
    return false;
}

void listener_cfg(void)
{
    int hlen, str_len;
    char *str = qmalloc(MAX_STR_SIZE);
    dwt_app_config_t *dwt_app_config = get_app_dwt_config();
    dwt_config_t *dwt_config = &dwt_app_config->dwt_config;

    str_len = sprintf(str, "JS%04X", 0x5A5A); // reserve space for length of JS object
    hlen = str_len;
    str_len += sprintf(&str[str_len], "{\"LCFG PARAM\":{\r\n");

    str_len += sprintf(&str[str_len], "\"CHAN\":%d,\r\n", deca_to_chan(dwt_config->chan));
    str_len += sprintf(&str[str_len], "\"PAC\":%d,\r\n", deca_to_pac(dwt_config->rxPAC));
    str_len += sprintf(&str[str_len], "\"PCODE\":%d,\r\n", deca_to_preamble_code(dwt_config->rxCode));
    str_len += sprintf(&str[str_len], "\"SFDTYPE\":%d,\r\n", deca_to_sfd_type(dwt_config->sfdType));
    str_len += sprintf(&str[str_len], "\"DRATE\":%d,\r\n", deca_to_bitrate(dwt_config->dataRate));
    str_len += sprintf(&str[str_len], "\"PHRMODE\":%d,\r\n", deca_to_phr_mode(dwt_config->phrMode));
    str_len += sprintf(&str[str_len], "\"PHRRATE\":%d,\r\n", deca_to_phr_rate(dwt_config->phrRate));
    str_len += sprintf(&str[str_len], "\"STSMODE\":%d,\r\n", deca_to_sts_mode(dwt_config->stsMode));
    str_len += sprintf(&str[str_len], "\"STSLEN\":%d,\r\n", deca_to_sts_length(dwt_config->stsLength));
    str_len += sprintf(&str[str_len], "\"PDOAMODE\":%d,\r\n", deca_to_pdoa_mode(dwt_config->pdoaMode));
    str_len += sprintf(&str[str_len], "\"XTALTRIM\":%u}}", dwt_app_config->xtal_trim);

    /* Add formatted 4X of length, this will erase first '{' */
    sprintf(&str[2], "%04X", str_len - hlen);
    /* Restore the start bracket. */
    str[hlen] = '{';
    str_len += sprintf(&str[str_len], "\r\n");
    reporter_instance.print((char *)str, str_len);

    qfree(str);
}

REG_FN(f_listener_cfg)
{
    uint8_t n = 0;
    char cmd[9];
    char arg[32];
    const char *ret;

    /* Check if argument is present. */
    n = sscanf(text, "%9s %32s", cmd, arg);
    if (n == 1)
    {
        ret = CMD_FN_RET_OK;
    }
    else if (n == 2)
    {
        ret = scan_listener_cfg(text) ? CMD_FN_RET_OK : CMD_FN_RET_KO;
    }
    else
    {
        ret = CMD_FN_RET_KO;
    }
    listener_cfg();

    return (ret);
}
#endif /* LISTENER_ENABLED */

REG_FN(f_stat)
{
    const uint16_t max_stat_str = 350;
    const char *ret = NULL;
    const char ver[] = FULL_VERSION;

    char *str = qmalloc(max_stat_str);

    if (str)
    {
        extern const char ProjectName[];
        int str_len, hlen;

        /* Reserve space for length of JS object. */
        str_len = sprintf(str, "JS%04X", 0x5A5A);
        hlen = str_len;

        str_len += sprintf(&str[str_len], "{\"Info\":{\r\n");
        str_len += sprintf(&str[str_len], "\"Device\":\"%s\",\r\n", (char *)ProjectName);
        str_len += sprintf(&str[str_len], "\"Current App\":\"%s\",\r\n", AppGet()->app_name);
        str_len += sprintf(&str[str_len], "\"Version\":\"%s\",\r\n", ver);
        str_len += sprintf(&str[str_len], "\"Build\":\"%s %s\",\r\n", __DATE__, __TIME__);
        str_len += sprintf(&str[str_len], "\"Apps\":[");

        /* Scan for known applications in the __known_command section*/
        extern uint32_t __known_commands_app_start;
        extern uint32_t __known_commands_app_end;

        command_t *knownApp;
        /* Loop and add the application to the output string. */
        for (knownApp = (command_t *)&__known_commands_app_start; knownApp < (command_t *)&__known_commands_app_end; knownApp++)
        {
            str_len += sprintf(&str[str_len], "\"%s\",", knownApp->name);
        }

        /* Last "," character is not needed ; decrease str_len to overwrite. */
        if (str[str_len - 1] == ',')
        {
            str_len--;
        }
        /* End the command bracket. */
        str_len += sprintf(&str[str_len], "],\r\n");
        str_len += sprintf(&str[str_len], "\"Driver\":\"%s\"", dwt_version_string());
        str_len += sprintf(&str[str_len], ",\r\n\"UWB stack\":\"%s\"", uwbmac_get_version());
        str_len += sprintf(&str[str_len], "}}");
        /* Insert formatted 4X of length, this will erase first '{' */
        sprintf(&str[2], "%04X", str_len - hlen);
        /* Restore the start bracket. */
        str[hlen] = '{';

        str_len += sprintf(&str[str_len], "\r\n");
        reporter_instance.print((char *)str, str_len);

        assert(str_len <= max_stat_str);

        qfree(str);

        ret = CMD_FN_RET_OK;
    }

    return (ret);
}

static int print_help_line_group(char *str, int cnt, const char *group)
{
    if (cnt > 0)
    {
        sprintf(&str[cnt], "\r\n");
        port_tx_msg((uint8_t *)str, strlen(str));
        cnt = 0;
    }

    char group_str[] = "---";
    int total_padding = MAX_GROUP_COMMENT_LEN - strlen(group) - 2 * strlen(group_str);
    if (total_padding >= 0)
    {
        /* Print the centralized, padded group name. */
        int left_padding = total_padding / 2;
        int right_padding = total_padding - left_padding;
        sprintf(str, "\r\n%s%*s%s%*s%s\r\n", group_str, left_padding, "", group, right_padding, "", group_str);
    }
    else
    {
        /* If padding does not fit, print raw text of group name. */
        sprintf(str, "%s\r\n", group);
    }

    port_tx_msg((uint8_t *)str, strlen(str));

    return cnt;
}

static int print_help_line(char *str, int cnt, const command_t *known_command)
{
    /* Print appropriate list of parameters for the current application. */
    if (known_command->name)
    {
        sprintf(&str[cnt], "%-10s", known_command->name);
        cnt += CMD_COLUMN_WIDTH;
        if (cnt >= CMD_COLUMN_WIDTH * CMD_COLUMN_MAX)
        {
            sprintf(&str[cnt], "\r\n");
            port_tx_msg((uint8_t *)str, strlen(str));
            cnt = 0;
        }
    }
    return cnt;
}

static int print_app_subcommand_group(char *str, int cnt, const app_definition_t *app)
{
    const struct subcommand_group_s *sub_cmd = app->subcommands;

    if (sub_cmd == NULL)
        return cnt;
    cnt = print_help_line_group(str, cnt, sub_cmd->name);

    const command_t *cmd = sub_cmd->subcommands;
    for (int i = 0; i < sub_cmd->cnt; i++)
    {
        cnt = print_help_line(str, cnt, cmd);
        cmd++;
    }

    return cnt;
}

REG_FN(f_help_std)
{
    int cnt = 0;
    const char *ret = NULL;
    char *str = qmalloc(MAX_STR_SIZE);

    extern uint32_t __known_commands_start;
    extern uint32_t __known_commands_end;
    extern uint32_t __known_commands_app_start;
    extern uint32_t __known_commands_idle_start;
    extern uint32_t __known_commands_service_start;

    known_commands = (command_t *)&__known_commands_start;

    if (str)
    {
        unsigned int lock = qirq_lock();

        extern const char ProjectName[];
        reporter_instance.print((char *)ProjectName, strlen(ProjectName));
        reporter_instance.print("\r\n", 2);

        /* Scan for known applications in the __known_command section. */
        for (known_commands = (command_t *)&__known_commands_start; known_commands < (command_t *)&__known_commands_end; known_commands++)
        {
            uint32_t mode = known_commands->mode;

            if (mode == mANY || mIDLE == AppGet()->app_mode)
            {
                /* Print group delimiters. */
                if (known_commands == (command_t *)&__known_commands_start)
                {
                    cnt = print_help_line_group(str, cnt, COMMENT_ANYTIME_OPTIONS);
                }
                else if (known_commands == (command_t *)&__known_commands_app_start)
                {
                    cnt = print_help_line_group(str, cnt, COMMENT_APPSELECTION);
                }
                else if (known_commands == (command_t *)&__known_commands_idle_start)
                {
                    cnt = print_help_line_group(str, cnt, COMMENT_IDLETIME_OPTIONS);
                }
                else if (known_commands == (command_t *)&__known_commands_service_start)
                {
                    cnt = print_help_line_group(str, cnt, COMMENT_SERVICE);
                }
                /* Print all commands except subcommands. */
                if (mode != mAPP)
                    cnt = print_help_line(str, cnt, known_commands);
            }
        }

        if ((AppGet()->app_mode == mAPP) && (AppGet()->subcommands != NULL))
        {
            /* Print all subcommands of the currently running application. */
            cnt = print_app_subcommand_group(str, cnt, AppGet());
        }
        else if (AppGet()->app_mode == mIDLE)
        {
            /* Print subcommands of all applications. */
            extern uint32_t __known_apps_start;
            extern uint32_t __known_apps_end;
            app_definition_t *knownApp;

            for (knownApp = (app_definition_t *)&__known_apps_start; knownApp < (app_definition_t *)&__known_apps_end; knownApp++)
            {
                cnt = print_app_subcommand_group(str, cnt, knownApp);
            }
        }
        sprintf(&str[cnt], "\r\n");
        reporter_instance.print((char *)str, strlen(str));

        qirq_unlock(lock);

        qfree(str);
        ret = CMD_FN_RET_OK;
    }

    return (ret);
}

REG_FN(f_help_help)
{
    int n = 0;
    const char *ret = CMD_FN_RET_OK;
    char *str = qmalloc(MAX_STR_SIZE);
    extern uint32_t __known_commands_start;
    extern uint32_t __known_commands_end;
    known_commands = (command_t *)&__known_commands_start;

    if (str)
    {
        unsigned int lock = qirq_lock();

        for (known_commands = (command_t *)&__known_commands_start; known_commands < (command_t *)&__known_commands_end; known_commands++)
        {
            uint32_t mode = known_commands->mode;

            if (mode == AppGet()->app_mode || mode == mANY || mIDLE == AppGet()->app_mode)
            {
                if ((known_commands->name != NULL) && (strcmp(known_commands->name, text) == 0))
                {
                    sprintf(str, "\r\n%s:\r\n", text);
                    reporter_instance.print((char *)str, strlen(str));

                    const char *ptr = known_commands->cmnt;

                    bool auto_crlf = (strchr(ptr, '\r') != NULL);
                    if (auto_crlf)
                    {
                        reporter_instance.print((char *)ptr, strlen(ptr));
                    }
                    else
                    {
                        while (ptr)
                        {
                            /* CMD_COLUMN_WIDTH*CMD_COLUMN_MAX. */
                            n = snprintf(str, 78, "%s", ptr);
                            sprintf(&str[strlen(str)], "\r\n");
                            reporter_instance.print((char *)str, strlen(str));
                            if (n < 78)
                            {
                                ptr = NULL;
                            }
                            else
                            {
                                ptr += 77;
                            }
                        }
                    }

                    break;
                }
            }
        }

        /* No command found. */
        if (known_commands >= (command_t *)&__known_commands_end)
        {
            sprintf(str, "\r\nHelp: no help for unknown command.\r\n");
            reporter_instance.print((char *)str, strlen(str));
            ret = CMD_FN_RET_KO;
        }
        qirq_unlock(lock);
        qfree(str);
    }


    return ret;
}

REG_FN(f_help_app)
{
    char help[12];
    char cmd[12];
    int n;
    const char *ret = NULL;

    n = sscanf(text, "%9s %10s", cmd, help);

    switch (n)
    {
        case 1:
            ret = f_help_std(cmd, val, params);
            break;
        case 2:
            if (help[0] == 0)
            {
                ret = f_help_help(&help[1], val, params);
            }
            else
            {
                ret = f_help_help(&help[0], val, params);
            }
            break;
        default:
            break;
    }

    return (ret);
}

REG_FN(f_save)
{
    fira_save_params();
    unsigned int lock = qirq_lock();
    save_bssConfig();
    qirq_unlock(lock);
    return (CMD_FN_RET_OK);
}

REG_FN(f_setapp)
{
    dwt_app_config_t *app_dwt = get_app_dwt_config();
    fira_param_t *fira_params = get_fira_config();
    char cmd[10];
    char app[10];
    int n = sscanf(text, "%6s %s", cmd, app);

    if (n != 2)
    {
        const char *name = app_dwt->app ? app_dwt->app->app_name : "NONE";
        diag_printf("SETAPP: %s \r\n", name);
        return (CMD_FN_RET_OK);
    }
    if (!strcmp(app, "INITF"))
    {
        app_dwt->app = &helpers_app_fira[FIRA_APP_INITF];
        fira_params->app_type = FIRA_APP_INITF;
    }
    else if (!strcmp(app, "RESPF"))
    {
        app_dwt->app = &helpers_app_fira[FIRA_APP_RESPF];
        fira_params->app_type = FIRA_APP_RESPF;
    }
#ifdef LISTENER_ENABLED
    else if (!strcmp(app, "LISTENER"))
    {
        app_dwt->app = helpers_app_listener;
    }
#endif
    else if (!strcmp(app, "NONE"))
    {
        app_dwt->app = idle_app;
        fira_params->app_type = FIRA_APP_NONE;
    }
    else
    {
        diag_printf("SETAPP: INVALID APPLICATION \r\n");
        return (CMD_FN_RET_KO);
    }
    return (CMD_FN_RET_OK);
}

const char STD_CMD_COMMENT[] = {"This command is described in the documentation"};
const char COMMENT_STOP[] = {"Stops running any top-level application"};
const char COMMENT_STAT[] = {"Displays the Status information"};
const char COMMENT_SAVE[] = {"Saves the configuration and the default app to the NVM, the SAVE command cannot be used during a ranging session"};
const char COMMENT_HELP[] = {"Displays the help information.\r\nUsage: \"HELP\" or \"HELP <CMD>\", <CMD> is the command from the list, e.g. \"HELP SAVE\"."};
const char COMMENT_RESTORE[] = {"Restores the default configuration, both UWB and System."};
const char COMMENT_DIAG[] = {"Enables diagnostic mode to display complementary information during ranging.\r\nUsage: \"DIAG <DEC>\" (0:OFF, 1:ON)"};
const char COMMENT_THREAD[] = {"Displays maximum stack usage per thread and total heap usage"};
const char COMMENT_DECAID[] = {"Displays UWB chip information"};
const char COMMENT_SETAPP[] = {"Sets the default application to be run after power cycle. Possible arguments: INITF, RESPF, LISTENER, NONE."};

command_t *known_commands;

const struct command_s known_commands_anytime_all[] __attribute__((section(".known_commands_anytime"))) = {
    {"HELP", mANY, f_help_app, COMMENT_HELP},
    {"?", mANY, f_help_app, COMMENT_HELP},
    {"STOP", mANY, f_stop, COMMENT_STOP},
    {"THREAD", mANY, f_thread, COMMENT_THREAD},
    {"STAT", mANY, f_stat, COMMENT_STAT},
};

const struct command_s known_commands_service_all[] __attribute__((section(".known_commands_service"))) = {
    {"RESTORE", mIDLE, f_restore, COMMENT_RESTORE},
    {"DIAG", mIDLE, f_diag, COMMENT_DIAG},
#ifdef LISTENER_ENABLED
    {"LCFG", mIDLE, f_listener_cfg, COMMENT_LISTENER_CFG},
#endif
    {"DECAID", mIDLE, f_decaid, COMMENT_DECAID},
    {"SAVE", mIDLE, f_save, COMMENT_SAVE},
    {"SETAPP", mIDLE, f_setapp, COMMENT_SETAPP},
};
