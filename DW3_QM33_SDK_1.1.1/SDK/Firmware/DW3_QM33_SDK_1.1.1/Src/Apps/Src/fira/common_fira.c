/**
 *  @file     common_fira.c
 *
 *  @brief    Fira params control
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "common_fira.h"

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>

#include "fira_app_config.h"
#include "uwb_translate.h"
#include "qtime.h"
#include "fira_default_params.h"
#include "driver_app_config.h"
#include "uwb_utils.h"

static char *fira_device_role_param_to_string(enum quwbs_fbs_device_role device_role)
{
    switch (device_role)
    {
        case QUWBS_FBS_DEVICE_ROLE_RESPONDER:
            return "RESPONDER";
            break;
        case QUWBS_FBS_DEVICE_ROLE_INITIATOR:
            return "INITIATOR";
            break;
        default:
            return "Unknown";
            break;
    }
}

static char *fira_ranging_round_usage_param_to_string(enum fira_ranging_round_usage ranging_round_usage)
{
    switch (ranging_round_usage)
    {
        case FIRA_RANGING_ROUND_USAGE_SSTWR_DEFERRED:
            return "SS_TWR_DEFERRED";
            break;
        case FIRA_RANGING_ROUND_USAGE_DSTWR_DEFERRED:
            return "DS_TWR_DEFERRED";
            break;
        case FIRA_RANGING_ROUND_USAGE_SSTWR_NON_DEFERRED:
            return "SS_TWR_NON_DEFERRED";
            break;
        case FIRA_RANGING_ROUND_USAGE_DSTWR_NON_DEFERRED:
            return "DS_TWR_NON_DEFERRED";
            break;
        default:
            return "Unknown";
            break;
    }
}

static char *fira_multi_node_mode_param_to_string(enum fira_multi_node_mode multi_node_mode)
{
    switch (multi_node_mode)
    {
        case FIRA_MULTI_NODE_MODE_UNICAST:
            return "UNICAST";
            break;
        case FIRA_MULTI_NODE_MODE_ONE_TO_MANY:
            return "ONE_TO_MANY";
            break;
        default:
            return "Unknown";
            break;
    }
}

static char *fira_rframe_config_param_to_string(enum fira_rframe_config rframe_config)
{
    switch (rframe_config)
    {
        case FIRA_RFRAME_CONFIG_SP1:
            return "SP1";
            break;
        case FIRA_RFRAME_CONFIG_SP3:
            return "SP3";
            break;
        default:
            return "Unknown";
            break;
    }
}

void fira_show_params()
{
#define REMAINING (2048 - strlen(str))
    /* Display the Fira session parameters. */
    char *str = qmalloc(2048);

    if (!str)
    {
        FIRA_LOGE("Not enough memory.", strlen("Not enough memory."));
        return;
    }
    fira_param_t *fira_param = get_fira_config();
    sprintf(str, "FiRa Session Parameters: {\r\n");

    snprintf(&str[strlen(str)], REMAINING, "SESSION_ID: %" PRIu32 ",\r\n", fira_param->session_id);
    snprintf(&str[strlen(str)], REMAINING, "CHANNEL_NUMBER: %d,\r\n", fira_param->session.channel_number);
    snprintf(&str[strlen(str)], REMAINING, "DEVICE_ROLE: %s,\r\n", fira_device_role_param_to_string(fira_param->session.device_role));
    snprintf(&str[strlen(str)], REMAINING, "RANGING_ROUND_USAGE: %s,\r\n", fira_ranging_round_usage_param_to_string(fira_param->session.ranging_round_usage));
    snprintf(&str[strlen(str)], REMAINING, "SLOT_DURATION [rstu]: %" PRIu32 ",\r\n", fira_param->session.slot_duration_rstu);
    snprintf(&str[strlen(str)], REMAINING, "RANGING_DURATION [ms]: %" PRIu32 ",\r\n", fira_param->session.block_duration_ms);
    snprintf(&str[strlen(str)], REMAINING, "SLOTS_PER_RR: %" PRIu32 ",\r\n", fira_param->session.round_duration_slots);
    snprintf(&str[strlen(str)], REMAINING, "MULTI_NODE_MODE: %s,\r\n", fira_multi_node_mode_param_to_string(fira_param->session.multi_node_mode));
    snprintf(&str[strlen(str)], REMAINING, "HOPPING_MODE: %s,\r\n", fira_param->session.round_hopping ? "Enabled" : "Disabled");
    snprintf(&str[strlen(str)], REMAINING, "RFRAME_CONFIG: %s,\r\n", fira_rframe_config_param_to_string(fira_param->session.rframe_config));
    snprintf(&str[strlen(str)], REMAINING, "SFD_ID: %d,\r\n", fira_param->session.sfd_id);
    snprintf(&str[strlen(str)], REMAINING, "PREAMBLE_CODE_INDEX: %d,\r\n", fira_param->session.preamble_code_index);
    uint8_t *v = fira_param->session.vupper64;
    snprintf(&str[strlen(str)], REMAINING, "STATIC_STS_IV: \"%02x:%02x:%02x:%02x:%02x:%02x\",\r\n", v[0], v[1], v[2], v[3], v[4], v[5]);
    snprintf(&str[strlen(str)], REMAINING, "VENDOR_ID: \"%02x:%02x\",\r\n", v[6], v[7]);
    snprintf(&str[strlen(str)], REMAINING, "DEVICE_MAC_ADDRESS: 0x%04X,\r\n", fira_param->session.short_addr);

    if (fira_param->session.device_type == QUWBS_FBS_DEVICE_TYPE_CONTROLLER)
    {
        for (int i = 0; i < fira_param->controlees_params.n_controlees; i++)
        {
            snprintf(&str[strlen(str)], REMAINING, "DST_MAC_ADDRESS[%d]: 0x%04X", i, fira_param->controlees_params.controlees[i].address);
            if (i < fira_param->controlees_params.n_controlees - 1)
                snprintf(&str[strlen(str)], REMAINING, ",");
            snprintf(&str[strlen(str)], REMAINING, "\r\n");
        }
    }
    else
    {
        snprintf(&str[strlen(str)], REMAINING, "DST_MAC_ADDRESS: 0x%04X\r\n", fira_param->session.destination_short_address[0]);
    }
    snprintf(&str[strlen(str)], REMAINING, "}");

    FIRA_LOGI(str, strlen(str));
    qfree(str);
    /* Wait a little bit for printing. */
    qtime_msleep(100);
#undef REMAINING
}

enum qerr fira_prepare_measurement_sequence(struct uwbmac_context *uwbmac_ctx, struct session_parameters *session, bool is_report_required)
{
    struct uwbmac_device_info device_info;
    if (uwbmac_get_device_info(uwbmac_ctx, &device_info) != QERR_SUCCESS)
        return QERR_EFAULT;

    enum qerr r = QERR_SUCCESS;
    const aoa_supported_t is_aoa = uwb_device_is_aoa(device_info.dev_id);

    switch (is_aoa)
    {
        case AOA_SUPPORTED:
            session->meas_seq.n_steps = 1;
            session->meas_seq.steps[0].type = FIRA_MEASUREMENT_TYPE_AOA_AZIMUTH;
            session->meas_seq.steps[0].n_measurements = 1;
            session->meas_seq.steps[0].rx_ant_set_nonranging = 0xff;
            session->meas_seq.steps[0].rx_ant_sets_ranging[0] = 0xff;
            session->meas_seq.steps[0].rx_ant_sets_ranging[1] = 0xff;
            session->meas_seq.steps[0].tx_ant_set_nonranging = 0xff;
            session->meas_seq.steps[0].tx_ant_set_ranging = 0xff;
            if (is_report_required)
            {
                session->result_report_config |= fira_helper_bool_to_result_report_config(false, true, false, true);
            }
            else
            {
                session->result_report_config |= fira_helper_bool_to_result_report_config(false, false, false, false); /* NI-1.0.0 protocol does not support RAoA report */
            }
            break;

        case AOA_NOT_SUPPORTED:
            session->meas_seq.n_steps = 1;
            session->meas_seq.steps[0].type = FIRA_MEASUREMENT_TYPE_RANGE;
            session->meas_seq.steps[0].n_measurements = 1;
            session->meas_seq.steps[0].rx_ant_set_nonranging = 0xff;
            session->meas_seq.steps[0].rx_ant_sets_ranging[0] = 0xff;
            session->meas_seq.steps[0].rx_ant_sets_ranging[1] = 0xff;
            session->meas_seq.steps[0].tx_ant_set_nonranging = 0xff;
            session->meas_seq.steps[0].tx_ant_set_ranging = 0xff;
            if (is_report_required)
            {
                session->result_report_config &= ~fira_helper_bool_to_result_report_config(false, true, true, true);
            }
            else
            {
                session->result_report_config |= fira_helper_bool_to_result_report_config(false, false, false, false);
            }
            break;

        default:
            FIRA_LOGE("Found unknown chip 0x%04X. Stop.\r\n", (unsigned int)device_info.dev_id);
            r = QERR_ENOTSUP;
            break;
    }

    return r;
}

enum qerr fira_set_session_parameters(struct fira_context *fira_context, uint32_t session_handle, struct session_parameters *session)
{
    enum qerr r = QERR_SUCCESS;

#define SET_AND_RETURN_IF_FAILED(param)                                                    \
    do                                                                                     \
    {                                                                                      \
        r = fira_helper_set_session_##param(fira_context, session_handle, session->param); \
        if (r)                                                                             \
            return r;                                                                      \
    } while (0)

    SET_AND_RETURN_IF_FAILED(channel_number);
    SET_AND_RETURN_IF_FAILED(preamble_code_index);
    SET_AND_RETURN_IF_FAILED(sfd_id);
    SET_AND_RETURN_IF_FAILED(phr_data_rate);
    SET_AND_RETURN_IF_FAILED(prf_mode);
    SET_AND_RETURN_IF_FAILED(device_type);
    SET_AND_RETURN_IF_FAILED(device_role);
    SET_AND_RETURN_IF_FAILED(multi_node_mode);
    SET_AND_RETURN_IF_FAILED(rframe_config);
    SET_AND_RETURN_IF_FAILED(slot_duration_rstu);
    SET_AND_RETURN_IF_FAILED(block_duration_ms);
    SET_AND_RETURN_IF_FAILED(round_duration_slots);
    SET_AND_RETURN_IF_FAILED(ranging_round_usage);
    SET_AND_RETURN_IF_FAILED(round_hopping);
    SET_AND_RETURN_IF_FAILED(block_stride_length);
    SET_AND_RETURN_IF_FAILED(schedule_mode);
    SET_AND_RETURN_IF_FAILED(vupper64);
    SET_AND_RETURN_IF_FAILED(result_report_config);
    SET_AND_RETURN_IF_FAILED(ranging_round_control);
    SET_AND_RETURN_IF_FAILED(enable_diagnostics);
    SET_AND_RETURN_IF_FAILED(report_rssi);

    r = fira_helper_set_session_short_address(fira_context, session_handle, session->short_addr);
    if (r)
        return r;

    r = fira_helper_set_session_destination_short_addresses(fira_context, session_handle, session->n_destination_short_address, session->destination_short_address);
    if (r)
        return r;

    r = fira_helper_set_session_measurement_sequence(fira_context, session_handle, &session->meas_seq);
    if (r)
        return r;

    r = fira_helper_set_session_diags_frame_reports_fields(fira_context, session_handle, FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_SEGMENT_METRICS | FIRA_RANGING_DIAGNOSTICS_FRAME_REPORT_CFO);
    if (r)
        return r;

    return r;
}
