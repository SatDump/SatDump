/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdbool.h>
#include <ctype.h>                      // isupper(), isdigit()
#include <string.h>                     // strstr()
#include <aero/libacars/libacars.h>          // la_proto_node, la_proto_tree_find_protocol
#include <aero/libacars/arinc.h>             // la_arinc_msg, LA_ARINC_IMI_CNT
#include <aero/libacars/crc.h>               // la_crc16_arinc()
#include <aero/libacars/macros.h>            // la_debug_print()
#include <aero/libacars/vstring.h>           // la_vstring_append_sprintf()
#include <aero/libacars/util.h>              // la_slurp_hexstring()
#include <aero/libacars/json.h>              // la_json_append_*()
#include <aero/libacars/adsc.h>              // la_adsc_parse()
#include <aero/libacars/cpdlc.h>             // la_cpdlc_parse()

#define LA_ARINC_IMI_LEN        3
#define LA_ARINC_AIR_REG_LEN    7
#define LA_ARINC_CRC_LEN        2
#define LA_CRC_ARINC_GOOD       0x1D0Fu

typedef enum {
	ARINC_APP_TYPE_UNKNOWN = 0,
	ARINC_APP_TYPE_CHARACTER,
	ARINC_APP_TYPE_BINARY
} la_arinc_app_type;

typedef struct {
	char const *imi_string;
	la_arinc_imi imi;
} la_arinc_imi_map;

typedef struct {
	la_arinc_app_type app_type;
	char const *description;
	char const *json_key;
} la_arinc_imi_props;

static la_arinc_imi_map const imi_map[LA_ARINC_IMI_CNT] = {
	{ .imi_string = ".AT1", .imi = ARINC_MSG_AT1 },
	{ .imi_string = ".CR1", .imi = ARINC_MSG_CR1 },
	{ .imi_string = ".CC1", .imi = ARINC_MSG_CC1 },
	{ .imi_string = ".DR1", .imi = ARINC_MSG_DR1 },
	{ .imi_string = ".ADS", .imi = ARINC_MSG_ADS },
	{ .imi_string = ".DIS", .imi = ARINC_MSG_DIS },
	{ .imi_string = NULL,   .imi = ARINC_MSG_UNKNOWN }
};

static la_arinc_imi_props const imi_props[LA_ARINC_IMI_CNT] = {
	[ARINC_MSG_UNKNOWN] = {
		.app_type = ARINC_APP_TYPE_UNKNOWN,
		.description = "Unknown message type",
		.json_key = "unknown"
	},
	[ARINC_MSG_AT1] = {
		.app_type = ARINC_APP_TYPE_BINARY,
		.description = "FANS-1/A CPDLC Message",
		.json_key = "fans1a_cpdlc_msg",
	},
	[ARINC_MSG_CR1] = {
		.app_type = ARINC_APP_TYPE_BINARY,
		.description = "FANS-1/A CPDLC Connect Request",
		.json_key = "fans1a_cpdlc_connect_request",
	},
	[ARINC_MSG_CC1] = {
		.app_type = ARINC_APP_TYPE_BINARY,
		.description = "FANS-1/A CPDLC Connect Confirm",
		.json_key = "fans1a_cpdlc_connect_confirm",
	},
	[ARINC_MSG_DR1] = {
		.app_type = ARINC_APP_TYPE_BINARY,
		.description = "FANS-1/A CPDLC Disconnect Request",
		.json_key = "fans1a_cpdlc_disconnect_request",
	},
	[ARINC_MSG_ADS] = {
		.app_type = ARINC_APP_TYPE_BINARY,
		.description = "ADS-C message",
		.json_key = "adsc_msg",
	},
	[ARINC_MSG_DIS] = {
		.app_type = ARINC_APP_TYPE_BINARY,
		.description = "ADS-C disconnect request",
		.json_key = "adsc_disconnect_request",
	}
};

static bool is_numeric_or_uppercase(char const *str, size_t len) {
	if(!str) return false;
	for(size_t i = 0; i < len; i++) {
		if((!isupper(str[i]) && !isdigit(str[i])) || str[i] == '\0') {
			return false;
		}
	}
	return true;
}

static char *guess_arinc_msg_type(char const *txt, la_arinc_msg *msg) {
	if(txt == NULL) {
		return NULL;
	}
	la_assert(msg);
	la_arinc_imi imi = ARINC_MSG_UNKNOWN;

	// H1 messages start with sublabel and MFI - these fields must be stripped
	// before passing message text to this routine. This is done by
	// la_acars_extract_sublabel_and_mfi(). In any case, message text
	// must start with the ground address, optionally preceded by '/'.
	if(txt[0] == '/') {
		txt++;
	}

	char *imi_ptr = NULL;
	for(la_arinc_imi_map const *p = imi_map; ; p++) {
		if(p->imi_string == NULL) break;
		if((imi_ptr = strstr(txt, p->imi_string)) != NULL) {
			imi = p->imi;
			break;
		}
	}
	if(imi == ARINC_MSG_UNKNOWN) {
		la_debug_print(D_INFO, "No known IMI found\n");
		return NULL;
	}
	char *gs_addr = NULL;
	size_t gs_addr_len = 0;
	// Check for seven-character ground address ("AKLCDYA.AT1...")
	if(imi_ptr - txt == 7) {
		if(is_numeric_or_uppercase(imi_ptr - 7, 7)) {
			gs_addr = imi_ptr - 7;
			gs_addr_len = 7;
			goto complete;
		}
		// Check for four-character ground address ("EDYY.AFN...")
	} else if(imi_ptr - txt == 4) {
		if(is_numeric_or_uppercase(imi_ptr - 4, 4)) {
			gs_addr = imi_ptr - 4;
			gs_addr_len = 4;
			goto complete;
		}
	}
	la_debug_print(D_ERROR, "IMI %d found but no GS address\n", imi);
	return NULL;
complete:
	msg->imi = imi;
	memcpy(msg->gs_addr, gs_addr, gs_addr_len);
	msg->gs_addr[gs_addr_len] = '\0';
	// Skip the dot before IMI and point to the start of the CRC-protected part
	return imi_ptr + 1;
}

static bool la_is_crc_ok(char const *text_part, uint8_t const *binary_part, size_t binary_part_len) {
	// compute CRC over IMI+air_reg+binary_part_with_CRC
	size_t buflen = LA_ARINC_IMI_LEN + LA_ARINC_AIR_REG_LEN + binary_part_len;
	uint8_t *buf = LA_XCALLOC(buflen, sizeof(uint8_t));
	memcpy(buf, text_part, LA_ARINC_IMI_LEN + LA_ARINC_AIR_REG_LEN);
	memcpy(buf + LA_ARINC_IMI_LEN + LA_ARINC_AIR_REG_LEN, binary_part, binary_part_len);
	bool result = (la_crc16_arinc(buf, buflen, 0xFFFFu) == LA_CRC_ARINC_GOOD);
	LA_XFREE(buf);
	la_debug_print(D_INFO, "crc_ok? %d\n", result);
	return result;
}

la_proto_node *la_arinc_parse(char const *txt, la_msg_dir msg_dir) {
	if(txt == NULL) {
		return NULL;
	}
	LA_NEW(la_arinc_msg, msg);
	la_proto_node *node = NULL;
	la_proto_node *next_node = NULL;

	char *payload = guess_arinc_msg_type(txt, msg);
	if(payload == NULL) {
		goto cleanup;
	}

	if(imi_props[msg->imi].app_type == ARINC_APP_TYPE_BINARY) {
		size_t payload_len = strlen(payload);
		if(payload_len < LA_ARINC_IMI_LEN + LA_ARINC_AIR_REG_LEN + LA_ARINC_CRC_LEN * 2) {
			la_debug_print(D_ERROR, "payload too short: %zu\n", payload_len);
			goto cleanup;
		}
		memcpy(msg->air_reg, payload + LA_ARINC_IMI_LEN, LA_ARINC_AIR_REG_LEN);
		msg->air_reg[LA_ARINC_AIR_REG_LEN] = '\0';
		la_debug_print(D_INFO, "air_reg: %s\n", msg->air_reg);
		uint8_t *buf = NULL;
		size_t buflen = la_slurp_hexstring(payload + LA_ARINC_IMI_LEN + LA_ARINC_AIR_REG_LEN, &buf);
		msg->crc_ok = la_is_crc_ok(payload, buf, buflen);
		buflen -= LA_ARINC_CRC_LEN; // strip CRC
		switch(msg->imi) {
			case ARINC_MSG_CR1:
			case ARINC_MSG_CC1:
			case ARINC_MSG_DR1:
			case ARINC_MSG_AT1:
				next_node = la_cpdlc_parse(buf, buflen, msg_dir);
				LA_XFREE(buf);
				break;
			case ARINC_MSG_ADS:
			case ARINC_MSG_DIS:
				next_node = la_adsc_parse(buf, buflen, msg_dir, msg->imi);
				LA_XFREE(buf);
				break;
			default:
				break;
		}
	}

	node = la_proto_node_new();
	node->data = msg;
	node->td = &la_DEF_arinc_message;
	node->next = next_node;
	return node;
cleanup:
	LA_XFREE(msg);
	LA_XFREE(node);
	return NULL;
}

void la_arinc_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_arinc_msg const *msg = data;
	LA_ISPRINTF(vstr, indent, "%s:\n", imi_props[msg->imi].description);
	if(!msg->crc_ok) {
		LA_ISPRINTF(vstr, indent + 1, "-- CRC check failed\n");
	}
}

void la_arinc_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_arinc_msg const *msg = data;
	la_json_append_string(vstr, "msg_type", imi_props[msg->imi].json_key);
	if(msg->imi == ARINC_MSG_UNKNOWN) {
		return;
	}
	la_json_append_bool(vstr, "crc_ok", msg->crc_ok);
	la_json_append_string(vstr, "gs_addr", msg->gs_addr);
	la_json_append_string(vstr, "air_addr", msg->air_reg);
}

la_type_descriptor const la_DEF_arinc_message = {
	.format_text = la_arinc_format_text,
	.format_json = la_arinc_format_json,
	.json_key = "arinc622",
	.destroy = NULL
};

la_proto_node *la_proto_tree_find_arinc(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_arinc_message);
}
