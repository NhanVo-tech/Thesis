/**
 * @file      fira_app.c
 *
 * @brief     Fira processes control
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "fira_app.h"

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#include "fira_app_config.h"
#include "fira_default_params.h"
#include "reporter.h"
#include "uwbmac/uwbmac.h"
#include "uwbmac/fira_helper.h"
#include "common_fira.h"
#include "task_signal.h"
#include "HAL_error.h"
#include "cmd.h"
#include "driver_app_config.h"
#include "qmalloc.h"
#include "uwb_convert.h"
#include "uwb_translate.h"


static struct uwbmac_context *uwbmac_ctx = NULL;
static struct fira_context fira_ctx;

#define STR_SIZE (256)
#define TEN_TO_6 1000000

static uint32_t session_id = 42;
static uint32_t session_handle = 0;
static bool started = false;
static void fira_session_info_ntf_twr_cb(const struct fira_twr_ranging_results *results, void *user_data);
static void fira_range_diagnostics_ntf_cb(const struct fira_ranging_info *info, void *user_data);
static void fira_session_status_ntf_cb(const struct fira_session_status_ntf_content *status, void *user_data);
static void fira_helper_ntfs_cb(enum fira_helper_cb_type cb_type, const void *content, void *user_data);
static struct string_measurement output_result;

void fira_set_default_params(bool controller)
{
    fira_param_t *fira_params = get_fira_config();
    struct session_parameters *session_params = &fira_params->session;

    if (controller)
    {
        session_params->n_destination_short_address = 0;
        session_params->short_addr = FIRA_DEFAULT_CONTROLLER_SHORT_ADDR;
        session_params->device_type = QUWBS_FBS_DEVICE_TYPE_CONTROLLER;
        session_params->device_role = QUWBS_FBS_DEVICE_ROLE_INITIATOR;
        fira_params->controlees_params.n_controlees = FIRA_DEFAULT_NUM_OF_CONTROLEES;
        fira_params->controlees_params.controlees[0].address = FIRA_DEFAULT_CONTROLLEE_SHORT_ADDR;
    }
    else
    {
        session_params->n_destination_short_address = 1;
        session_params->destination_short_address[0] = FIRA_DEFAULT_CONTROLLER_SHORT_ADDR;
        session_params->short_addr = FIRA_DEFAULT_CONTROLLEE_SHORT_ADDR;
        session_params->device_type = QUWBS_FBS_DEVICE_TYPE_CONTROLEE;
        session_params->device_role = QUWBS_FBS_DEVICE_ROLE_RESPONDER;
        fira_params->controlees_params.n_controlees = 0;
    }

    fira_params->session_id = FIRA_DEFAULT_SESSION_ID;
    fira_params->config_state = FIRA_APP_CONFIG_DEFAULT;
    session_params->rframe_config = FIRA_DEFAULT_RFRAME_CONFIG;
    session_params->sfd_id = FIRA_DEFAULT_SFD_ID;
    session_params->slot_duration_rstu = FIRA_DEFAULT_SLOT_DURATION_RSTU;
    session_params->block_duration_ms = FIRA_DEFAULT_BLOCK_DURATION_MS;
    session_params->round_duration_slots = FIRA_DEFAULT_ROUND_DURATION_SLOTS;
    session_params->ranging_round_usage = FIRA_DEFAULT_RANGING_ROUND_USAGE;
    session_params->multi_node_mode = FIRA_DEFAULT_MULTI_NODE_MODE;
    session_params->round_hopping = FIRA_DEFAULT_ROUND_HOPPING;
    session_params->schedule_mode = FIRA_SCHEDULE_MODE_TIME_SCHEDULED;

    /* FiRa PHY default config. */
    session_params->preamble_code_index = FIRA_DEFAULT_PREAMBLE_CODE_INDEX;
    /* Default: standard PHR rate (0). */
    session_params->phr_data_rate = FIRA_PRF_MODE_BPRF;
    /* Default: 9. */
    session_params->channel_number = FIRA_DEFAULT_CHANNEL_NUMBER;
    /* Bit 0 is for setting tof report. */
    session_params->result_report_config |= fira_helper_bool_to_result_report_config(true, false, false, false);
    /* Request measurement report phase. */
    session_params->ranging_round_control |= fira_helper_bool_to_ranging_round_control(true, false);

    uint8_t *v = session_params->vupper64;
    for (int i = 0; i < FIRA_VUPPER64_SIZE; i++)
        v[i] = i + 1;
}

bool fira_set_user_params(char *text, bool controller)
{
    const fira_app_type_t new_app_type = controller ? FIRA_APP_INITF : FIRA_APP_RESPF;
    fira_param_t *fira_param = get_fira_config();

    /* User provided parameters. */
    if (text && strchr(text, '-') != NULL)
    {
        fira_set_default_params(controller);
        if (!fira_scan_params(text, controller))
            return false;
        fira_param->config_state = FIRA_APP_CONFIG_USER;
    }
    /* User not provided parameters but config was not saved or application has changed. */
    else if ((fira_param->config_state != FIRA_APP_CONFIG_SAVED) || (fira_param->app_type != new_app_type))
    {
        fira_set_default_params(controller);
    }
    fira_param->app_type = new_app_type;

    return true;
}

bool fira_scan_params(char *text, bool controller)
{
    fira_param_t *fira_param = get_fira_config();
    char *pch = strtok(text, " -");
    unsigned int tmp_val = 0;
    char tmp_str[30] = "";
    int sz;
    char err_msg[128];
    uint8_t cpt_arg = 0;
    int param[8];

    while (pch != NULL)
    {
        if (sscanf(pch, "CHAN=%d", &tmp_val) == 1)
        {
            if (chan_to_deca(tmp_val) != -1)
            {
                fira_param->session.channel_number = chan_to_deca(tmp_val);
            }
            else
            {
                sz = sprintf(err_msg, "Incorrect Channel: %d\r\n", tmp_val);
                reporter_instance.print(err_msg, sz);
                return false;
            }
        }
        else if (sscanf(pch, "PRFSET=%cPRF%d", &tmp_str[0], &tmp_val) == 2)
        {
            /* Only BPRF supporetd. */
            if (tmp_str[0] == 'B')
            {
                fira_param->session.prf_mode = FIRA_PRF_MODE_BPRF;
                switch (tmp_val)
                {
                    case 3: /* SP1, SFD 2. */
                        fira_param->session.rframe_config = FIRA_RFRAME_CONFIG_SP1;
                        fira_param->session.sfd_id = FIRA_SFD_ID_2;
                        break;
                    case 4: /* SP3, SFD 2. */
                        fira_param->session.rframe_config = FIRA_RFRAME_CONFIG_SP3;
                        fira_param->session.sfd_id = FIRA_SFD_ID_2;
                        break;
                    case 5: /* SP1, SFD 0. */
                        fira_param->session.rframe_config = FIRA_RFRAME_CONFIG_SP1;
                        fira_param->session.sfd_id = FIRA_SFD_ID_0;
                        break;
                    case 6: /* SP3, SFD 0. */
                        fira_param->session.rframe_config = FIRA_RFRAME_CONFIG_SP3;
                        fira_param->session.sfd_id = FIRA_SFD_ID_0;
                        break;
                    default:
                        sz = sprintf(err_msg, "Incorrect PRF Set\r\n");
                        reporter_instance.print(err_msg, sz);
                        return false;
                }
            }
            else
            {
                sz = sprintf(err_msg, "Incorrect PRF\r\n");
                reporter_instance.print(err_msg, sz);
                return false;
            }
        }
        else if (sscanf(pch, "SLOT=%d", &tmp_val) == 1)
        {
            /* Slot duration (RSTU). */
            if (tmp_val >= 2400)
            {
                fira_param->session.slot_duration_rstu = tmp_val;
            }
            else
            {
                sz = sprintf(err_msg, "Incorrect Slot Duration (RSTU)\r\n");
                reporter_instance.print(err_msg, sz);
                return false;
            }
        }
        else if (sscanf(pch, "BLOCK=%d", &tmp_val) == 1)
        {
            /* Block duration (ms). */
            if (tmp_val >= 1)
            {
                fira_param->session.block_duration_ms = tmp_val;
            }
            else
            {
                sz = sprintf(err_msg, "Incorrect Block Duration (ms)\r\n");
                reporter_instance.print(err_msg, sz);
                return false;
            }
        }
        else if (sscanf(pch, "ROUND=%d", &tmp_val) == 1)
        {
            /* Round duration (slots). */
            if (tmp_val >= 1 && tmp_val <= 255)
            {
                fira_param->session.round_duration_slots = tmp_val;
            }
            else
            {
                sz = sprintf(err_msg, "Incorrect Round Duration (slots)\r\n");
                reporter_instance.print(err_msg, sz);
                return false;
            }
        }
        else if (sscanf(pch, "RRU=%s", tmp_str) == 1)
        {
            /* Ranging Round Usage. */
            if (!strcmp(tmp_str, "SSTWR"))
            {
                fira_param->session.ranging_round_usage = FIRA_RANGING_ROUND_USAGE_SSTWR_DEFERRED;
            }
            else if (!strcmp(tmp_str, "DSTWR"))
            {
                fira_param->session.ranging_round_usage = FIRA_RANGING_ROUND_USAGE_DSTWR_DEFERRED;
            }
            else if (!strcmp(tmp_str, "SSTWRNDEF"))
            {
                fira_param->session.ranging_round_usage = FIRA_RANGING_ROUND_USAGE_SSTWR_NON_DEFERRED;
            }
            else if (!strcmp(tmp_str, "DSTWRNDEF"))
            {
                fira_param->session.ranging_round_usage = FIRA_RANGING_ROUND_USAGE_DSTWR_NON_DEFERRED;
            }
            else
            {
                sz = sprintf(err_msg, "Incorrect Ranging Round Usage\r\n");
                reporter_instance.print(err_msg, sz);
                return false;
            }
        }
        else if (sscanf(pch, "ID=%d", &tmp_val) == 1)
        {
            /* Session ID. */
            if (tmp_val != 0)
            {
                fira_param->session_id = tmp_val;
            }
            else
            {
                sz = sprintf(err_msg, "Incorrect Session ID\r\n");
                reporter_instance.print(err_msg, sz);
                return false;
            }
        }
        else if (sscanf(pch, "VUPPER=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", &param[0], &param[1], &param[2], &param[3], &param[4], &param[5], &param[6], &param[7]) == 8)
        {
            /* vUpper64. */
            uint8_t *v = fira_param->session.vupper64;
            for (int i = 0; i < FIRA_VUPPER64_SIZE; i++)
                v[i] = param[i];
        }
        else if (!strcmp(pch, "MULTI"))
        {
            /* Multi Node Mode. */
            fira_param->session.multi_node_mode = FIRA_MULTI_NODE_MODE_ONE_TO_MANY;
        }
        else if (!strcmp(pch, "HOP"))
        {
            /* Round hopping. */
            fira_param->session.round_hopping = true;
        }
        else if (sscanf(pch, "ADDR=%d", &tmp_val) == 1)
        {
            /* Own device address. */
            fira_param->session.short_addr = tmp_val;
        }
        else if (sscanf(pch, "PADDR=%s", tmp_str) == 1)
        {
            /* Peer address. */

            fira_param->controlees_params.n_controlees = 0;
            /* Single address. */
            if (tmp_str[0] >= '0' && tmp_str[0] <= '9')
            {
                if (controller)
                    fira_param->controlees_params.controlees[fira_param->controlees_params.n_controlees++].address = atoi(tmp_str);
                else
                    fira_param->session.destination_short_address[0] = atoi(tmp_str);
            }
            /* Address list, only applicable to controller. */
            else if (controller)
            {
                const char *filter = "[],";
                char *peer = tmp_str;
                char *end;

                fira_param->controlees_params.n_controlees = 0;
                while (peer != NULL && *peer != '\0')
                {
                    end = strpbrk(peer, filter); // Find next delimiter
                    if (end != NULL)
                    {
                        *end = '\0'; // Null-terminate the current token
                    }

                    if (*peer != '\0') // Ensure there's a valid token
                    {
                        if (fira_param->controlees_params.n_controlees < FIRA_RESPONDERS_MAX)
                        {
                            fira_param->controlees_params.controlees[fira_param->controlees_params.n_controlees++].address = atoi(peer);
                        }
                        else
                        {
                            sz = sprintf(err_msg, "Number of peers exceeds the max: (%d) \r\n", FIRA_RESPONDERS_MAX);
                            reporter_instance.print(err_msg, sz);
                            return false;
                        }
                    }

                    peer = (end != NULL) ? end + 1 : NULL; // Move to the next token
                }
            }
            else
            {
                sz = sprintf(err_msg, "Not a valid address given: %s \r\n", tmp_str);
                reporter_instance.print(err_msg, sz);
                return false;
            }
        }
        else if (sscanf(pch, "PCODE=%d", &tmp_val) == 1)
        {
            if (preamble_code_to_deca(tmp_val) != -1)
            {
                fira_param->session.preamble_code_index = preamble_code_to_deca(tmp_val);
            }
            else
            {
                sz = sprintf(err_msg, "Incorrect Preamble Code Index\r\n");
                reporter_instance.print(err_msg, sz);
                return false;
            }
        }
        else if (sscanf(pch, "DRATE=%d", &tmp_val) == 1)
        {
            if (bitrate_to_deca(tmp_val) != -1)
            {
                fira_param->session.phr_data_rate = tmp_val;
            }
            else
            {
                sz = sprintf(err_msg, "Incorrect PHR Data Rate\r\n");
                reporter_instance.print(err_msg, sz);
                return false;
            }
        }
        else
        {
            if (cpt_arg != 0)
            {
                sz = sprintf(err_msg, "unknown argument %s\r\n", pch);
                reporter_instance.print(err_msg, sz);
                return false;
            }
        }
        pch = strtok(NULL, " -");
        cpt_arg++;
    }

    return true;
}

static error_e fira_app_process_init(bool controller, void const *arg)
{
    int r;

    fira_param_t *fira_param = (fira_param_t *)arg;
    struct fbs_session_init_rsp rsp;

    session_id = fira_param->session_id;
    uint16_t string_len = STR_SIZE * (controller ? fira_param->controlees_params.n_controlees : 1);

    output_result.str = qmalloc(string_len);
    if (!(output_result.str))
    {
        char *err = "not enough memory";
        reporter_instance.print((char *)err, strlen(err));
        return _ERR_Cannot_Alloc_Memory;
    }
    output_result.len = string_len;

    r = fira_uwb_mcps_init(&uwbmac_ctx);

    if (r != QERR_SUCCESS)
    {
        reporter_instance.print("uwb_mcps init failed", strlen("uwb_mcps init failed"));
        return _ERR;
    }

    /* Set local short address. */
    uwbmac_set_short_addr(uwbmac_ctx, fira_param->session.short_addr);
    /* Register report cb. */
    r = fira_helper_open(&fira_ctx, uwbmac_ctx, &fira_helper_ntfs_cb, "endless", 0, &output_result);
    if (r != QERR_SUCCESS)
    {
        reporter_instance.print("fira_helper_open failed", strlen("fira_helper_open failed"));
        return _ERR;
    }

    /* Set fira scheduler. */
    r = fira_helper_set_scheduler(&fira_ctx);
    if (r != QERR_SUCCESS)
    {
        reporter_instance.print("fira_helper_set_scheduler failed", strlen("fira_helper_set_scheduler failed"));
        return _ERR;
    }

    r = fira_prepare_measurement_sequence(uwbmac_ctx, &fira_param->session, true);
    if (r != QERR_SUCCESS)
    {
        reporter_instance.print("fira_prepare_measurement_sequence failed", strlen("fira_prepare_measurement_sequence failed"));
        return _ERR;
    }

    /* Init session. */
    r = fira_helper_init_session(&fira_ctx, session_id, QUWBS_FBS_SESSION_TYPE_RANGING_NO_IN_BAND_DATA, &rsp);
    if (r != QERR_SUCCESS)
    {
        reporter_instance.print("fira_helper_init_session failed", strlen("fira_helper_init_session failed"));
        return _ERR;
    }
    session_handle = rsp.session_handle;

    /* Set session parameters. */
    r = fira_set_session_parameters(&fira_ctx, session_handle, &fira_param->session);
    if (r != QERR_SUCCESS)
    {
        reporter_instance.print("fira_set_session_parameters failed", strlen("fira_set_session_parameters failed"));
        return _ERR;
    }

    if (controller)
    {
        for (int i = 0; i < fira_param->controlees_params.n_controlees; i++)
        {
            /* Add controlee session parameters. */
            r = fira_helper_add_controlee(&fira_ctx, session_handle, (const struct controlee_parameters *)&fira_param->controlees_params.controlees[i]);
            if (r != QERR_SUCCESS)
            {
                reporter_instance.print("fira_helper_add_controlee failed", strlen("fira_helper_add_controlee failed"));
                return _ERR;
            }
        }
    }
    return _NO_ERR;
}

static void fira_app_process_start(void)
{
    int r;

    /* OK, let's start. */
    r = uwbmac_start(uwbmac_ctx);
    if (r != QERR_SUCCESS)
    {
        reporter_instance.print("uwbmac_start failed", strlen("uwbmac_start failed"));
        return;
    }

    /* Start session. */
    r = fira_helper_start_session(&fira_ctx, session_handle);
    if (r != QERR_SUCCESS)
    {
        reporter_instance.print("fira_helper_start_session failed", strlen("fira_helper_start_session failed"));
        return;
    }
    started = true;
}

static error_e fira_app_process_terminate(void)
{
    int r;

    if (started)
    {
        started = false; /* do not allow re-entrance. */

        /* Stop the MAC. */
        uwbmac_stop(uwbmac_ctx);

        /* Stop session. */
        r = fira_helper_stop_session(&fira_ctx, session_handle);
        if (r != QERR_SUCCESS)
        {
            reporter_instance.print("fira_helper_stop_session failed", strlen("fira_helper_stop_session failed"));
            return r;
        }

        /* Uninit session. */
        r = fira_helper_deinit_session(&fira_ctx, session_handle);
        if (r != QERR_SUCCESS)
        {
            reporter_instance.print("fira_helper_deinit_session failed", strlen("fira_helper_deinit_session failed"));
            return r;
        }

        fira_helper_close(&fira_ctx);

        /* Unregister driver. */
        fira_uwb_mcps_deinit(uwbmac_ctx);

        qfree(output_result.str);
    }
    return _NO_ERR;
}

static void fira_helper_ntfs_cb(enum fira_helper_cb_type cb_type, const void *content, void *user_data)
{
    switch (cb_type)
    {
        case FIRA_HELPER_CB_TYPE_TWR_RANGE_NTF:
            fira_session_info_ntf_twr_cb((const struct fira_twr_ranging_results *)content, user_data);
            fira_range_diagnostics_ntf_cb(((const struct fira_twr_ranging_results *)content)->info, user_data);
            break;
        case FIRA_HELPER_CB_TYPE_SESSION_STATUS_NTF:
            fira_session_status_ntf_cb((const struct fira_session_status_ntf_content *)content, user_data);
            break;
        default:
            break;
    }
}

static char *fira_session_info_ntf_twr_status_to_string(uint8_t status)
{
    switch (status)
    {
        case QUWBS_FBS_STATUS_RANGING_SUCCESS:
            return "SUCCESS";
            break;
        case QUWBS_FBS_STATUS_RANGING_TX_FAILED:
            return "TX_FAILED";
            break;
        case QUWBS_FBS_STATUS_RANGING_RX_TIMEOUT:
            return "RX_TIMEOUT";
            break;
        case QUWBS_FBS_STATUS_RANGING_RX_PHY_DEC_FAILED:
            return "RX_PHY_DEC_FAILED";
            break;
        case QUWBS_FBS_STATUS_RANGING_RX_PHY_TOA_FAILED:
            return "RX_PHY_TOA_FAILED";
            break;
        case QUWBS_FBS_STATUS_RANGING_RX_PHY_STS_FAILED:
            return "RX_PHY_STS_FAILED";
            break;
        case QUWBS_FBS_STATUS_RANGING_RX_MAC_DEC_FAILED:
            return "RX_MAC_DEC_FAILED";
            break;
        case QUWBS_FBS_STATUS_RANGING_RX_MAC_IE_DEC_FAILED:
            return "RX_MAC_IE_DEC_FAILED";
            break;
        case QUWBS_FBS_STATUS_RANGING_RX_MAC_IE_MISSING:
            return "RX_MAC_IE_MISSING";
            break;
        default:
            return "Unknown";
            break;
    }
}

static char *fira_session_status_ntf_state_to_string(enum quwbs_fbs_session_state state)
{
    switch (state)
    {
        case QUWBS_FBS_SESSION_STATE_INIT:
            return "INIT";
            break;
        case QUWBS_FBS_SESSION_STATE_DEINIT:
            return "DEINIT";
            break;
        case QUWBS_FBS_SESSION_STATE_ACTIVE:
            return "ACTIVE";
            break;
        case QUWBS_FBS_SESSION_STATE_IDLE:
            return "IDLE";
            break;
        default:
            return "Unknown";
            break;
    }
}

static char *fira_session_status_ntf_reason_code_to_string(enum quwbs_fbs_reason_code reason_code)
{
    switch (reason_code)
    {
        case QUWBS_FBS_REASON_CODE_STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS:
            return "State change with session management commands";
        case QUWBS_FBS_REASON_CODE_MAX_RANGING_ROUND_RETRY_COUNT_REACHED:
            return "Max ranging round retry count reached";
        case QUWBS_FBS_REASON_CODE_MAX_NUMBER_OF_MEASUREMENTS_REACHED:
            return "Max number of measurements reached";
        case QUWBS_FBS_REASON_CODE_SESSION_SUSPENDED_DUE_TO_INBAND_SIGNAL:
            return "Session suspended due to inband signal";
        case QUWBS_FBS_REASON_CODE_SESSION_RESUMED_DUE_TO_INBAND_SIGNAL:
            return "Session resumed due to inband signal";
        case QUWBS_FBS_REASON_CODE_SESSION_STOPPED_DUE_TO_INBAND_SIGNAL:
            return "Session stopped due to inband signal";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_UL_TDOA_RANDOM_WINDOW:
            return "Invalid UL TDOA random window";
        case QUWBS_FBS_REASON_CODE_ERROR_MIN_FRAMES_PER_RR_NOT_SUPPORTED:
            return "Minimum frames per ranging round not supported";
        case QUWBS_FBS_REASON_CODE_ERROR_INTER_FRAME_INTERVAL_NOT_SUPPORTED:
            return "Inter frame interval not supported";
        case QUWBS_FBS_REASON_CODE_ERROR_SLOT_LENGTH_NOT_SUPPORTED:
            return "Slot length not supported";
        case QUWBS_FBS_REASON_CODE_ERROR_INSUFFICIENT_SLOTS_PER_RR:
            return "Insufficient slots per ranging round";
        case QUWBS_FBS_REASON_CODE_ERROR_MAC_ADDRESS_MODE_NOT_SUPPORTED:
            return "MAC address mode not supported";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_RANGING_DURATION:
            return "Invalid ranging duration";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_STS_CONFIG:
            return "Invalid STS configuration";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_RFRAME_CONFIG:
            return "Invalid RFrame configuration";
        case QUWBS_FBS_REASON_CODE_ERROR_HUS_NOT_ENOUGH_SLOTS:
            return "HUS not enough slots";
        case QUWBS_FBS_REASON_CODE_ERROR_HUS_CFP_PHASE_TOO_SHORT:
            return "HUS CFP phase too short";
        case QUWBS_FBS_REASON_CODE_ERROR_HUS_CAP_PHASE_TOO_SHORT:
            return "HUS CAP phase too short";
        case QUWBS_FBS_REASON_CODE_ERROR_HUS_OTHERS:
            return "HUS others";
        case QUWBS_FBS_REASON_CODE_ERROR_STATUS_SESSION_KEY_NOT_FOUND:
            return "Status session key not found";
        case QUWBS_FBS_REASON_CODE_ERROR_STATUS_SUB_SESSION_KEY_NOT_FOUND:
            return "Status sub session key not found";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_PREAMBLE_CODE_INDEX:
            return "Invalid preamble code index";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_SFD_ID:
            return "Invalid SFD ID";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_PSDU_DATA_RATE:
            return "Invalid PSDU data rate";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_PHR_DATA_RATE:
            return "Invalid PHR data rate";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_PREAMBLE_DURATION:
            return "Invalid preamble duration";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_STS_LENGTH:
            return "Invalid STS length";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_NUM_OF_STS_SEGMENTS:
            return "Invalid number of STS segments";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_NUM_OF_CONTROLEES:
            return "Invalid number of controlees";
        case QUWBS_FBS_REASON_CODE_ERROR_MAX_RANGING_REPLY_TIME_EXCEEDED:
            return "Max ranging reply time exceeded";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_DST_ADDRESS_LIST:
            return "Invalid destination address list";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_OR_NOT_FOUND_SUB_SESSION_ID:
            return "Invalid or not found sub session ID";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_RESULT_REPORT_CONFIG:
            return "Invalid result report configuration";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_RANGING_ROUND_CONTROL_CONFIG:
            return "Invalid ranging round control configuration";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_RANGING_ROUND_USAGE:
            return "Invalid ranging round usage";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_MULTI_NODE_MODE:
            return "Invalid multi node mode";
        case QUWBS_FBS_REASON_CODE_ERROR_RDS_FETCH_FAILURE:
            return "RDS fetch failure";
        case QUWBS_FBS_REASON_CODE_ERROR_REF_UWB_SESSION_DOES_NOT_EXIST:
            return "Ref UWB session does not exist";
        case QUWBS_FBS_REASON_CODE_ERROR_REF_UWB_SESSION_RANGING_DURATION_MISMATCH:
            return "Ref UWB session ranging duration mismatch";
        case QUWBS_FBS_REASON_CODE_ERROR_REF_UWB_SESSION_INVALID_OFFSET_TIME:
            return "Ref UWB session invalid offset time";
        case QUWBS_FBS_REASON_CODE_ERROR_REF_UWB_SESSION_LOST:
            return "Ref UWB session lost";
        case QUWBS_FBS_REASON_CODE_ERROR_DT_ANCHOR_RANGING_ROUNDS_NOT_CONFIGURED:
            return "DT anchor ranging rounds not configured";
        case QUWBS_FBS_REASON_CODE_ERROR_DT_TAG_RANGING_ROUNDS_NOT_CONFIGURED:
            return "DT tag ranging rounds not configured";
        case QUWBS_FBS_REASON_CODE_ERROR_UWB_INITIATION_TIME_EXPIRED:
            return "UWB initiation time expired";
        case QUWBS_FBS_REASON_CODE_AOSP_ERROR_INVALID_CHANNEL_WITH_AOA:
            return "AOSP error invalid channel with AOA";
        case QUWBS_FBS_REASON_CODE_AOSP_ERROR_STOPPED_DUE_TO_OTHER_SESSION_CONFLICT:
            return "AOSP error stopped due to other session conflict";
        case QUWBS_FBS_REASON_CODE_AOSP_REGULATION_UWB_OFF:
            return "AOSP regulation UWB off";
        case QUWBS_FBS_REASON_CODE_ERROR_MAX_STS_REACHED:
            return "Max STS reached";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_DEVICE_ROLE:
            return "Invalid device role";
        case QUWBS_FBS_REASON_CODE_ERROR_NOMEM:
            return "No memory";
        case QUWBS_FBS_REASON_CODE_ERROR_DRIVER_DOWN:
            return "Driver down";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_PROXIMITY_RANGE:
            return "Invalid proximity range";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_FRAME_INTERVAL:
            return "Invalid frame interval";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_CAP_SIZE_RANGE:
            return "Invalid CAP size range";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_SCHEDULE_MODE:
            return "Invalid schedule mode";
        case QUWBS_FBS_REASON_CODE_ERROR_INVALID_PRF_MODE:
            return "Invalid PRF mode";
        case QUWBS_FBS_REASON_CODE_ERROR_START_CONFIG:
            return "Start configuration";
        case QUWBS_FBS_REASON_CODE_ERROR_RDS_BUSY:
            return "RDS busy";
        default:
            return "Unknown";
    }
}

static char *fira_range_diagnostics_ntf_msg_id_to_string(uint8_t msg_id)
{
    switch (msg_id)
    {
        case FBS_MESSAGE_ID_RANGING_INITIATION:
            return "RANGING_INITIATION";
            break;
        case FBS_MESSAGE_ID_RANGING_RESPONSE:
            return "RANGING_RESPONSE";
            break;
        case FBS_MESSAGE_ID_RANGING_FINAL:
            return "RANGING_FINAL";
            break;
        case FBS_MESSAGE_ID_CONTROL:
            return "CONTROL";
            break;
        case FBS_MESSAGE_ID_MEASUREMENT_REPORT:
            return "MEASUREMENT_REPORT";
            break;
        case FBS_MESSAGE_ID_RESULT_REPORT:
            return "RESULT_REPORT";
            break;
        case FBS_MESSAGE_ID_CONTROL_UPDATE:
            return "CONTROL_UPDATE";
            break;
        default:
            return "Unknown";
            break;
    }
}

static char *fira_range_diagnostics_ntf_action_to_string(uint8_t action)
{
    switch (action)
    {
        case 0:
            return "RX";
            break;
        case 1:
            return "TX";
            break;
        default:
            return "RFU";
            break;
    }
}

static char *fira_range_diagnostics_ntf_frame_status_to_string(uint8_t frame_status)
{
    const char *str = "SUCCESS: %d, WIFI_COEX: %d, GRANT_DURATION_EXCEEDED: %d";
    static char result[60];

    sprintf(result, str, (frame_status & FIRA_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_SUCCESS) ? 1 : 0, (frame_status & FIRA_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_WIFI_COEX) ? 1 : 0, (frame_status & FIRA_RANGING_DIAGNOSTICS_FRAME_REPORTS_STATUS_FLAGS_GRANT_DURATION_EXCEEDED) ? 1 : 0);

    return result;
}

static void fira_session_info_ntf_twr_cb(const struct fira_twr_ranging_results *results, void *user_data)
{
    int len = 0;
    struct string_measurement *str_result = (struct string_measurement *)user_data;
    struct fira_twr_measurements *rm;

    len += snprintf(&str_result->str[len], str_result->len, "SESSION_INFO_NTF: ");
    len += snprintf(&str_result->str[len], str_result->len - len, "{session_handle=%" PRIu32 "", results->info->session_handle);
    len += snprintf(&str_result->str[len], str_result->len - len, ", sequence_number=%" PRIu32 "", results->info->sequence_number);
    len += snprintf(&str_result->str[len], str_result->len - len, ", block_index=%" PRIu32 "", results->info->block_index);
    len += snprintf(&str_result->str[len], str_result->len - len, ", n_measurements=%d", results->n_measurements);

    for (int i = 0; i < results->n_measurements; i++)
    {
        if (i > 0)
            len += snprintf(&str_result->str[len], str_result->len - len, ";");

        rm = (struct fira_twr_measurements *)(&results->measurements[i]);

        len += snprintf(&str_result->str[len], str_result->len - len, "\r\n\r [mac_address=0x%04x, status=\"%s\"", rm->short_addr, fira_session_info_ntf_twr_status_to_string(rm->status));

        if (rm->status == 0)
        {
            len += snprintf(&str_result->str[len], str_result->len - len, ", distance[cm]=%d", (int)rm->distance_cm);

            if (rm->local_aoa_measurements[0].aoa_fom_100 > 0)
                len += snprintf(&str_result->str[len], str_result->len - len, ", loc_az_pdoa=%0.2f, loc_az=%0.2f", convert_aoa_2pi_q16_to_deg(rm->local_aoa_measurements[0].pdoa_2pi), convert_aoa_2pi_q16_to_deg(rm->local_aoa_measurements[0].aoa_2pi));

            if (rm->local_aoa_measurements[1].aoa_fom_100 > 0)
                len += snprintf(&str_result->str[len], str_result->len - len, ", loc_el_pdoa=%0.2f, loc_el=%0.2f", convert_aoa_2pi_q16_to_deg(rm->local_aoa_measurements[1].pdoa_2pi), convert_aoa_2pi_q16_to_deg(rm->local_aoa_measurements[1].aoa_2pi));

            if (rm->remote_aoa_azimuth_fom_100 > 0)
                len += snprintf(&str_result->str[len], str_result->len - len, ", rmt_az=%0.2f", convert_aoa_2pi_q16_to_deg(rm->remote_aoa_azimuth_2pi));

            if (rm->remote_aoa_elevation_fom_100 > 0)
                len += snprintf(&str_result->str[len], str_result->len - len, ", rmt_el=%0.2f", convert_aoa_2pi_q16_to_deg(rm->remote_aoa_elevation_pi));

            if (rm->rssi)
                len += snprintf(&str_result->str[len], str_result->len - len, ", RSSI[dBm]=%0.1f", convert_rssi_q7_to_dbm(rm->rssi));
        }
        len += snprintf(&str_result->str[len], str_result->len - len, "]");
    }
    len += snprintf(&str_result->str[len], str_result->len - len, "}\r\n");
    reporter_instance.print((char *)str_result->str, len);
}

static void fira_range_diagnostics_ntf_cb(const struct fira_ranging_info *info, void *user_data)
{
    struct diagnostic_info *diagnostics_info = info->diagnostic;

    if (diagnostics_info == NULL)
        return;

    int len = 0;
    struct string_measurement *str_result = (struct string_measurement *)user_data;

    len += snprintf(&str_result->str[len], str_result->len, "RANGE_DIAGNOSTICS_NTF: ");
    len += snprintf(&str_result->str[len], str_result->len - len, "{n_reports=%d", (int)diagnostics_info->nb_reports);
    reporter_instance.print((char *)str_result->str, len);

    struct frame_report *current_report = diagnostics_info->reports;

    for (int i = 0; i < diagnostics_info->nb_reports; i++)
    {
        if (current_report == NULL)
            return;

        len = 0;
        str_result = (struct string_measurement *)user_data;

        len += snprintf(&str_result->str[len], str_result->len, "\r\n\r [msg_id=%s", fira_range_diagnostics_ntf_msg_id_to_string(current_report->msg_id));
        len += snprintf(&str_result->str[len], str_result->len - len, ", action=%s", fira_range_diagnostics_ntf_action_to_string(current_report->action));
        len += snprintf(&str_result->str[len], str_result->len - len, ", antenna_set=%u", current_report->antenna_set);
        len += snprintf(&str_result->str[len], str_result->len - len, ", frame_status={%s}", fira_range_diagnostics_ntf_frame_status_to_string(current_report->extra_status));
        len += snprintf(&str_result->str[len], str_result->len - len, ", cfo_present=%d", current_report->cfo_present);

        if (current_report->cfo_present)
        {
            /* Convert Q26 to hundredths of ppm: cfo_ppm = cfo_q26 * 100 * 2^10 / 2^26 */
            int32_t cfo_100ppm = (int32_t)((((int64_t)current_report->cfo_q26) * 100 * TEN_TO_6) >> 26);
            len += snprintf(&str_result->str[len], str_result->len - len, ", cfo_ppm=%" PRIi32 ".%u", cfo_100ppm / 100, abs(cfo_100ppm) % 100);
        }
        len += snprintf(&str_result->str[len], str_result->len - len, ", nb_aoa=%u", current_report->nb_aoa);

        struct aoa_measurement *current_aoa_meas = current_report->aoas;

        for (int i = 0; i < current_report->nb_aoa; i++)
        {
            if (current_aoa_meas == NULL)
                return;

            len += snprintf(&str_result->str[len], str_result->len - len, "\r\n\r tdoa=%i", current_aoa_meas->tdoa);
            len += snprintf(&str_result->str[len], str_result->len - len, ", pdoa=%i", current_aoa_meas->pdoa);
            len += snprintf(&str_result->str[len], str_result->len - len, ", aoa=%i", current_aoa_meas->aoa);
            len += snprintf(&str_result->str[len], str_result->len - len, ", fom=%u", current_aoa_meas->fom);
            len += snprintf(&str_result->str[len], str_result->len - len, ", type=%u", current_aoa_meas->type);

            current_aoa_meas = current_aoa_meas->next;
        }

        len += snprintf(&str_result->str[len], str_result->len - len, "]");
        if (i < (diagnostics_info->nb_reports - 1))
            len += snprintf(&str_result->str[len], str_result->len - len, ";");

        reporter_instance.print((char *)str_result->str, len);

        current_report = current_report->next;
    }

    len = 0;
    str_result = (struct string_measurement *)user_data;

    len += snprintf(&str_result->str[len], str_result->len, "}\r\n");
    reporter_instance.print((char *)str_result->str, len);
}

static void fira_session_status_ntf_cb(const struct fira_session_status_ntf_content *status, void *user_data)
{
    struct string_measurement *str_result = (struct string_measurement *)user_data;
    const char *fmt = "SESSION_STATUS_NTF: {state=\"%s\", reason=\"%s\"}\r\n";
    int len = 0;

    len = sprintf(str_result->str, fmt, fira_session_status_ntf_state_to_string(status->state), fira_session_status_ntf_reason_code_to_string(status->reason_code));

    reporter_instance.print(str_result->str, len);
}

/** @brief is a service function which starts the FiRa TWR top-level  application. */
static void fira_app(bool controller, void *arg)
{
    error_e err;

    err = fira_app_process_init(controller, arg);
    if (err != _NO_ERR)
    {
        error_handler(1, err);
    }
    fira_app_process_start();
}

/**
 * @brief Kills all task and timers related to FiRa.
 * DW3000's RX and IRQ shall be switched off before task termination,
 * that IRQ will not produce unexpected Signal.
 */
void fira_terminate(void)
{
    fira_app_process_terminate();
    uwbmac_exit(uwbmac_ctx);
}

void fira_helper_controller(const void *arg)
{
    /* Not used. */
    (void)arg;

    fira_app(true, (void *)get_fira_config());
}

/**
 * @brief Start FiRa Controller+Responder.
 * @param arg Pointer to fira_param_t or NULL if use global config.
 */
void fira_helper_controlee(const void *arg)
{
    /* Not used. */
    (void)arg;

    fira_app(false, (void *)get_fira_config());
}

const app_definition_t helpers_app_fira[] __attribute__((
    section(".known_apps")
))
= {
    {"INITF", mAPP, fira_helper_controller, fira_terminate, waitForCommand, command_parser, NULL},
    {"RESPF", mAPP, fira_helper_controlee, fira_terminate, waitForCommand, command_parser, NULL},
};

/**
 * @brief Action to be taken before saving params to NVM.
 * Ensures initialization if user didn't provide configuration before.
 */
void fira_save_params(void)
{
    fira_param_t *fira_params = get_fira_config();

    /* Reinit when config is default (user set application without providing params). */
    if (fira_params->config_state == FIRA_APP_CONFIG_DEFAULT)
    {
        fira_set_default_params(fira_params->app_type == FIRA_APP_INITF);
    }
    fira_params->config_state = FIRA_APP_CONFIG_SAVED;
}
