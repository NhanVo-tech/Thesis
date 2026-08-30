/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct wpan_phy_cca {
	uint8_t dummy;
};

struct wpan_phy_supported {
	uint32_t channels[5];
};

struct wpan_phy {
	uint8_t current_page;
	uint8_t current_channel;
	struct wpan_phy_supported supported;
	uint64_t perm_extended_addr;
};

static inline const char *wpan_phy_name(__attribute__((unused)) struct wpan_phy *phy)
{
	return "phy0";
}

#ifdef __cplusplus
}
#endif
