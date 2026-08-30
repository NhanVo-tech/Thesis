/**
 * @file      listener.c
 *
 * @brief     Listener application implementation
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

/* Includes */
#include <math.h>
#include "listener.h"
#include "deca_dbg.h"
#include "circular_buffer.h"
#include "HAL_error.h"
#include "task_listener.h"
#include "driver_app_config.h"
#include "qirq.h"
#include "qplatform.h"
#include "uwb_utils.h"

#define NODE_MALLOC qmalloc
#define NODE_FREE   qfree

/* For 64 MHz PRF the RX code is 9. */
#define RX_CODE_THRESHOLD 8
/* Constant A for PRF of 16 MHz. See User Manual for more information. */
#define ALPHA_PRF_16 113.8
/* Constant A for PRF of 64 MHz. See User Manual for more information. */
#define ALPHA_PRF_64 120.7
/* 10log10(2^21) = 63.2    See User Manual for more information. */
#define LOG_CONSTANT_C0 63.2
/* 10log10(2^17) = 51.175  See User Manual for more information. */
#define LOG_CONSTANT_D0_E0 51.175

/** @brief holds all Listener's process parameters. */
static listener_info_t *psListenerInfo = NULL;

static int listener_mode = 0;

/**
 * @brief ISR level (need to be protected if called from APP level)
 * low-level configuration for DW3000.
 *
 * @note If called from app, shall be performed with DW IRQ off &
 * qirq_lock(); / qirq_unlock();
 * The SPI for selected UWB chip shall already be chosen.
 */
static void rxtx_listener_configure(dwt_config_t *pdwCfg, uint16_t frameFilter)
{
    /* Configure the Physical Channel parameters (PLEN, PRF, etc). */
    if (dwt_configure(pdwCfg))
    {
        error_handler(1, _ERR_INIT);
    }
    /* Any delays are set by default : part of config of receiver on Tx sending. */
    dwt_setrxaftertxdelay(0);
    dwt_setrxtimeout(0);
    dwt_configureframefilter(DWT_FF_DISABLE, 0);
}

static void listener_rssi_cal(int *rsl100, int *fsl100)
{
    dwt_nlos_alldiag_t all_diag;
    uint8_t D;
    dwt_config_t *dwt_config = get_dwt_config();

    /* All float variables used for recording different diagnostic results and probability. */
    float ip_alpha, log_constant = 0;
    float ip_f1, ip_f2, ip_f3, ip_n, ip_cp, ip_rsl, ip_fsl;

    uint32_t dev_id = dwt_readdevid();

    if ((dev_id == (uint32_t)DWT_DW3000_DEV_ID) || (dev_id == (uint32_t)DWT_DW3000_PDOA_DEV_ID))
    {
        log_constant = LOG_CONSTANT_C0;
    }
    else
    {
        log_constant = LOG_CONSTANT_D0_E0;
    }

    /* Select IPATOV to read Ipatov diagnostic registers from API function dwt_nlos_alldiag(). */
    all_diag.diag_type = IPATOV;
    dwt_nlos_alldiag(&all_diag);
    ip_alpha = (dwt_config->rxCode > RX_CODE_THRESHOLD) ? (-(ALPHA_PRF_64 + 1)) : -(ALPHA_PRF_16);
    /* The number of preamble symbols accumulated.  */
    ip_n = all_diag.accumCount;
    /* The First Path Amplitude (point 1) magnitude value (it has 2 fractional bits). */
    ip_f1 = all_diag.F1 / 4;
    /* The First Path Amplitude (point 2) magnitude value (it has 2 fractional bits). */
    ip_f2 = all_diag.F2 / 4;
    /* The First Path Amplitude (point 3) magnitude value (it has 2 fractional bits). */
    ip_f3 = all_diag.F3 / 4;
    ip_cp = all_diag.cir_power;

    D = all_diag.D * 6;

    /* For IPATOV. */
    ip_n *= ip_n;
    ip_f1 *= ip_f1;
    ip_f2 *= ip_f2;
    ip_f3 *= ip_f3;

    /* For the CIR Ipatov. */
    ip_rsl = 10 * log10((float)ip_cp / ip_n) + ip_alpha + log_constant + D;
    ip_fsl = 10 * log10(((ip_f1 + ip_f2 + ip_f3) / ip_n)) + ip_alpha + D;
    if ((ip_rsl < -120) || (ip_rsl > 0))
    {
        ip_rsl = 0;
    }
    if ((ip_fsl < -120) || (ip_fsl > 0))
    {
        ip_fsl = 0;
    }
    *rsl100 = (int)(ip_rsl * 100);
    *fsl100 = (int)(ip_fsl * 100);
}

static void listener_timeout_cb(const dwt_cb_data_t *rxd)
{
    listener_info_t *pListenerInfo = getListenerInfoPtr();
    dwt_readeventcounters(&pListenerInfo->event_counts);
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
}


static void listener_configure_uwb(dwt_cb_t cbRxOk, dwt_cb_t cbRxTo, dwt_cb_t cbRxErr)
{
    dwt_callbacks_s cbs = {0};

    /* DEBUG I/O 2&3 : configure the GPIOs which control the LEDs on HW. */
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
    /* DEBUG I/O 5&6 : configure TX/RX states to output on GPIOs. */
    dwt_setlnapamode(DWT_PA_ENABLE | DWT_LNA_ENABLE);

    cbs.cbRxOk = cbRxOk;
    cbs.cbRxTo = cbRxTo;
    cbs.cbRxErr = cbRxErr;
    dwt_setcallbacks(&cbs);

    dwt_setinterrupt(DWT_INT_TXFRS_BIT_MASK | DWT_INT_RXFCG_BIT_MASK | (DWT_INT_ARFE_BIT_MASK | DWT_INT_RXFSL_BIT_MASK | DWT_INT_RXSTO_BIT_MASK | DWT_INT_RXPHE_BIT_MASK | DWT_INT_RXFCE_BIT_MASK | DWT_INT_RXFTO_BIT_MASK), 0, 2);
    dwt_configciadiag(DW_CIA_DIAG_LOG_ALL);

    dwt_configeventcounters(1);

    dwt_app_config_t *dwt_app_config = get_app_dwt_config();
    dwt_setxtaltrim(dwt_app_config->xtal_trim);
}

/** @brief Get pointer to the twrInfo structure. */
listener_info_t *getListenerInfoPtr(void)
{
    return (psListenerInfo);
}

void listener_set_mode(int mode)
{
    listener_mode = mode;
}

int listener_get_mode(void)
{
    return listener_mode;
}

/** @brief ISR level TWR application Rx callback to be called from dwt_isr() as an Rx call-back. */
void rx_listener_cb(const dwt_cb_data_t *rxd)
{
    listener_info_t *pListenerInfo = getListenerInfoPtr();

    if (!pListenerInfo)
    {
        return;
    }

    const int size = sizeof(pListenerInfo->rxPcktBuf.buf) / sizeof(pListenerInfo->rxPcktBuf.buf[0]);
    int head = pListenerInfo->rxPcktBuf.head;
    int tail = pListenerInfo->rxPcktBuf.tail;

    if (CIRC_SPACE(head, tail, size) > 0)
    {
        rx_listener_pckt_t *p = &pListenerInfo->rxPcktBuf.buf[head];

        /* Raw Rx TimeStamp (STS or IPATOV based on STS config). */
        dwt_readrxtimestamp(p->timeStamp, DWT_COMPAT_NONE);
        /* Reading Clock offset for any Rx packets. */
        p->clock_offset = dwt_readclockoffset();
        p->status = rxd->status;

        pListenerInfo->event_counts_sfd_detect++;

        /* Check if this is an SP3 packet. */
        if (rxd->rx_flags & DWT_CB_DATA_RX_FLAG_ND)
        {
            p->rxDataLen = 0;
        }
        else
        {
            p->rxDataLen = MIN(rxd->datalength, sizeof(p->msg));
            /* Raw message. */
            dwt_readrxdata((uint8_t *)&p->msg, p->rxDataLen, 0);
        }

        listener_rssi_cal(&p->rsl100, &p->fsl100);
        /* RTOS : listenerTask can be not started yet. */
        if (listener_task_started())
        {
            head = (head + 1) & (size - 1);
            /* ISR level : do not need to protect. */
            pListenerInfo->rxPcktBuf.head = head;
        }
    }

    listener_task_notify();

    /* Take a snapshot of event counters. */
    dwt_readeventcounters(&pListenerInfo->event_counts);
    /* Re-enable receiver again - no timeout. */
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    /* ready to serve next raw reception */
}

void listener_error_cb(const dwt_cb_data_t *rxd)
{
    listener_timeout_cb(rxd);
}

/**
 * @brief App level RTOS-independent application level function
 * initializing of a TWR Node functionality.
 */
error_e listener_process_init()
{
    enum qerr err = qplatform_init();
    if (err != QERR_SUCCESS)
    {
        error_handler(1, _ERR_INIT);
        return _ERR_INIT;
    }

    if (!psListenerInfo)
    {
        psListenerInfo = NODE_MALLOC(sizeof(listener_info_t));
    }

    listener_info_t *pListenerInfo = getListenerInfoPtr();

    if (!pListenerInfo)
    {
        return (_ERR_Cannot_Alloc_NodeMemory);
    }

    /* Switch off receiver's rxTimeOut, RxAfterTxDelay, delayedRxTime,
     * autoRxEnable, dblBufferMode and autoACK,
     * clear all initial counters, etc. */
    memset(pListenerInfo, 0, sizeof(listener_info_t));
    memset(&pListenerInfo->event_counts, 0, sizeof(pListenerInfo->event_counts));

    pListenerInfo->event_counts_sfd_detect = 0;

    /* Configure non-zero initial variables.1 : from app parameters
     * The Listener has its configuration in the app->pConfig, see DEFAULT_CONFIG. */
    dwt_config_t *dwt_config = get_dwt_config();

    /* dwt_xx calls in app level Must be in protected mode (DW3000 IRQ disabled). */
    err = qplatform_uwb_interrupt_disable();
    if (err != QERR_SUCCESS)
    {
        error_handler(1, _ERR_INIT);
        return _ERR_INIT;
    }

    unsigned int lock = qirq_lock();
    /* Set callbacks to NULL inside dwt_initialise. */
    if (dwt_initialise(0) != DWT_SUCCESS)
    {
        qirq_unlock(lock);
        return (_ERR_INIT);
    }

    qplatform_uwb_spi_set_fast_rate_freq();

    const aoa_supported_t is_aoa = uwb_device_is_aoa(dwt_readdevid());
    if (is_aoa == AOA_SUPPORTED)
    {
        diag_printf("Found AOA chip. PDoA is available.\r\n");
    }
    else if (is_aoa == AOA_NOT_SUPPORTED)
    {
        dwt_config->pdoaMode = DWT_PDOA_M0;
        diag_printf("Found non-AOA chip. PDoA is not available.\r\n");
    }
    else
    {
        diag_printf("Found unknown chip 0x%04X. Stop.\r\n", (unsigned int)dwt_readdevid());
        qirq_unlock(lock);
        return DWT_ERROR;
    }

    /* Configure DW IC's UWB mode, no frame filtering for Listener. */
    rxtx_listener_configure(dwt_config, DWT_FF_DISABLE);
    listener_configure_uwb(rx_listener_cb, listener_timeout_cb, listener_error_cb);

    qirq_unlock(lock);

    return (_NO_ERR);
}

/**
 * @brief Enable DW3000 IRQ to start
 */
error_e listener_process_start(void)
{
    enum qerr r = qplatform_uwb_interrupt_enable();
    if (r != QERR_SUCCESS)
    {
        diag_printf("Listener Top Application: Failed to start, error %d\r\n", r);
        error_handler(1, _ERR_INIT);
        return (_ERR_INIT);
    }

    diag_printf("Listener Top Application: Started\r\n");

    return (_NO_ERR);
}

/**
 * @brief app level RTOS-independent application level function.
 * Deinitialize the pListenerInfo structure.
 * This must be executed in protected mode.
 */
void listener_process_terminate(void)
{
    if (psListenerInfo)
    {
        NODE_FREE(psListenerInfo);
        psListenerInfo = NULL;
    }
}
