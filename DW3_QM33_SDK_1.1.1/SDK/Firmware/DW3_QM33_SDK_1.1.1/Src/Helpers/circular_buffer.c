/**
 * @file      circular_buffer.c
 *
 * @brief     Implementation of circular buffer
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "circular_buffer.h"

#include "qlog.h"

#include <string.h>
#include <stdint.h>

void cc_buff_init(struct cc_buff *buff, uint8_t *buff_arr, uint16_t size, uint16_t chunk_size)
{
    assert(buff);
    assert(buff_arr);
    buff->data = buff_arr;
    buff->size = size;
    buff->head = 0;
    buff->tail = 0;
    buff->cnt = 0;
    buff->padding = 0;
    buff->chunk_size = chunk_size;
}

void cc_buff_reset(struct cc_buff *buff)
{
    assert(buff);
    buff->head = 0;
    buff->tail = 0;
    buff->cnt = 0;
    buff->padding = 0;
}

uint8_t *cc_buff_get_write_ptr(struct cc_buff *buff, uint16_t max_write)
{
    assert(buff);
    uint16_t space_left = buff->size - buff->head;

    if (max_write > buff->size - buff->cnt - buff->padding)
    {
        QLOGE("CC BUFF: Not enough space, cannot write.");
        return NULL;
    }
    /* Specific rollover to handle case when DMA can write whole chunk at once.
     * It can add empty padding bytes at the end but it speeds up the writing. */
    if (space_left < buff->chunk_size)
    {
        buff->padding = space_left;
        buff->head = 0;
    }

    return &buff->data[buff->head];
}

void cc_buff_commit_write(struct cc_buff *buff, uint16_t bytes_written)
{
    assert(buff);
    buff->head = (buff->head + bytes_written) % buff->size;
    buff->cnt += bytes_written;
}

bool cc_buff_write_byte(struct cc_buff *buff, uint8_t data)
{
    assert(buff);
    uint8_t *ptr = cc_buff_get_write_ptr(buff, sizeof(uint8_t));
    if (ptr)
    {
        *ptr = data;
        cc_buff_commit_write(buff, sizeof(uint8_t));
    }
    else
    {
        QLOGE("CC BUFF: Buffer full, cannot write.");
        return false;
    }

    return true;
}

bool cc_buff_get_byte(struct cc_buff *buff, uint8_t *data)
{
    assert(buff);
    assert(data);
    if (buff->cnt == 0)
    {
        QLOGE("CC BUFF: Buffer empty, cannot read.");
        return false;
    }

    return (bool)cc_buff_copy_n(buff, data, 1);
}

uint16_t cc_buff_copy_n(struct cc_buff *buff, uint8_t *dest, uint16_t n)
{
    assert(buff);
    assert(dest);
    uint16_t read_cnt = (n > buff->cnt) ? buff->cnt : n;
    uint16_t space_to_end = buff->size - buff->tail;

    if (buff->cnt == 0)
    {
        QLOGE("CC BUFF: Buffer empty, cannot read.");
        return 0;
    }

    /* Ignore padding bytes. */
    if (buff->padding)
    {
        space_to_end -= buff->padding;
    }
    /* If buffer is about to rollover, perform one copy from end of buffer, and second from the beginning. */
    if (read_cnt > space_to_end)
    {
        memcpy(dest, &buff->data[buff->tail], space_to_end);
        memcpy(dest + space_to_end, buff->data, read_cnt - space_to_end);
        buff->padding = 0;
        buff->tail = read_cnt - space_to_end;
    }
    else
    {
        memcpy(dest, &buff->data[buff->tail], read_cnt);
        buff->tail = (buff->tail + read_cnt);
    }
    buff->cnt -= read_cnt;

    return read_cnt;
}
