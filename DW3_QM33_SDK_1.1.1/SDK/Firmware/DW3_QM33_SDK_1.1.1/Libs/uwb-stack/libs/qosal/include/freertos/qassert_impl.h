/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#ifndef QASSERT

#ifdef NDEBUG
/* Empty implementation to supress unused variables compilation error. */
#define QASSERT(cond) (void)(cond);
#endif

#endif /* QASSERT */
