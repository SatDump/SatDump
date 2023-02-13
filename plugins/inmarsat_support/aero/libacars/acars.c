/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <string.h>                         // memcpy(), strdup()
#ifdef WIN32
#else
#include <sys/time.h>                       // struct timeval
#endif
#include "config.h"                         // WITH_LIBXML2
#ifdef WITH_LIBXML2
#include <libxml/tree.h>                    // xmlBufferPtr, xmlBufferFree()
#endif
#include <aero/libacars/libacars.h>              // la_proto_node, la_proto_tree_find_protocol
#include <aero/libacars/macros.h>                // la_assert, la_debug_print
#include <aero/libacars/arinc.h>                 // la_arinc_parse()
#include <aero/libacars/media-adv.h>             // la_media_adv_parse()
#include <aero/libacars/miam.h>                  // la_miam_parse_and_reassemble()
#include <aero/libacars/crc.h>                   // la_crc16_ccitt()
#include <aero/libacars/vstring.h>               // la_vstring, LA_ISPRINTF()
#include <aero/libacars/json.h>                  // la_json_append_*()
#include <aero/libacars/util.h>                  // LA_XCALLOC, LA_XFREE, la_prettify_xml
#include <aero/libacars/hash.h>                  // LA_HASH_INIT, la_hash_string()
#include <aero/libacars/reassembly.h>
#include <aero/libacars/acars.h>

#define LA_ACARS_PREAMBLE_LEN    16         // including CRC and DEL, not including SOH
#define DEL 0x7f
#define STX 0x02
#define ETX 0x03
#define ETB 0x17
#define ACK 0x06
#define NAK 0x15
#define IS_DOWNLINK_BLK(bid) ((bid) >= '0' && (bid) <= '9')

#define LA_ACARS_REASM_TABLE_CLEANUP_INTERVAL 1000

typedef struct {
	struct timeval downlink, uplink;
} la_acars_timeout_profile;

// Reassembly timers for various ACARS radio bearers.
static la_acars_timeout_profile const timeout_profiles[] = {
	[LA_ACARS_BEARER_INVALID] = {
		.downlink = { .tv_sec = 0,    .tv_usec = 0 },
		.uplink   = { .tv_sec = 0,    .tv_usec = 0 }
	},
	// Note: LA_ACARS_BEARER_VHF applies both to PoA and VDL2.
	// ARINC 618 specifies 420 and 90 seconds for VDL2, respectively,
	// however these values are too small and would result in many
	// incomplete reassemblies, especially in downlink direction.
	// PoA timers seem to work better in practice.
	[LA_ACARS_BEARER_VHF] = {
		.downlink = { .tv_sec = 660,  .tv_usec = 0 },    // VGT4
		.uplink   = { .tv_sec = 90,   .tv_usec = 0 }     // VAT4
	},
	[LA_ACARS_BEARER_HFDL] = {
		.downlink = { .tv_sec = 1260, .tv_usec = 0 },    // HFGT4
		.uplink   = { .tv_sec = 370,  .tv_usec = 0 }     // HFAT4
	},
	[LA_ACARS_BEARER_SATCOM] = {
		.downlink = { .tv_sec = 1260, .tv_usec = 0 },    // SGT4
		.uplink   = { .tv_sec = 280,  .tv_usec = 0 }     // SAT4
	}
};

typedef struct {
	char *reg, *label, *msg_num;
} la_acars_key;

static uint32_t la_acars_key_hash(void const *key) {
	la_acars_key *k = (la_acars_key *)key;
	uint32_t h = la_hash_string(k->reg, LA_HASH_INIT);
	h = la_hash_string(k->label, h);
	h = la_hash_string(k->msg_num, h);
	return h;
}

static bool la_acars_key_compare(void const *key1, void const *key2) {
	la_acars_key const *k1 = key1;
	la_acars_key const *k2 = key2;
	return (!strcmp(k1->reg, k2->reg) &&
			!strcmp(k1->label, k2->label) &&
			!strcmp(k1->msg_num, k2->msg_num));
}

static void la_acars_key_destroy(void *ptr) {
	if(ptr == NULL) {
		return;
	}
	la_acars_key *key = ptr;
	la_debug_print(D_INFO, "DESTROY KEY %s %s %s\n", key->reg, key->label, key->msg_num);
	LA_XFREE(key->reg);
	LA_XFREE(key->label);
	LA_XFREE(key->msg_num);
	LA_XFREE(key);
}

static void *la_acars_tmp_key_get(void const *msg) {
	la_assert(msg != NULL);
	la_acars_msg const *amsg = msg;
	LA_NEW(la_acars_key, key);
	key->reg = (char *)amsg->reg;
	key->label = (char *)amsg->label;
	key->msg_num = (char *)amsg->msg_num;
	return (void *)key;
}

static void *la_acars_key_get(void const *msg) {
	la_assert(msg != NULL);
	la_acars_msg const *amsg = msg;
	LA_NEW(la_acars_key, key);
	key->reg = strdup(amsg->reg);
	key->label = strdup(amsg->label);
	key->msg_num = strdup(amsg->msg_num);
	la_debug_print(D_INFO, "ALLOC KEY %s %s %s\n", key->reg, key->label, key->msg_num);
	return (void *)key;
}

static la_reasm_table_funcs acars_reasm_funcs = {
	.get_key = la_acars_key_get,
	.get_tmp_key = la_acars_tmp_key_get,
	.hash_key = la_acars_key_hash,
	.compare_keys = la_acars_key_compare,
	.destroy_key = la_acars_key_destroy
};

la_proto_node *la_acars_apps_parse_and_reassemble(char const *reg,
		char const *label, char const *txt, la_msg_dir msg_dir,
		la_reasm_ctx *rtables, struct timeval rx_time) {
	la_proto_node *ret = NULL;
	if(label == NULL || txt == NULL) {
		goto end;
	}
	switch(label[0]) {
		case 'A':
			switch(label[1]) {
				case '6':
				case 'A':
					if((ret = la_arinc_parse(txt, msg_dir)) != NULL) {
						goto end;
					}
					break;
			}
			break;
		case 'B':
			switch(label[1]) {
				case '6':
				case 'A':
					if((ret = la_arinc_parse(txt, msg_dir)) != NULL) {
						goto end;
					}
					break;
			}
			break;
		case 'H':
			switch(label[1]) {
				case '1':
					if((ret = la_arinc_parse(txt, msg_dir)) != NULL) {
						goto end;
					}
					if((ret = la_miam_parse_and_reassemble(reg, txt, rtables, rx_time)) != NULL) {
						goto end;
					}
					break;
			}
			break;
		case 'M':
			switch(label[1]) {
				case 'A':
					if((ret = la_miam_parse_and_reassemble(reg, txt, rtables, rx_time)) != NULL) {
						goto end;
					}
					break;
			}
			break;
		case 'S':
			switch(label[1]) {
				case 'A':
					if((ret = la_media_adv_parse(txt)) != NULL) {
						goto end;
					}
					break;
			}
			break;
	}
end:
	return ret;
}

la_proto_node *la_acars_decode_apps(char const *label,
		char const *txt, la_msg_dir msg_dir) {
	return la_acars_apps_parse_and_reassemble(NULL, label, txt, msg_dir,
			NULL, (struct timeval){ .tv_sec = 0, .tv_usec = 0 });
}

#define COPY_IF_NOT_NULL(d, s, l) do { \
	if((d) != NULL && (s) != NULL) { \
		memcpy((d), (s), (l)); \
	} } while(0)

#define BUF_CLEAR(p, l) do { \
	if((p) != NULL) { \
		memset((p), 0, (l)); \
	} } while(0)

int la_acars_extract_sublabel_and_mfi(char const *label, la_msg_dir msg_dir,
		char const *txt, int len, char *sublabel, char *mfi) {

	if(txt == NULL || label == NULL || strlen(label) < 2) {
		return -1;
	}
	if(msg_dir != LA_MSG_DIR_AIR2GND && msg_dir != LA_MSG_DIR_GND2AIR) {
		return -1;
	}

	int consumed = 0;
	int remaining = len;
	char const *ptr = txt;
	char const *sublabel_ptr = NULL, *mfi_ptr = NULL;

	BUF_CLEAR(sublabel, 3);
	BUF_CLEAR(mfi, 3);

	if(label[0] == 'H' && label[1] == '1') {
		if(msg_dir == LA_MSG_DIR_GND2AIR) {
			// Note: this algorithm works correctly only for service-related messages
			// without SMT header. The header, if present, precedes the "- #" character
			// sequence, while this algorithm expects this sequence to appear at the
			// start of the message text. However this is not a big deal since the main
			// purpose of this routine is to skip initial sublabel/MFI part and return
			// a pointer to the beginning of the next layer application payload (eg.
			// ARINC-622 ATS message). Right now libacars does not decode any application
			// layer protocols transmitted with SMT headers, so it's not a huge issue.
			if(remaining >= 5 && strncmp(ptr, "- #", 3) == 0) {
				la_debug_print(D_INFO, "Uplink sublabel: %c%c\n", ptr[3], ptr[4]);
				sublabel_ptr = ptr + 3;
				ptr += 5; consumed += 5; remaining -= 5;
			}
		} else if(msg_dir == LA_MSG_DIR_AIR2GND) {
			if(remaining >= 4 && ptr[0] == '#' && ptr[3] == 'B') {
				la_debug_print(D_INFO, "Downlink sublabel: %c%c\n", ptr[1], ptr[2]);
				sublabel_ptr = ptr + 1;
				ptr += 4; consumed += 4; remaining -= 4;
			}
		}
		// Look for MFI only if sublabel has been found
		if(sublabel_ptr == NULL) {
			goto end;
		}
		// MFI format is the same for both directions
		if(remaining >= 4 && ptr[0] == '/' && ptr[3] == ' ') {
			la_debug_print(D_INFO, "MFI: %c%c\n", ptr[1], ptr[2]);
			mfi_ptr = ptr + 1;
			ptr += 4; consumed += 4; remaining -= 4;
		}
	}
end:
	COPY_IF_NOT_NULL(sublabel, sublabel_ptr, 2);
	COPY_IF_NOT_NULL(mfi, mfi_ptr, 2);
	la_debug_print(D_INFO, "consumed %d bytes\n", consumed);
	return consumed;
}

// Note: buf must contain raw ACARS bytes, NOT including initial SOH byte
// (0x01) and including terminating DEL byte (0x7f).
la_proto_node *la_acars_parse_and_reassemble(uint8_t const* buf, int len, la_msg_dir msg_dir,
		la_reasm_ctx *rtables, struct timeval rx_time) {
	if(buf == NULL) {
		return NULL;
	}

	la_proto_node *node = la_proto_node_new();
	LA_NEW(la_acars_msg, msg);
	node->data = msg;
	node->td = &la_DEF_acars_message;
	char *buf2 = LA_XCALLOC(len, sizeof(char));

	msg->err = false;
	if(len < LA_ACARS_PREAMBLE_LEN) {
		la_debug_print(D_ERROR, "Preamble too short: %u < %u\n", len, LA_ACARS_PREAMBLE_LEN);
		goto fail;
	}

	if(buf[len-1] != DEL) {
		la_debug_print(D_ERROR, "%02x: no DEL byte at end\n", buf[len-1]);
		goto fail;
	}
	len--;

	uint16_t crc = la_crc16_ccitt(buf, len, 0);
	la_debug_print(D_INFO, "CRC check result: %04x\n", crc);
	len -= 2;
	msg->crc_ok = (crc == 0);

	int i = 0;
	for(i = 0; i < len; i++) {
		buf2[i] = buf[i] & 0x7f;
	}
	la_debug_print_buf_hex(D_VERBOSE, buf2, len, "After CRC and parity bit removal:\n");
	la_debug_print(D_INFO, "Length: %d\n", len);

	if(buf2[len-1] == ETX) {
		msg->final_block = true;
	} else if(buf2[len-1] == ETB) {
		msg->final_block = false;
	} else {
		la_debug_print(D_ERROR, "%02x: no ETX/ETB byte at end of text\n", buf2[len-1]);
		goto fail;
	}
	len--;
	// Here we have DEL, CRC and ETX/ETB bytes removed.
	// There are at least 12 bytes remaining.

	int remaining = len;
	char *ptr = buf2;

	msg->mode = *ptr;
	ptr++; remaining--;

	memcpy(msg->reg, ptr, 7);
	msg->reg[7] = '\0';
	ptr += 7; remaining -= 7;

	msg->ack = *ptr;
	ptr++; remaining--;

	// change special values to something printable
	if (msg->ack == NAK) {
		msg->ack = '!';
	} else if(msg->ack == ACK) {
		msg->ack = '^';
	}

	msg->label[0] = *ptr++;
	msg->label[1] = *ptr++;
	remaining -= 2;

	if (msg->label[1] == 0x7f) {
		msg->label[1] = 'd';
	}
	msg->label[2] = '\0';

	msg->block_id = *ptr;
	ptr++; remaining--;

	if (msg->block_id == 0) {
		msg->block_id = ' ';
	}
	// If the message direction is unknown, guess it using the block ID character.
	if(msg_dir == LA_MSG_DIR_UNKNOWN) {
		if(IS_DOWNLINK_BLK(msg->block_id)) {
			msg_dir = LA_MSG_DIR_AIR2GND;
		} else {
			msg_dir = LA_MSG_DIR_GND2AIR;
		}
		la_debug_print(D_ERROR, "Assuming msg_dir=%d\n", msg_dir);
	}
	if(remaining < 1) {
		// ACARS preamble has been consumed up to this point.
		// If this is an uplink with an empty message text, then we are done.
		// XXX: we are skipping the reassembly stage here, is this 100% correct?
		// If this ever needs to be changed, special care has to be taken for
		// empty ACKs (label: _<7F> aka _d), because they have out-of-sequence
		// block IDs (X, Y, Z, X, ...).
		if(!IS_DOWNLINK_BLK(msg->block_id)) {
			msg->txt = strdup("");
			msg->reasm_status = LA_REASM_SKIPPED;
			goto end;
		} else {
			la_debug_print(D_ERROR, "No text field in downlink message\n");
			goto fail;
		}
	}
	// Otherwise we expect STX here.
	if(*ptr != STX) {
		la_debug_print(D_ERROR, "%02x: No STX byte after preamble\n", *ptr);
		goto fail;
	}
	ptr++; remaining--;

	// Replace NULLs in message text to make it printable
	// XXX: Should we replace all nonprintable chars here?
	for(i = 0; i < remaining; i++) {
		if(ptr[i] == 0) {
			ptr[i] = '.';
		}
	}
	// Extract downlink-specific fields from message text
	if (IS_DOWNLINK_BLK(msg->block_id)) {
		if(remaining < 10) {
			la_debug_print(D_ERROR, "Downlink text field too short: %d < 10\n", remaining);
			goto fail;
		}
		memcpy(msg->msg_num, ptr, 3);
		msg->msg_num[3] = '\0';
		msg->msg_num_seq = ptr[3];
		ptr += 4; remaining -= 4;
		memcpy(msg->flight_id, ptr, 6);
		ptr += 6; remaining -= 6;
	}

	// Extract sublabel and MFI if present
	int offset = la_acars_extract_sublabel_and_mfi(msg->label, msg_dir,
			ptr, remaining, msg->sublabel, msg->mfi);
	if(offset > 0) {
		ptr += offset;
		remaining -= offset;
	}

	la_reasm_table *acars_rtable = NULL;
	if(rtables != NULL) {       // reassembly engine is enabled
		acars_rtable = la_reasm_table_lookup(rtables, &la_DEF_acars_message);
		if(acars_rtable == NULL) {
			acars_rtable = la_reasm_table_new(rtables, &la_DEF_acars_message,
					acars_reasm_funcs, LA_ACARS_REASM_TABLE_CLEANUP_INTERVAL);
		}
		bool down = IS_DOWNLINK_BLK(msg->block_id);

		long int acars_bearer = LA_ACARS_BEARER_INVALID;
		(void)la_config_get_int("acars_bearer", &acars_bearer);
		if(acars_bearer < LA_ACARS_BEARER_MIN || acars_bearer > LA_ACARS_BEARER_MAX) {
			// This bearer will cause reassembly to fail with LA_REASM_INVALID_ARGS
			acars_bearer = LA_ACARS_BEARER_INVALID;
		}
		la_acars_timeout_profile const *timeout_profile = timeout_profiles + acars_bearer;
		la_debug_print(D_VERBOSE, "Using timeout profile for bearer %ld (up: %lu dn: %lu)\n",
				acars_bearer,
				timeout_profile->uplink.tv_sec,
				timeout_profile->downlink.tv_sec);

		msg->reasm_status = la_reasm_fragment_add(acars_rtable,
				&(la_reasm_fragment_info){
				.msg_info = msg,
				.msg_data = (uint8_t *)ptr,
				.msg_data_len = remaining,
				.total_pdu_len = 0,        // not used here
				.seq_num = down ? msg->msg_num_seq - 'A' : msg->block_id - 'A',
				.seq_num_first = down ? 0 : SEQ_FIRST_NONE,
				.seq_num_wrap = down ? SEQ_WRAP_NONE : 'X' - 'A',
				.is_final_fragment = msg->final_block,
				.rx_time = rx_time,
				.reasm_timeout = down ? timeout_profile->downlink : timeout_profile->uplink
				});
	}
	uint8_t *reassembled_msg = NULL;
	if(msg->reasm_status == LA_REASM_COMPLETE &&
			la_reasm_payload_get(acars_rtable, msg, &reassembled_msg) > 0) {
		// reassembled_msg is a newly allocated byte buffer, which is guaranteed to
		// be NULL-terminated, so we can cast it to char * directly.
		msg->txt = (char *)reassembled_msg;

	} else {        // this will also trigger when reassembly engine is disabled
		msg->txt = LA_XCALLOC(remaining + 1, sizeof(char));
		if(remaining > 0) {
			memcpy(msg->txt, ptr, remaining);
		}
	}

	if(strlen(msg->txt) > 0) {
		bool decode_apps = true;
		// If reassembly is enabled and is now in progress (ie. the message is not yet complete),
		// then decode_fragments config flag decides whether to decode apps in this message
		// or not.
		if(rtables != NULL && (msg->reasm_status == LA_REASM_IN_PROGRESS ||
					msg->reasm_status == LA_REASM_DUPLICATE)) {
			(void)la_config_get_bool("decode_fragments", &decode_apps);
		}
		if(decode_apps) {
			node->next = la_acars_apps_parse_and_reassemble(msg->reg, msg->label,
					msg->txt, msg_dir, rtables, rx_time);
		}
	}
	goto end;
fail:
	msg->err = true;
end:
	LA_XFREE(buf2);
	return node;
}

la_proto_node *la_acars_parse(uint8_t const *buf, int len, la_msg_dir msg_dir) {
	return la_acars_parse_and_reassemble(buf, len, msg_dir, NULL,
			(struct timeval){ .tv_sec = 0, .tv_usec = 0 });
}

void la_acars_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_acars_msg const *msg = data;
	if(msg->err) {
		LA_ISPRINTF(vstr, indent, "-- Unparseable ACARS message\n");
		return;
	}
	LA_ISPRINTF(vstr, indent, "ACARS%s:\n", msg->crc_ok ? "" : " (warning: CRC error)");
	indent++;

	LA_ISPRINTF(vstr, indent, "Reassembly: %s\n", la_reasm_status_name_get(msg->reasm_status));
	LA_ISPRINTF(vstr, indent, "Reg: %s", msg->reg);
	if(IS_DOWNLINK_BLK(msg->block_id)) {
		la_vstring_append_sprintf(vstr, " Flight: %s\n", msg->flight_id);
	} else {
		la_vstring_append_sprintf(vstr, "%s", "\n");
	}

	LA_ISPRINTF(vstr, indent, "Mode: %1c Label: %s Blk id: %c More: %d Ack: %c",
			msg->mode, msg->label, msg->block_id, !msg->final_block, msg->ack);
	if(IS_DOWNLINK_BLK(msg->block_id)) {
		la_vstring_append_sprintf(vstr, " Msg num: %s%c\n", msg->msg_num, msg->msg_num_seq);
	} else {
		la_vstring_append_sprintf(vstr, "%s", "\n");
	}
	if(msg->sublabel[0] != '\0') {
		LA_ISPRINTF(vstr, indent, "Sublabel: %s", msg->sublabel);
		if(msg->mfi[0] != '\0') {
			la_vstring_append_sprintf(vstr, " MFI: %s", msg->mfi);
		}
		la_vstring_append_sprintf(vstr, "%s", "\n");
	}
	if(msg->txt[0] != '\0') {
		bool prettify_xml = false;
#ifdef WITH_LIBXML2
		(void)la_config_get_bool("prettify_xml", &prettify_xml);
		if(prettify_xml == true) {
			xmlBufferPtr xmlbufptr = NULL;
			if((xmlbufptr = la_prettify_xml(msg->txt)) != NULL) {
				LA_ISPRINTF(vstr, indent, "Message (reformatted):\n");
				la_isprintf_multiline_text(vstr, indent + 1, (char *)xmlbufptr->content);
				xmlBufferFree(xmlbufptr);
			} else {
				// Doesn't look like XML - print it as normal
				prettify_xml = false;
			}
		}
#endif
		if(prettify_xml == false) {
			LA_ISPRINTF(vstr, indent, "Message:\n");
			la_isprintf_multiline_text(vstr, indent+1, msg->txt);
		}
	}
}

void la_acars_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_acars_msg const *msg = data;
	la_json_append_bool(vstr, "err", msg->err);
	if(msg->err) {
		return;
	}
	la_json_append_bool(vstr, "crc_ok", msg->crc_ok);
	la_json_append_bool(vstr, "more", !msg->final_block);
	la_json_append_string(vstr, "reg", msg->reg);
	la_json_append_char(vstr, "mode", msg->mode);
	la_json_append_string(vstr, "label", msg->label);
	la_json_append_char(vstr, "blk_id", msg->block_id);
	la_json_append_char(vstr, "ack", msg->ack);
	if(IS_DOWNLINK_BLK(msg->block_id)) {
		la_json_append_string(vstr, "flight", msg->flight_id);
		la_json_append_string(vstr, "msg_num", msg->msg_num);
		la_json_append_char(vstr, "msg_num_seq", msg->msg_num_seq);
	}
	if(msg->sublabel[0] != '\0') {
		la_json_append_string(vstr, "sublabel", msg->sublabel);
	}
	if(msg->mfi[0] != '\0') {
		la_json_append_string(vstr, "mfi", msg->mfi);
	}
	la_json_append_string(vstr, "msg_text", msg->txt);
}

void la_acars_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	la_acars_msg *msg = data;
	LA_XFREE(msg->txt);
	LA_XFREE(data);
}

la_type_descriptor const la_DEF_acars_message = {
	.format_text = la_acars_format_text,
	.format_json = la_acars_format_json,
	.json_key = "acars",
	.destroy = la_acars_destroy
};

la_proto_node *la_proto_tree_find_acars(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_acars_message);
}
