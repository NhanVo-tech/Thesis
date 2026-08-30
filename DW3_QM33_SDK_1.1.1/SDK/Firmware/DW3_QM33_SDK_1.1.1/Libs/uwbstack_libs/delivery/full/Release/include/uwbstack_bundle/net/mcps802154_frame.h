/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "linux/skbuff.h"
#include "net/mcps802154.h"
#include "qbitfield.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IEEE802154_MTU 127

#define IEEE802154_PAN_ID_BROADCAST 0xffff
#define IEEE802154_ADDR_SHORT_BROADCAST 0xffff
#define IEEE802154_ADDR_SHORT_UNSPEC 0xfffe

#define IEEE802154_PAN_ID_LEN 2
#define IEEE802154_EXTENDED_ADDR_LEN 8
#define IEEE802154_SHORT_ADDR_LEN 2

/**
 * GET_MASK() - Get the bitmask from width and shift.
 * @prefix: Common prefix.
 * @member: Member name.
 */
#define GET_MASK(prefix, member) ((prefix##_##member##_WIDTH) << (prefix##_##member##_SHIFT))

/* From 802.15.4 v2.0 7.2.2 Frame Control field */
#define IEEE802154_FC_LEN 2

#define IEEE802154_FC_TYPE_BEACON 0x00
#define IEEE802154_FC_TYPE_DATA 0x01
#define IEEE802154_FC_TYPE_ACK 0x02
#define IEEE802154_FC_TYPE_MAC_CMD 0x03
#define IEEE802154_FC_TYPE_MULTIPURPOSE 0x5

#define IEEE802154_FC_TYPE_SHIFT 0
#define IEEE802154_FC_TYPE_MASK ((1 << 3) - 1)
#define IEEE802154_FC_TYPE(x) (((x)&IEEE802154_FC_TYPE_MASK) >> IEEE802154_FC_TYPE_SHIFT)
#define IEEE802154_FC_TYPE_WIDTH 0x7u

#define IEEE802154_FC_SECEN_SHIFT 3u
#define IEEE802154_FC_SECEN_WIDTH 0x1u
#define IEEE802154_FC_SECEN_MASK GET_MASK(IEEE802154_FC, SECEN)

#define IEEE802154_FC_FRPEND_SHIFT 4u
#define IEEE802154_FC_FRPEND_WIDTH 0x1u
#define IEEE802154_FC_FRPEND_MASK GET_MASK(IEEE802154_FC, FRPEND)

#define IEEE802154_FC_ACK_REQ_SHIFT 5u
#define IEEE802154_FC_ACK_REQ_WIDTH 0x1u
#define IEEE802154_FC_ACK_REQ_MASK GET_MASK(IEEE802154_FC, ACK_REQ)
#define IEEE802154_FC_ACK_REQ (1 << IEEE802154_FC_ACK_REQ_SHIFT)

#define IEEE802154_FC_INTRA_PAN_SHIFT 6u
#define IEEE802154_FC_INTRA_PAN_WIDTH 0x1u
#define IEEE802154_FC_INTRA_PAN_MASK GET_MASK(IEEE802154_FC, INTRA_PAN)

#define IEEE802154_FC_NO_SEQ_SHIFT 8u
#define IEEE802154_FC_NO_SEQ_WIDTH 0x1u
#define IEEE802154_FC_NO_SEQ_MASK GET_MASK(IEEE802154_FC, NO_SEQ)

#define IEEE802154_FC_IE_PRESENT_SHIFT 9u
#define IEEE802154_FC_IE_PRESENT_WIDTH 0x1u
#define IEEE802154_FC_IE_PRESENT_MASK GET_MASK(IEEE802154_FC, IE_PRESENT)

#define IEEE802154_FC_DAMODE_SHIFT 10u
#define IEEE802154_FC_DAMODE_WIDTH 0x3u
#define IEEE802154_FC_DAMODE_MASK GET_MASK(IEEE802154_FC, DAMODE)

#define IEEE802154_FC_VERSION_SHIFT 12u
#define IEEE802154_FC_VERSION_WIDTH 0x3u
#define IEEE802154_FC_VERSION_MASK GET_MASK(IEEE802154_FC, VERSION)

#define IEEE802154_FC_SAMODE_SHIFT 14u
#define IEEE802154_FC_SAMODE_WIDTH 0x3u
#define IEEE802154_FC_SAMODE_MASK GET_MASK(IEEE802154_FC, SAMODE)

#define IEEE802154_FC_MP_LONG_FC_WIDTH 0x1u
#define IEEE802154_FC_MP_LONG_FC_SHIFT 3u
#define IEEE802154_FC_MP_LONG_FC_MASK GET_MASK(IEEE802154_FC_MP, LONG_FC)

#define IEEE802154_FC_MP_DAMODE_WIDTH 0x3u
#define IEEE802154_FC_MP_DAMODE_SHIFT 4u
#define IEEE802154_FC_MP_DAMODE_MASK GET_MASK(IEEE802154_FC_MP, DAMODE)

#define IEEE802154_FC_MP_SAMODE_WIDTH 0x3u
#define IEEE802154_FC_MP_SAMODE_SHIFT 6u
#define IEEE802154_FC_MP_SAMODE_MASK GET_MASK(IEEE802154_FC_MP, SAMODE)

#define IEEE802154_FC_MP_PANID_WIDTH 0x3u
#define IEEE802154_FC_MP_PANID_SHIFT 8u
#define IEEE802154_FC_MP_PANID_MASK GET_MASK(IEEE802154_FC_MP, PANID)

#define IEEE802154_FC_MP_SECEN_WIDTH 0x1u
#define IEEE802154_FC_MP_SECEN_SHIFT 9u
#define IEEE802154_FC_MP_SECEN_MASK GET_MASK(IEEE802154_FC_MP, SECEN)

#define IEEE802154_FC_MP_NO_SEQ_WIDTH 0x1u
#define IEEE802154_FC_MP_NO_SEQ_SHIFT 10u
#define IEEE802154_FC_MP_NO_SEQ_MASK GET_MASK(IEEE802154_FC_MP, NO_SEQ)

#define IEEE802154_FC_MP_FRAME_PENDING_WIDTH 0x1u
#define IEEE802154_FC_MP_FRAME_PENDING_SHIFT 11u
#define IEEE802154_FC_MP_FRAME_PENDING_MASK GET_MASK(IEEE802154_FC_MP, FRAME_PENDING)

#define IEEE802154_FC_MP_VERSION_WIDTH 0x3u
#define IEEE802154_FC_MP_VERSION_SHIFT 12u
#define IEEE802154_FC_MP_VERSION_MASK GET_MASK(IEEE802154_FC_MP, VERSION)

#define IEEE802154_FC_MP_ACK_REQ_WIDTH 0x1u
#define IEEE802154_FC_MP_ACK_REQ_SHIFT 14u
#define IEEE802154_FC_MP_ACK_REQ_MASK GET_MASK(IEEE802154_FC_MP, ACK_REQ)

#define IEEE802154_FC_MP_IE_PRESENT_WIDTH 0x1u
#define IEEE802154_FC_MP_IE_PRESENT_SHIFT 15
#define IEEE802154_FC_MP_IE_PRESENT_MASK GET_MASK(IEEE802154_FC_MP, IE_PRESENT)

/**
 * IEEE802154_FC_VALUE() - Build a standard Frame Control field
 * as described in 802.15.4 v2.0, 7.2.2.
 * @type: Frame Type (3 bits). Not MULTIPURPOSE.
 * @secen: Security Enable flag.
 * @frpend: Frame Pending flag.
 * @ack: Acknowledge required flag.
 * @panc: PAN Id Compression flag.
 * @no_seq: Sequence number suppression flag (Warning negative logic).
 * @ie: IE presence flag.
 * @dam: Destination Address Mode (2 bits).
 * @ver: Frame version number (2 bits).
 * @sam: Source Address Mode (2 bits).
 */
#define IEEE802154_FC_VALUE(type, secen, frpend, ack, panc, no_seq, ie, dam, ver, sam) \
	(QFIELD_PREP(IEEE802154_FC_TYPE_MASK, (type)) |                                \
	 QFIELD_PREP(IEEE802154_FC_SECEN_MASK, (secen)) |                              \
	 QFIELD_PREP(IEEE802154_FC_FRPEND_MASK, (frpend)) |                            \
	 QFIELD_PREP(IEEE802154_FC_ACK_REQ_MASK, (ack)) |                              \
	 QFIELD_PREP(IEEE802154_FC_INTRA_PAN_MASK, (panc)) |                           \
	 QFIELD_PREP(IEEE802154_FC_NO_SEQ_MASK, (no_seq)) |                            \
	 QFIELD_PREP(IEEE802154_FC_IE_PRESENT_MASK, (ie)) |                            \
	 QFIELD_PREP(IEEE802154_FC_DAMODE_MASK, (dam)) |                               \
	 QFIELD_PREP(IEEE802154_FC_VERSION_MASK, (ver)) |                              \
	 QFIELD_PREP(IEEE802154_FC_SAMODE_MASK, (sam)))

/* From 802.15.4 v2.0 7.2.3 Sequence Number field */
#define IEEE802154_SEQ_LEN 1

/**
 * IEEE802154_FC_MP_VALUE_SHORT() - Build a short format MULTIPURPOSE Frame
 * Control field as described in 802.15.4 v2.0, 7.3.5.
 * @dam: Destination Adress mode (2 bits).
 * @sam: Source Adress mode (2 bits).
 */
#define IEEE802154_FC_MP_VALUE_SHORT(dam, sam)                                             \
	(IEEE802154_FC_TYPE_MULTIPURPOSE | QFIELD_PREP(IEEE802154_FC_MP_LONG_FC_MASK, 0) | \
	 QFIELD_PREP(IEEE802154_FC_MP_DAMODE_MASK, (dam)) |                                \
	 QFIELD_PREP(IEEE802154_FC_MP_SAMODE_MASK, (sam)))

/**
 * IEEE802154_FC_MP_VALUE_LONG() - Build a long format MULTIPURPOSE Frame
 * Control field as described in 802.15.4 v2.0, 7.3.5.
 * @dam: Destination Adress mode (2 bits).
 * @sam: Source Adress mode (2 bits).
 * @panid: PAN Id flag.
 * @secen: Security Enable flag.
 * @no_seq: Sequence Number suppression flag (Warning negative logic).
 * @frpend: Frame Pending flag.
 * @ver: Frame Version number (2 bits).
 * @ack: Acknowledge Requiered flag.
 * @ie: IE presence flag.
 */
#define IEEE802154_FC_MP_VALUE_LONG(dam, sam, panid, secen, no_seq, frpend, ver, ack, ie)  \
	(IEEE802154_FC_TYPE_MULTIPURPOSE | QFIELD_PREP(IEEE802154_FC_MP_LONG_FC_MASK, 1) | \
	 QFIELD_PREP(IEEE802154_FC_MP_DAMODE_MASK, (dam)) |                                \
	 QFIELD_PREP(IEEE802154_FC_MP_SAMODE_MASK, (sam)) |                                \
	 QFIELD_PREP(IEEE802154_FC_MP_PANID_MASK, (panid)) |                               \
	 QFIELD_PREP(IEEE802154_FC_MP_SECEN_MASK, (secen)) |                               \
	 QFIELD_PREP(IEEE802154_FC_MP_NO_SEQ_MASK, (no_seq)) |                             \
	 QFIELD_PREP(IEEE802154_FC_MP_FRAME_PENDING_MASK, (frpend)) |                      \
	 QFIELD_PREP(IEEE802154_FC_MP_VERSION_MASK, (ver)) |                               \
	 QFIELD_PREP(IEEE802154_FC_MP_ACK_REQ_MASK, (ack)) |                               \
	 QFIELD_PREP(IEEE802154_FC_MP_IE_PRESENT_MASK, (ie)))

/* From 802.15.4 v2.0 7.2.11 FCS field */
#define IEEE802154_FCS_LEN 2

/* From 802.15.4 v2.0 7.4.2 Header IEs. */
#define IEEE802154_IE_HEADER_LEN 2
#define IEEE802154_IE_HEADER_TYPE_SHIFT 15
#define IEEE802154_IE_HEADER_TYPE (1 << IEEE802154_IE_HEADER_TYPE_SHIFT)

#define IEEE802154_HEADER_IE_HEADER_LENGTH 0x7f
#define IEEE802154_HEADER_IE_HEADER_ELEMENT_ID (0xff << 7)
#define IEEE802154_HEADER_IE_HEADER_TYPE (0 << IEEE802154_IE_HEADER_TYPE_SHIFT)

/* From 802.15.4 v2.0 7.4.3 Payload IEs. */
#define IEEE802154_PAYLOAD_IE_HEADER_LENGTH 0x7ff
#define IEEE802154_PAYLOAD_IE_HEADER_GROUP_ID (0xf << 11)
#define IEEE802154_PAYLOAD_IE_HEADER_TYPE (1 << IEEE802154_IE_HEADER_TYPE_SHIFT)

/* From 802.15.4 v2.0 7.4.4 Nested IEs. */
#define IEEE802154_LONG_NESTED_IE_HEADER_LENGTH 0x7ff
#define IEEE802154_LONG_NESTED_IE_HEADER_SUB_ID (0xf << 11)
#define IEEE802154_LONG_NESTED_IE_HEADER_TYPE (1 << IEEE802154_IE_HEADER_TYPE_SHIFT)

#define IEEE802154_SHORT_NESTED_IE_HEADER_LENGTH 0xff
#define IEEE802154_SHORT_NESTED_IE_HEADER_SUB_ID (0x7f << 8)
#define IEEE802154_SHORT_NESTED_IE_HEADER_TYPE (0 << IEEE802154_IE_HEADER_TYPE_SHIFT)

#define IEEE802154_IE_NESTED_SHORT_MIN_SID 0x10

#define IEEE802154_IE_HEADER_VENDOR_ID 0x00
#define IEEE802154_IE_HEADER_TERMINATION_1_ID 0x7e
#define IEEE802154_IE_HEADER_TERMINATION_2_ID 0x7f

#define IEEE802154_IE_PAYLOAD_MLME_GID 0x1
#define IEEE802154_IE_PAYLOAD_VENDOR_GID 0x2
#define IEEE802154_IE_PAYLOAD_TERMINATION_GID 0xf

/* From 802.15.4 v2.0 7.5.1 MAC command ID field */
#define IEEE802154_CMD_ASSOCIATION_REQ 0x01
#define IEEE802154_CMD_ASSOCIATION_RESP 0x02
#define IEEE802154_CMD_DISASSOCIATION_NOTIFY 0x03
#define IEEE802154_CMD_DATA_REQ 0x04
#define IEEE802154_CMD_PANID_CONFLICT_NOTIFY 0x05
#define IEEE802154_CMD_ORPHAN_NOTIFY 0x06
#define IEEE802154_CMD_BEACON_REQ 0x07
#define IEEE802154_CMD_COORD_REALIGN_NOTIFY 0x08
#define IEEE802154_CMD_GTS_REQ 0x09

/* From 802.15.4 v2.0 9.4.2 Security Control field. */
#define IEEE802154_SCF_LEN 1
#define IEEE802154_SCF_NO_FRAME_COUNTER (1 << 5)

#define IEEE802154_SCF_SECLEVEL_NONE 0
#define IEEE802154_SCF_SECLEVEL_ENC_MIC64 6

#define IEEE802154_SCF_SECLEVEL_MASK 7
#define IEEE802154_SCF_KEY_ID_MODE_SHIFT 3
#define IEEE802154_SCF_KEY_ID_MODE_MASK (3 << IEEE802154_SCF_KEY_ID_MODE_SHIFT)
#define IEEE802154_SCF_FRAME_COUNTER_SUPPR_SHIFT 5
#define IEEE802154_SCF_FRAME_COUNTER_SUPPR_MASK (1 << IEEE802154_SCF_FRAME_COUNTER_SUPPR_SHIFT)
#define IEEE802154_SCF_ASN_IN_NONCE_SHIFT 6
#define IEEE802154_SCF_ASN_IN_NONCE_MASK (1 << IEEE802154_SCF_ASN_IN_NONCE_SHIFT)

struct mcps802154_llhw;

/**
 * enum mcps802154_ie_get_kind - Kind of IE, or none.
 * @MCPS802154_IE_GET_KIND_NONE: No IE decoded (at end of frame).
 * @MCPS802154_IE_GET_KIND_HEADER: Header IE decoded.
 * @MCPS802154_IE_GET_KIND_PAYLOAD: Payload IE decoded.
 * @MCPS802154_IE_GET_KIND_MLME_NESTED:
 *	Nested IE inside a MLME payload IE decoded.
 */
enum mcps802154_ie_get_kind {
	MCPS802154_IE_GET_KIND_NONE,
	MCPS802154_IE_GET_KIND_HEADER,
	MCPS802154_IE_GET_KIND_PAYLOAD,
	MCPS802154_IE_GET_KIND_MLME_NESTED,
};

/**
 * struct mcps802154_ie - Context for IE decoding, to be used with
 * mcps802154_ie_get(). Initialize to zero.
 */
struct mcps802154_ie {
	/**
	 * @in_payload: If true, next decoding is in payload.
	 */
	bool in_payload;
	/**
	 * @kind: Kind of decoded IE.
	 */
	enum mcps802154_ie_get_kind kind;
	/**
	 * @id: Element identifier, group identifier or sub identifier of the
	 * decoded IE.
	 */
	int id;
	/**
	 * @len: Length of the decoded IE.
	 */
	unsigned int len;
	/**
	 * @mlme_len: While an MLME IE is decoded, length of data still in the
	 * frame buffer for this IE. Set this to 0 if you pulled all the MLME
	 * payload.
	 */
	unsigned int mlme_len;
};

/**
 * mcps802154_frame_alloc() - Allocate a buffer for TX.
 * @llhw: Low-level device pointer.
 * @size: Header and payload size.
 * @flags: Allocation mask.
 *
 * This is to allocate a buffer for sending a frame to the low-level driver
 * directly.  Additional space is reserved for low-level driver headroom and for
 * checksum.
 *
 * Return: Allocated buffer, or NULL.
 */
struct sk_buff *mcps802154_frame_alloc(struct mcps802154_llhw *llhw, unsigned int size, int flags);

/**
 * mcps802154_ie_put_begin() - Begin writing information elements.
 * @skb: Frame buffer.
 *
 * Prepare a frame buffer for writing IEs. The buffer control buffer is used to
 * store state information.
 */
void mcps802154_ie_put_begin(struct sk_buff *skb);

/**
 * mcps802154_ie_put_end() - End writing information elements.
 * @skb: Frame buffer.
 * @data_payload: True if data will be appended to the buffer after the IEs. In
 * this case, a terminator IE may be needed.
 *
 * This function appends a terminator IE if needed.
 *
 * Return: Length of frame header or QERR_ENOBUFS in case of error.
 */
int mcps802154_ie_put_end(struct sk_buff *skb, bool data_payload);

/**
 * mcps802154_ie_put_header_ie() - Add a header IE.
 * @skb: Frame buffer.
 * @element_id: Header IE element identifier.
 * @len: Header IE payload length.
 *
 * This adds the IE header and reserves room to write your payload. This works
 * like skb_put, you must write at the returned address.
 *
 * Return: Address of reserved space to write payload, or NULL in case of error.
 */
void *mcps802154_ie_put_header_ie(struct sk_buff *skb, int element_id, unsigned int len);

/**
 * mcps802154_ie_put_payload_ie() - Add a payload IE.
 * @skb: Frame buffer.
 * @group_id: Payload IE group identifier.
 * @len: Payload IE payload length.
 *
 * This adds the IE header and reserves room to write your payload. This works
 * like skb_put, you must write at the returned address.
 *
 * Return: Address of reserved space to write payload, or NULL in case of error.
 */
void *mcps802154_ie_put_payload_ie(struct sk_buff *skb, int group_id, uint16_t len);

/**
 * mcps802154_ie_put_nested_mlme_ie() - Add a nested IE, inside a MLME IE.
 * @skb: Frame buffer.
 * @sub_id: Nested IE element identifier.
 * @len: Nested IE payload length.
 *
 * This adds the IE header and reserves room to write your payload. This works
 * like skb_put, you must write at the returned address.
 *
 * The MLME payload IE is added automatically if needed and its length is
 * incremented if present yet.
 *
 * Return: Address of reserved space to write payload, or NULL in case of error.
 */
void *mcps802154_ie_put_nested_mlme_ie(struct sk_buff *skb, int sub_id, unsigned int len);

/**
 * mcps802154_ie_get() - Parse one IE and fill context.
 * @skb: Frame buffer.
 * @ie: IE parsed context, should be zero initialized at first call.
 *
 * This should only be called if the buffer contains IEs. This can be determined
 * using the IE_PRESENT bit in the frame control.
 *
 * On successful parsing, the context structure is filled with information about
 * the read IE. The IE payload can be read at the head of the frame buffer,
 * headers are consumed.
 *
 * On last return, 1 is returned and a termination IE can be present in the
 * context, it usually can be ignored.
 *
 * When an MLME IE is found, you have two options:
 *  - ignore it and call again to parse nested IE.
 *  - pull nested payload from the frame buffer, in this case you should set
 *    mlme_len to zero to proceed with the next IE.
 *
 * Return: 1 if last IE, 0 on successfully decoded IE, else negative error code.
 */
int mcps802154_ie_get(struct sk_buff *skb, struct mcps802154_ie *ie);

/**
 * mcps802154_get_extended_addr() - Get current extended address.
 * @llhw: Low-level device pointer.
 *
 * Return: Extended address.
 */
uint64_t mcps802154_get_extended_addr(struct mcps802154_llhw *llhw);

/**
 * mcps802154_get_pan_id() - Get current PAN identifier.
 * @llhw: Low-level device pointer.
 *
 * Return: PAN ID.
 */
uint16_t mcps802154_get_pan_id(struct mcps802154_llhw *llhw);

/**
 * mcps802154_get_short_addr() - Get current short address.
 * @llhw: Low-level device pointer.
 *
 * Return: Short address.
 */
uint16_t mcps802154_get_short_addr(struct mcps802154_llhw *llhw);

/**
 * mcps802154_get_current_channel() - Get current channel.
 * @llhw: Low-level device pointer.
 *
 * Return: Channel parameters.
 */
const struct mcps802154_channel *mcps802154_get_current_channel(struct mcps802154_llhw *llhw);

/**
 * mcps802154_get_current_timestamp_dtu() - Get current timestamp in device time
 * unit.
 * @llhw: Low-level device pointer.
 * @timestamp_dtu: Pointer to timestamp to write.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_get_current_timestamp_dtu(struct mcps802154_llhw *llhw,
					       uint32_t *timestamp_dtu);

/**
 * mcps802154_tx_timestamp_dtu_to_rmarker_rctu() - Compute the RMARKER timestamp
 * in ranging counter time unit for a frame transmitted at given timestamp in
 * device time unit (RDEV only).
 * @llhw: Low-level device pointer.
 * @info: RMARKER information.
 * @rmarker_rctu: Pointer to RMARKER timestamp in rctu to write.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_tx_timestamp_dtu_to_rmarker_rctu(struct mcps802154_llhw *llhw,
						      const struct mcps802154_rmarker_info *info,
						      uint64_t *rmarker_rctu);

/**
 * mcps802154_difference_timestamp_rctu() - Compute the difference between two
 * timestamp values.
 * @llhw: Low-level device pointer.
 * @timestamp_a_rctu: Timestamp A value.
 * @timestamp_b_rctu: Timestamp B value.
 *
 * Return: Difference between A and B.
 */
int64_t mcps802154_difference_timestamp_rctu(struct mcps802154_llhw *llhw,
					     uint64_t timestamp_a_rctu, uint64_t timestamp_b_rctu);

/**
 * mcps802154_compute_frame_duration_dtu() - Compute the duration of a frame.
 * @llhw: Low-level device pointer.
 * @hrp_params: HRP UWB frame parameters. NULL is not allowed.
 * @sts_params: Frame parameters related to STS. NULL or zero-filled struct for
 * SP0.
 * @payload_bytes: Payload length (header and checksum included). Zero for SP3.
 *
 * Return: The duration in device time unit.
 */
int mcps802154_compute_frame_duration_dtu(struct mcps802154_llhw *llhw,
					  const struct mcps802154_hrp_uwb_params *hrp_params,
					  const struct mcps802154_sts_params *sts_params,
					  int payload_bytes);
/**
 * mcps802154_compute_max_payload() - Compute maximal possible payload length
 * (header and checksum included) for the given frame duration.
 * @llhw: Low-level device pointer.
 * @hrp_params: HRP UWB frame parameters. NULL is not allowed.
 * @sts_params: Frame parameters related to STS. NULL or zero-filled struct for
 * SP0.
 * @frame_duration_dtu: Frame duration in device time unit.
 *
 * Return: Maximal possible payload length (header and checksum included).
 */
int mcps802154_compute_max_payload(struct mcps802154_llhw *llhw,
				   const struct mcps802154_hrp_uwb_params *hrp_params,
				   const struct mcps802154_sts_params *sts_params,
				   int frame_duration_dtu);

/**
 * mcps802154_vendor_cmd() - Run a driver vendor specific command.
 * @llhw: Low-level device pointer.
 * @vendor_id: Vendor identifier as an OUI.
 * @subcmd: Command identifier.
 * @data: Data to be passed to driver, can be in/out.
 * @data_len: Data length.
 *
 * The valid moment to call this function is driver and command specific.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_vendor_cmd(struct mcps802154_llhw *llhw, uint32_t vendor_id, uint32_t subcmd,
				void *data, size_t data_len);

/**
 * mcps802154_get_aux_sec_index() - Get the MAC header buffer offset where
 * Auxiliary Security Header field is located.
 * @fc: Value of Frame Control field from MAC header.
 *
 * Return: Buffer offset or QERR_EBADMSG in case of failure.
 */
int mcps802154_get_aux_sec_index(uint16_t fc);

/**
 * mcps802154_rx_enable() - Enable RX.
 * @llhw: Low-level device pointer.
 * @info: frame config information.
 * @frame_idx: index of the frame in access block.
 * @next_delay_dtu: Delay between the start of the received
 *	frame and the next action.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_rx_enable(struct mcps802154_llhw *llhw,
			       const struct mcps802154_rx_frame_config *info, int frame_idx,
			       int next_delay_dtu);

/**
 * mcps802154_rx_get_frame() - Receive frame.
 * @llhw: Low-level device pointer.
 * @skb: Socket buffer to be filled.
 * @info: RX settings.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_rx_get_frame(struct mcps802154_llhw *llhw, struct sk_buff **skb,
				  struct mcps802154_rx_frame_info *info);

/**
 * mcps802154_rx_get_measurement() - Get measurement.
 * @llhw: Low-level device pointer.
 * @rx_ctx: Rx context (can be NULL).
 * @info: Measurements updated by the llhw.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps802154_rx_get_measurement(struct mcps802154_llhw *llhw, void *rx_ctx,
					struct mcps802154_rx_measurement_info *info);

#ifdef __cplusplus
}
#endif
