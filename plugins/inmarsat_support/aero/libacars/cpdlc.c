/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <aero/libacars/asn1/FANSATCDownlinkMessage.h>   // asn_DEF_FANSATCDownlinkMessage
#include <aero/libacars/asn1/FANSATCUplinkMessage.h>     // asn_DEF_FANSATCUplinkMessage
#include <aero/libacars/asn1/asn_application.h>          // asn_sprintf()
#include <aero/libacars/macros.h>                        // la_assert
#include <aero/libacars/asn1-util.h>                     // la_asn1_decode_as()
#include <aero/libacars/asn1-format-cpdlc.h>             // la_asn1_output_cpdlc_as_*()
#include <aero/libacars/cpdlc.h>                         // la_cpdlc_msg
#include <aero/libacars/libacars.h>                      // la_proto_node, la_config_get_bool, la_proto_tree_find_protocol
#include <aero/libacars/macros.h>                        // la_debug_print
#include <aero/libacars/util.h>                          // LA_XFREE
#include <aero/libacars/vstring.h>                       // la_vstring, la_vstring_append_sprintf()
#include <aero/libacars/json.h>                          // la_json_append_bool()

la_proto_node *la_cpdlc_parse(uint8_t const *buf, int len, la_msg_dir msg_dir) {
	if(buf == NULL)
		return NULL;

	la_proto_node *node = la_proto_node_new();
	LA_NEW(la_cpdlc_msg, msg);
	node->data = msg;
	node->td = &la_DEF_cpdlc_message;

	if(msg_dir == LA_MSG_DIR_GND2AIR) {
		msg->asn_type = &asn_DEF_FANSATCUplinkMessage;
	} else if(msg_dir == LA_MSG_DIR_AIR2GND) {
		msg->asn_type = &asn_DEF_FANSATCDownlinkMessage;
	}
	la_assert(msg->asn_type != NULL);
	if(len == 0) {
		// empty payload is not an error
		la_debug_print(D_INFO, "Empty CPDLC message, decoding skipped\n");
		return node;
	}

	la_debug_print(D_INFO, "Decoding as %s, len: %d\n", msg->asn_type->name, len);
	if(la_asn1_decode_as(msg->asn_type, &msg->data, buf, len) != 0) {
		msg->err = true;
	} else {
		msg->err = false;
	}
	return node;
}

void la_cpdlc_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_cpdlc_msg const *msg = data;
	if(msg->err == true) {
		LA_ISPRINTF(vstr, indent, "-- Unparseable FANS-1/A message\n");
		return;
	}
	if(msg->asn_type != NULL) {
		if(msg->data != NULL) {
			bool dump_asn1 = false;
			(void)la_config_get_bool("dump_asn1", &dump_asn1);
			if(dump_asn1 == true) {
				LA_ISPRINTF(vstr, indent, "ASN.1 dump:\n");
				// asn_fprint does not indent the first line
				LA_ISPRINTF(vstr, indent + 1, "");
				asn_sprintf(vstr, msg->asn_type, msg->data, indent + 2);
				LA_EOL(vstr);
			}
			la_asn1_output_cpdlc_as_text((la_asn1_formatter_params){
					.vstr = vstr,
					.td = msg->asn_type,
					.sptr = msg->data,
					.indent = indent
					});
		} else {
			LA_ISPRINTF(vstr, indent, "-- <empty PDU>\n");
		}
	}
}

void la_cpdlc_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_cpdlc_msg const *msg = data;
	la_json_append_bool(vstr, "err", msg->err);
	if(msg->err == true) {
		return;
	}
	if(msg->asn_type != NULL) {
		if(msg->data != NULL) {
			la_asn1_output_cpdlc_as_json((la_asn1_formatter_params){
					.vstr = vstr,
					.td = msg->asn_type,
					.sptr = msg->data,
					});
		}
	}
}

void la_cpdlc_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	la_cpdlc_msg *msg = data;
	if(msg->asn_type != NULL) {
		msg->asn_type->free_struct(msg->asn_type, msg->data, 0);
	}
	LA_XFREE(data);
}

la_type_descriptor const la_DEF_cpdlc_message = {
	.format_text = la_cpdlc_format_text,
	.format_json = la_cpdlc_format_json,
	.json_key = "cpdlc",
	.destroy = la_cpdlc_destroy
};

la_proto_node *la_proto_tree_find_cpdlc(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_cpdlc_message);
}
