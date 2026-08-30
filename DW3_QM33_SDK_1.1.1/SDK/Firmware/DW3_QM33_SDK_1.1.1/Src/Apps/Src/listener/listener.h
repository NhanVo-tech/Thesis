/**
 * @file      listener.h
 *
 * @brief     Interface for listener application
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "uwb_frames.h"
#include "appConfig.h"
#include "deca_device_api.h"

#define LISTENER_DATA 2

/* Rx Events circular buffer : used to transfer RxPckt from ISR to APP
 * 0x02, 0x04, 0x08, 0x10, etc.
 * As per design, the amount of RxPckt in the buffer at any given time shall not be more than 1. */
#define EVENT_BUF_L_SIZE (0x10)

/** @brief RxPckt is the structure is for the current reception. */
struct rx_listener_pckt_s
{
    int16_t rxDataLen;

    union
    {
        std_msg_t stdMsg;
        twr_msg_t twrMsg;
        blink_msg_t blinkMsg;
        rng_cfg_msg_t rngCfgMsg;
        poll_msg_t pollMsg;
        resp_pdoa_msg_t respMsg;
        final_msg_accel_t finalMsg;
        uint8_t data[STANDARD_FRAME_SIZE];
    } msg;

    /* Full TimeStamp. */
    uint8_t timeStamp[TS_40B_SIZE];

    /* Below is Qorvo's diagnostics information. */
    uint32_t status;
    int rsl100;
    int fsl100;
    int16_t clock_offset;
};

typedef struct rx_listener_pckt_s rx_listener_pckt_t;

/** @brief This structure holds Listener's application parameters. */
struct listener_info_s
{
    /** @brief Circular Buffer of received Rx packets
     * used in transferring of the data from ISR to APP level.
     */
    struct
    {
        rx_listener_pckt_t buf[EVENT_BUF_L_SIZE];
        uint16_t head;
        uint16_t tail;
    } rxPcktBuf;

    dwt_deviceentcnts_t event_counts;
    uint32_t event_counts_sfd_detect; /* Counts the number of SFD detections (RXFR has to be set also). */
};

typedef struct listener_info_s listener_info_t;

/* Exported functions prototypes. */
listener_info_t *getListenerInfoPtr(void);

/* Responder (Listener). */
error_e listener_process_init(void);
error_e listener_process_start(void);
void listener_process_terminate(void);
void listener_set_mode(int mode);
int listener_get_mode(void);

#ifdef __cplusplus
}
#endif
