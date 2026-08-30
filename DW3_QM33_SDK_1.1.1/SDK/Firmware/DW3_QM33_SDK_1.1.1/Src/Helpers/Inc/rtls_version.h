/**
 * @file      rtls_version.h
 *
 * @brief     Version number
 *
 *            TAG_RELEASE_VERSION == 1
 *            Construct the version name as "MAJOR.MINOR.PATCH"
 *
 *            VER_MAJOR 0..999
 *            VER_MINOR 0..999
 *            VER_PATCH 0..999
 *
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

#define TAG_RELEASE_VERSION (1)

#define VER_MAJOR           1
#define VER_MINOR           1
#define VER_PATCH           0

#if VER_MAJOR >= 100
#define VMAJOR ((VER_MAJOR / 100) + '0'),        \
               (((VER_MAJOR % 100) / 10) + '0'), \
               ((VER_MAJOR % 10) + '0')
#elif VER_MAJOR >= 10
#define VMAJOR ((VER_MAJOR / 10) + '0'), \
               ((VER_MAJOR % 10) + '0')
#else
#define VMAJOR (VER_MAJOR + '0')
#endif

#if VER_MINOR >= 100
#define VMINOR ((VER_MINOR / 100) + '0'),        \
               (((VER_MINOR % 100) / 10) + '0'), \
               ((VER_MINOR % 10) + '0')
#elif VER_MINOR >= 10
#define VMINOR ((VER_MINOR / 10) + '0'), \
               ((VER_MINOR % 10) + '0')
#else
#define VMINOR (VER_MINOR + '0')
#endif

#if VER_PATCH >= 100
#define VPATCH ((VER_PATCH / 100) + '0'),        \
               (((VER_PATCH % 100) / 10) + '0'), \
               ((VER_PATCH % 10) + '0')
#elif VER_PATCH >= 10
#define VPATCH ((VER_PATCH / 10) + '0'), \
               ((VER_PATCH % 10) + '0')
#else
#define VPATCH (VER_PATCH + '0')
#endif

/* VERSION */
#define FULL_VERSION                           \
    {                                          \
        VMAJOR, '.', VMINOR, '.', VPATCH, '\0' \
    }

#ifdef __cplusplus
}
#endif
