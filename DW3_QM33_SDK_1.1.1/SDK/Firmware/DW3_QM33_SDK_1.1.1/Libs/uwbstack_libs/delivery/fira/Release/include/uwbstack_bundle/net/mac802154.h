/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include <linux/bits.h>
#include <linux/skbuff.h>
#include <net/cfg802154.h>
#include <qerr.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Structure ieee802154_ops below have support for scanning. */
#define CONFIG_HAVE_IEEE802154_SCANNING 1

enum ieee802154_hw_addr_filt_flags {
	IEEE802154_AFILT_SADDR_CHANGED = BIT(0),
	IEEE802154_AFILT_IEEEADDR_CHANGED = BIT(1),
	IEEE802154_AFILT_PANID_CHANGED = BIT(2),
	IEEE802154_AFILT_PANC_CHANGED = BIT(3),
};

struct ieee802154_hw_addr_filt {
	uint64_t ieee_addr;
	uint16_t pan_id;
	uint16_t short_addr;
	bool pan_coord;
};

enum ieee802154_hw_flags {
	IEEE802154_HW_TX_OMIT_CKSUM = BIT(0),
	IEEE802154_HW_LBT = BIT(1),
	IEEE802154_HW_CSMA_PARAMS = BIT(2),
	IEEE802154_HW_FRAME_RETRIES = BIT(3),
	IEEE802154_HW_AFILT = BIT(4),
	IEEE802154_HW_PROMISCUOUS = BIT(5),
	IEEE802154_HW_RX_OMIT_CKSUM = BIT(6),
	IEEE802154_HW_RX_DROP_BAD_CKSUM = BIT(7),
};

#define IEEE802154_HW_OMIT_CKSUM (IEEE802154_HW_TX_OMIT_CKSUM | IEEE802154_HW_RX_OMIT_CKSUM)

struct ieee802154_hw {
	int extra_tx_headroom;
	uint32_t flags;
	struct device *parent;
	void *priv;
	struct wpan_phy *phy;
};

struct ieee802154_ops {
	struct module *owner;
	bool (*is_started)(struct ieee802154_hw *hw);
	enum qerr (*start)(struct ieee802154_hw *hw);
	void (*stop)(struct ieee802154_hw *hw);
	enum qerr (*xmit_sync)(struct ieee802154_hw *hw, struct sk_buff *skb);
	enum qerr (*xmit_async)(struct ieee802154_hw *hw, struct sk_buff *skb);
	enum qerr (*ed)(struct ieee802154_hw *hw, uint8_t *level);
	enum qerr (*set_channel)(struct ieee802154_hw *hw, uint8_t page, uint8_t channel);
	enum qerr (*set_hw_addr_filt)(struct ieee802154_hw *hw,
				      struct ieee802154_hw_addr_filt *filt, unsigned long changed);
	int (*set_txpower)(struct ieee802154_hw *hw, int32_t mbm);
	int (*set_lbt)(struct ieee802154_hw *hw, bool on);
	int (*set_cca_mode)(struct ieee802154_hw *hw, const struct wpan_phy_cca *cca);
	int (*set_cca_ed_level)(struct ieee802154_hw *hw, int32_t mbm);
	int (*set_csma_params)(struct ieee802154_hw *hw, uint8_t min_be, uint8_t max_be,
			       uint8_t retries);
	enum qerr (*set_frame_retries)(struct ieee802154_hw *hw, int8_t retries);
	enum qerr (*set_promiscuous_mode)(struct ieee802154_hw *hw, const bool on);
	void (*sw_scan_start)(struct ieee802154_hw *hw, uint64_t addr);
	void (*sw_scan_complete)(struct ieee802154_hw *hw);
};

struct ieee802154_hw *ieee802154_alloc_hw(size_t priv_data_len, const struct ieee802154_ops *ops);

void ieee802154_free_hw(struct ieee802154_hw *hw);

enum qerr ieee802154_register_hw(struct ieee802154_hw *hw);

void ieee802154_unregister_hw(struct ieee802154_hw *hw);

void ieee802154_rx_irqsafe(struct ieee802154_hw *hw, struct sk_buff *skb, uint8_t lqi);

void ieee802154_wake_queue(struct ieee802154_hw *hw);

void ieee802154_xmit_complete(struct ieee802154_hw *hw, struct sk_buff *skb, bool ifs_handling);

#ifdef __cplusplus
}
#endif
