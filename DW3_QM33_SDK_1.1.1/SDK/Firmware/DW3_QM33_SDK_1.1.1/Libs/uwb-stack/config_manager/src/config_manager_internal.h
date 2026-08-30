/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#ifdef __CONFIG_MANAGER_INTERNAL_H__
/*
 * Do not include this file in a header !
 */
#error "Do not include this file in another header to avoid contamination !"
#endif

#ifndef __CONFIG_MANAGER_INTERNAL_H__
#define __CONFIG_MANAGER_INTERNAL_H__

/* Following trick allows unit tests for static functions defined here. */
#ifndef GOOGLE_TEST

#ifndef STATIC
#define STATIC static
#endif

#ifndef INLINE
#define INLINE inline
#endif

#else
/* Test only */
#define STATIC
#define INLINE

#endif /* GOOGLE_TEST. */

#endif /* __CONFIG_MANAGER_INTERNAL_H__. */
