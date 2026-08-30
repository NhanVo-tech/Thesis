/**
 * @file      circular_buffer.h
 *
 * @brief     Interface for circular buffer
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

#ifndef CIRC_CNT
/* Return count in buffer.  */
#define CIRC_CNT(head, tail, size) (((head) - (tail)) & ((size)-1))
#endif

#ifndef CIRC_SPACE
/* Return space available, 0..size-1. */
#define CIRC_SPACE(head, tail, size) CIRC_CNT((tail), ((head) + 1), (size))
#endif


/**
 * @brief Chunked Circular Buffer structure.
 *
 * Chunked Circular Buffer works as a regular circular buffer but it allows to write data in chunks.
 * It's especially useful for DMA transfers where buffer's API cannot be used for write so we cannot
 * guarantee proper rollover of head and tail pointers. Downside is that we can have some padding
 * bytes at the end.
 */
struct cc_buff
{
    uint8_t *data;
    uint16_t size;
    uint16_t head;
    uint16_t tail;
    uint16_t cnt;
    uint16_t padding;
    uint16_t chunk_size;
};

/**
 * @brief Initialize buffer
 *
 * @param buff Pointer to instance of cc buffer.
 * @param buff_arr Pointer to the buffer array.
 * @param size Size of the buffer array.
 * @param chunk_size Size of the chunk.
 */
void cc_buff_init(struct cc_buff *buff, uint8_t *buff_arr, uint16_t size, uint16_t chunk_size);

/**
 * @brief Reset internal state of buffer
 *
 * @param buff Pointer to instance of cc buffer.
 */
void cc_buff_reset(struct cc_buff *buff);

/**
 * @brief  Get next pointer to free space in buffer.
 * Useful when we have to give pointer to DMA to write data.
 *
 * @param buff Pointer to instance of cc buffer.
 * @param max_write Maximum number of bytes to write.
 * @return Pointer to next free byte in the buffer, NULL if buffer is full.
 */
uint8_t *cc_buff_get_write_ptr(struct cc_buff *buff, uint16_t max_write);

/**
 * @brief  Update head and tail after DMA write.
 *
 * @param buff Pointer to instance of cc buffer.
 * @param bytes_written Number of bytes written without the API.
 */
void cc_buff_commit_write(struct cc_buff *buff, uint16_t bytes_written);

/**
 * @brief Write one byte to buffer.
 *
 * @param buff Pointer to instance of cc buffer.
 * @param data Byte to write.
 */
bool cc_buff_write_byte(struct cc_buff *buff, uint8_t data);

/**
 * @brief Get next byte from buffer.
 *
 * @param buff Pointer to instance of cc buffer.
 * @param data Pointer to store read byte.
 * @return true if byte was read, false if buffer is empty.
 */
bool cc_buff_get_byte(struct cc_buff *buff, uint8_t *data);

/**
 * @brief Copy n bytes from buffer to given destination array.
 * Specific rollover of head/tail is performed to handle padding bytes.
 *
 * @param buff Pointer to instance of cc buffer.
 * @param dest Pointer to store read data.
 * @param n Number of bytes to read.
 * @return Number of bytes copied.
 */
uint16_t cc_buff_copy_n(struct cc_buff *buff, uint8_t *dest, uint16_t n);
