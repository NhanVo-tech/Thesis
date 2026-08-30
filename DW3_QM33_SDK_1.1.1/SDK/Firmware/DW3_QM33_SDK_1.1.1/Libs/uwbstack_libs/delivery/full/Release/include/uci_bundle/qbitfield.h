/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "qtoolchain.h"

#define __bf_shf(x) (__builtin_ffsll(x) - 1)

#define QFIELD_MAX(_mask) ((typeof(_mask))((_mask) >> __bf_shf(_mask)))

#ifndef QFIELD_PREP
#define QFIELD_PREP(_mask, _val) (((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask))
#endif

#ifndef QFIELD_GET
#define QFIELD_GET(_mask, _reg) ((typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask)))
#endif
