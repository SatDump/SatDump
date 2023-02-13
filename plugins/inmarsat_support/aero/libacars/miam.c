/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>                 /* calloc() */
#include <string.h>                 /* strchr(), strlen(), strncmp(), strcmp() */
#ifdef WIN32
#else
#include <sys/time.h>               /* struct timeval */
#endif
#include <aero/libacars/macros.h>        /* la_assert() */
#include <aero/libacars/libacars.h>      /* la_proto_node, la_type_descriptor */
#include <aero/libacars/vstring.h>       /* la_vstring */
#include <aero/libacars/json.h>          /* la_json_append_*() */
#include <aero/libacars/util.h>          /* la_strntouint16_t(), la_simple_strptime() */
#include <aero/libacars/dict.h>          /* la_dict, la_dict_search() */
#include <aero/libacars/reassembly.h>
#include <aero/libacars/miam-core.h>     /* la_miam_core_pdu_parse(), la_miam_core_format_*() */
#include <aero/libacars/miam.h>

// Clean up stale reassembly entries every 20 File Segment frames.
#define LA_MIAM_FILE_REASM_TABLE_CLEANUP_INTERVAL 20

typedef struct {
	char fid_char;
	la_miam_frame_id frame_id;
} la_miam_frame_id_map;

static la_miam_frame_id_map const frame_id_map[LA_MIAM_FRAME_ID_CNT] = {
	{ .fid_char= 'T',  .frame_id = LA_MIAM_FID_SINGLE_TRANSFER },
	{ .fid_char= 'F',  .frame_id = LA_MIAM_FID_FILE_TRANSFER_REQ },
	{ .fid_char= 'K',  .frame_id = LA_MIAM_FID_FILE_TRANSFER_ACCEPT },
	{ .fid_char= 'S',  .frame_id = LA_MIAM_FID_FILE_SEGMENT },
	{ .fid_char= 'A',  .frame_id = LA_MIAM_FID_FILE_TRANSFER_ABORT },
	{ .fid_char= 'Y',  .frame_id = LA_MIAM_FID_XOFF_IND },
	{ .fid_char= 'X',  .frame_id = LA_MIAM_FID_XON_IND },
	{ .fid_char= '\0', .frame_id = LA_MIAM_FID_UNKNOWN },
};

static la_dict const la_miam_frame_names[] = {
	{ .id = LA_MIAM_FID_SINGLE_TRANSFER, .val = "Single Transfer" },
	{ .id = LA_MIAM_FID_FILE_TRANSFER_REQ, .val = "File Transfer Request" },
	{ .id = LA_MIAM_FID_FILE_TRANSFER_ACCEPT, .val = "File Transfer Accept" },
	{ .id = LA_MIAM_FID_FILE_SEGMENT, .val = "File Segment" },
	{ .id = LA_MIAM_FID_FILE_TRANSFER_ABORT, .val = "File Transfer Abort" },
	{ .id = LA_MIAM_FID_XOFF_IND, .val = "File Transfer Pause" },
	{ .id = LA_MIAM_FID_XON_IND, .val = "File Transfer Resume" },
	{ .id = 0, .val = NULL }
};

/********************************************************************************
 * MIAM File Transfer reassembly constants and callbacks
 ********************************************************************************/

// MIAM File Transfer reassembly timeout.
// XXX: shall we respect message validity field from File Transfer Request frames,
// which often sets this timeout to many hours?
static struct timeval const la_miam_file_reasm_timeout = {
	.tv_sec = 900,
	.tv_usec = 0
};

typedef struct {
	char *reg;
	uint16_t file_id;
} la_miam_file_key;

static uint32_t la_miam_file_key_hash(void const *key) {
	la_miam_file_key const *k = key;
	uint32_t h = la_hash_string(k->reg, LA_HASH_INIT);
	h += k->file_id;
	return h;
}

static bool la_miam_file_key_compare(void const *key1, void const *key2) {
	la_miam_file_key const *k1 = key1;
	la_miam_file_key const *k2 = key2;
	return (!strcmp(k1->reg, k2->reg) &&
			(k1->file_id == k2->file_id));
}

static void *la_miam_file_key_get(void const *msg_info) {
	la_assert(msg_info != NULL);
	la_miam_file_key const *msg = msg_info;
	LA_NEW(la_miam_file_key, key);
	key->reg = strdup(msg->reg);
	key->file_id = msg->file_id;
	la_debug_print(D_INFO, "ALLOC KEY %s %d\n", key->reg, key->file_id);
	return (void *)key;
}

static void *la_miam_file_tmp_key_get(void const *msg_info) {
	la_assert(msg_info != NULL);
	la_miam_file_key const *msg = msg_info;
	LA_NEW(la_miam_file_key, key);
	key->reg = (char *)msg->reg;
	key->file_id = msg->file_id;
	return (void *)key;
}

static void la_miam_file_key_destroy(void *ptr) {
	if(ptr == NULL) {
		return;
	}
	la_miam_file_key *key = ptr;
	la_debug_print(D_INFO, "DESTROY KEY %s %d\n", key->reg, key->file_id);
	LA_XFREE(key->reg);
	LA_XFREE(key);
}

static la_reasm_table_funcs miam_file_reasm_funcs = {
	.get_key = la_miam_file_key_get,
	.get_tmp_key = la_miam_file_tmp_key_get,
	.hash_key = la_miam_file_key_hash,
	.compare_keys = la_miam_file_key_compare,
	.destroy_key = la_miam_file_key_destroy
};

/********************************************************************************
 * MIAM frame parsers
 ********************************************************************************/

static la_proto_node *la_miam_single_transfer_parse(char const *txt) {
	void *next = la_miam_core_pdu_parse(txt);
	if(next == NULL) {
		return NULL;
	}
	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_single_transfer_message;
	node->data = NULL;
	node->next = next;
	return node;
}

static la_proto_node *la_miam_file_transfer_request_parse(char const *reg, char const *txt,
		la_reasm_ctx *rtables, struct timeval rx_time) {
	la_assert(txt != NULL);

	la_miam_file_transfer_request_msg *msg = NULL;
	if(chomped_strlen(txt) != 21) {
		goto hdr_error;
	}

	msg = LA_XCALLOC(1, sizeof(la_miam_file_transfer_request_msg));
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->file_id = (uint16_t)i;
	txt += 3;

	if((i = la_strntouint16_t(txt, 6)) < 0) {
		goto hdr_error;
	}
	msg->file_size = (size_t)i;
	txt += 6;

	la_debug_print(D_INFO, "file_id: %u file_size: %zu\n", msg->file_id, msg->file_size);
	char const *ptr = la_simple_strptime(txt, &msg->validity_time);
	if(ptr == NULL) {
		goto hdr_error;
	}

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_file_transfer_request_message;
	node->data = msg;
	node->next = NULL;

	// If reassembly engine is enabled, then we have to create a reassembly table
	// entry for this transfer now, because File Segment frames do not contain
	// all necessary state information (file size, in particular).
	if(rtables != NULL && reg != NULL) {
		la_reasm_table *miam_file_table = la_reasm_table_lookup(rtables,
				&la_DEF_miam_file_segment_message);
		if(miam_file_table == NULL) {
			miam_file_table = la_reasm_table_new(rtables,
					&la_DEF_miam_file_segment_message, miam_file_reasm_funcs,
					LA_MIAM_FILE_REASM_TABLE_CLEANUP_INTERVAL);
		}
		// Add the initial empty fragment to the table.
		// Can't use msg as msg_info directly, because we will be adding subsequent
		// fragments from la_miam_file_segment_parse(), where the type of msg is
		// different.
		msg->reasm_status = la_reasm_fragment_add(miam_file_table,
				&(la_reasm_fragment_info){
					.msg_info = &(la_miam_file_key){
						.reg = (char *)reg,
						.file_id = msg->file_id
					},
					.msg_data = NULL,       // payload will start in the next segment
					.msg_data_len = 0,
					.total_pdu_len = msg->file_size,
					.seq_num = 0,           // in sequence with file segment numbers, which go from 1
					.seq_num_first = 0,
					.seq_num_wrap = SEQ_WRAP_NONE,
					.is_final_fragment = false,
					.rx_time = rx_time,
					.reasm_timeout = la_miam_file_reasm_timeout
				});
	}
	return node;
hdr_error:
	la_debug_print(D_VERBOSE, "Not a file_transfer_request header\n");
	LA_XFREE(msg);
	return NULL;
}

static la_proto_node *la_miam_file_transfer_accept_parse(char const *txt) {
	la_assert(txt != NULL);

	la_miam_file_transfer_accept_msg *msg = NULL;
	if(chomped_strlen(txt) != 10) {
		goto hdr_error;
	}
	msg = LA_XCALLOC(1, sizeof(la_miam_file_transfer_accept_msg));
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->file_id = (uint16_t)i;
	txt += 3;

	if(txt[0] >= '0' && txt[0] <= '9') {
		msg->segment_size = txt[0] - '0';
	} else if(txt[0] >= 'A' && txt[0] <= 'F') {
		msg->segment_size = txt[0] - 'A' + 10;
	} else {
		goto hdr_error;
	}
	txt++;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->onground_segment_tempo = (uint16_t)i;
	txt += 3;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->inflight_segment_tempo = (uint16_t)i;
	txt += 3;

	la_debug_print(D_INFO, "file_id: %u seg_size: %u onground_tempo: %u inflight_tempo: %u\n",
			msg->file_id, msg->segment_size, msg->onground_segment_tempo, msg->inflight_segment_tempo);

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_file_transfer_accept_message;
	node->data = msg;
	node->next = NULL;
	return node;
hdr_error:
	la_debug_print(D_VERBOSE, "Not a file_transfer_accept header\n");
	LA_XFREE(msg);
	return NULL;
}

static la_proto_node *la_miam_file_segment_parse(char const *reg, char const *txt,
		la_reasm_ctx *rtables, struct timeval rx_time) {
	la_assert(txt != NULL);
	LA_NEW(la_miam_file_segment_msg, msg);
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->file_id = (uint16_t)i;
	txt += 3;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->segment_id = (uint16_t)i;
	txt += 3;

	la_debug_print(D_INFO, "file_id: %u segment_id: %u\n", msg->file_id, msg->segment_id);

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_file_segment_message;
	node->data = msg;
	node->next = NULL;

	// Can't use msg as msg_info directly in la_reasm_fragment_info, because the
	// initial fragment is added by la_miam_file_transfer_request_parse(), where
	// the type of msg is different.
	la_miam_file_key msg_key = {
		.reg = (char *)reg,
		.file_id = msg->file_id
	};
	la_reasm_table *miam_file_table = NULL;

	if(rtables != NULL && reg != NULL) {
		miam_file_table = la_reasm_table_lookup(rtables, &la_DEF_miam_file_segment_message);
		if(miam_file_table == NULL) {
			miam_file_table = la_reasm_table_new(rtables,
					&la_DEF_miam_file_segment_message, miam_file_reasm_funcs,
					LA_MIAM_FILE_REASM_TABLE_CLEANUP_INTERVAL);
		}
		// Add the fragment to the table.
		msg->reasm_status = la_reasm_fragment_add(miam_file_table,
				&(la_reasm_fragment_info){
					.msg_info = &msg_key,
					.msg_data = (uint8_t *)txt,
					.msg_data_len = strlen(txt),
					.total_pdu_len = 0,         // already set in 1st fragment
					.seq_num = msg->segment_id,
					.seq_num_first = 0,
					.seq_num_wrap = SEQ_WRAP_NONE,
					.is_final_fragment = false, // not used here
					.rx_time = rx_time,
					.reasm_timeout = la_miam_file_reasm_timeout
				});
	}

	uint8_t *reassembled_msg = NULL;
	if(msg->reasm_status == LA_REASM_COMPLETE &&
			la_reasm_payload_get(miam_file_table, &msg_key, &reassembled_msg) > 0) {
		// reassembled_msg is a newly allocated byte buffer, which is guaranteed to
		// be NULL-terminated, so we can cast it to char * directly.
		// Store the pointer to it in msg struct for freeing it later.
		txt = msg->txt = (char *)reassembled_msg;

	}

	bool decode_payload = true;
	// If reassembly is enabled and is now in progress (ie. the message is not yet complete),
	// then decode_fragments config flag decides whether to decode apps in this message
	// or not.
	if(rtables != NULL && (msg->reasm_status == LA_REASM_IN_PROGRESS ||
				msg->reasm_status == LA_REASM_DUPLICATE)) {
		(void)la_config_get_bool("decode_fragments", &decode_payload);
	}
	if(decode_payload) {
		node->next = la_miam_core_pdu_parse(txt);
	}

	return node;
hdr_error:
	la_debug_print(D_VERBOSE, "Not a file_segment header\n");
	LA_XFREE(msg);
	return NULL;
}

static void la_miam_file_segment_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	la_miam_file_segment_msg *msg = data;
	LA_XFREE(msg->txt);
	LA_XFREE(msg);
}

static la_proto_node *la_miam_file_transfer_abort_parse(char const *txt) {
	la_assert(txt != NULL);

	la_miam_file_transfer_abort_msg *msg = NULL;
	if(chomped_strlen(txt) != 4) {
		goto hdr_error;
	}
	msg = LA_XCALLOC(1, sizeof(la_miam_file_transfer_abort_msg));
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->file_id = (uint16_t)i;
	txt += 3;

	if(txt[0] >= '0' && txt[0] <= '9') {
		msg->reason = txt[0] - '0';
	} else {
		goto hdr_error;
	}
	txt++;

	la_debug_print(D_INFO, "file_id: %u reason: %u\n", msg->file_id, msg->reason);

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_file_transfer_abort_message;
	node->data = msg;
	node->next = NULL;
	return node;
hdr_error:
	la_debug_print(D_VERBOSE, "Not a file_transfer_abort header\n");
	LA_XFREE(msg);
	return NULL;
}

static la_proto_node *la_miam_xoff_ind_parse(char const *txt) {
	la_assert(txt != NULL);

	la_miam_xoff_ind_msg *msg = NULL;
	if(chomped_strlen(txt) != 3) {
		goto hdr_error;
	}
	msg = LA_XCALLOC(1, sizeof(la_miam_xoff_ind_msg));
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		if(strncmp(txt, "FFF", 3) == 0) {
			i = 0xFFF;
		} else {
			goto hdr_error;
		}
	}
	msg->file_id = (uint16_t)i;
	txt += 3;
	la_debug_print(D_INFO, "file_id: %u\n", msg->file_id);

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_xoff_ind_message;
	node->data = msg;
	node->next = NULL;
	return node;
hdr_error:
	la_debug_print(D_VERBOSE, "Not a xoff_ind header\n");
	LA_XFREE(msg);
	return NULL;
}

static la_proto_node *la_miam_xon_ind_parse(char const *txt) {
	la_assert(txt != NULL);

	la_miam_xon_ind_msg *msg = NULL;
	if(chomped_strlen(txt) != 9) {
		goto hdr_error;
	}
	msg = LA_XCALLOC(1, sizeof(la_miam_xon_ind_msg));
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		if(strncmp(txt, "FFF", 3) == 0) {
			i = 0xFFF;
		} else {
			goto hdr_error;
		}
	}
	msg->file_id = (uint16_t)i;
	txt += 3;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->onground_segment_tempo = (uint16_t)i;
	txt += 3;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->inflight_segment_tempo = (uint16_t)i;
	txt += 3;

	la_debug_print(D_INFO, "file_id: %u onground_tempo: %u inflight_tempo: %u\n",
			msg->file_id, msg->onground_segment_tempo, msg->inflight_segment_tempo);

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_xon_ind_message;
	node->data = msg;
	node->next = NULL;
	return node;
hdr_error:
	la_debug_print(D_VERBOSE, "Not a xon_ind header\n");
	LA_XFREE(msg);
	return NULL;
}

la_proto_node *la_miam_parse_and_reassemble(char const *reg, char const *txt,
		la_reasm_ctx *rtables, struct timeval rx_time) {
	if(txt == NULL) {
		return NULL;
	}
	size_t len = strlen(txt);

	// First character identifies the ACARS CF frame
	if(len == 0) {
		return NULL;
	}
	la_miam_frame_id fid = LA_MIAM_FID_UNKNOWN;     // safe default
	for(int i = 0; i < LA_MIAM_FRAME_ID_CNT; i++) {
		if(txt[0] == frame_id_map[i].fid_char) {
			fid = frame_id_map[i].frame_id;
			la_debug_print(D_INFO, "txt[0]: %c frame_id: %d\n", txt[0], fid);
			break;
		}
	}
	if(fid == LA_MIAM_FID_UNKNOWN) {
		la_debug_print(D_VERBOSE, "not a MIAM message (unknown ACARS CF frame)\n");
		return NULL;
	}
	txt++; len--;
	la_proto_node *next_node = NULL;
	switch(fid) {
		case LA_MIAM_FID_SINGLE_TRANSFER:
			next_node = la_miam_single_transfer_parse(txt);
			break;
		case LA_MIAM_FID_FILE_TRANSFER_REQ:
			next_node = la_miam_file_transfer_request_parse(reg, txt, rtables, rx_time);
			break;
		case LA_MIAM_FID_FILE_TRANSFER_ACCEPT:
			next_node = la_miam_file_transfer_accept_parse(txt);
			break;
		case LA_MIAM_FID_FILE_SEGMENT:
			next_node = la_miam_file_segment_parse(reg, txt, rtables, rx_time);
			break;
		case LA_MIAM_FID_FILE_TRANSFER_ABORT:
			next_node = la_miam_file_transfer_abort_parse(txt);
			break;
		case LA_MIAM_FID_XOFF_IND:
			next_node = la_miam_xoff_ind_parse(txt);
			break;
		case LA_MIAM_FID_XON_IND:
			next_node = la_miam_xon_ind_parse(txt);
			break;
		default:
			break;
	}
	if(next_node == NULL) {
		return NULL;
	}
	LA_NEW(la_miam_msg, msg);
	msg->frame_id = fid;
	la_proto_node *node = la_proto_node_new();
	node->data = msg;
	node->td = &la_DEF_miam_message;
	node->next = next_node;
	return node;
}

la_proto_node *la_miam_parse(char const *txt) {
	return la_miam_parse_and_reassemble(NULL, txt, NULL,
			(struct timeval){ .tv_sec = 0, .tv_usec = 0});
}

static void la_miam_single_transfer_format_text(la_vstring *vstr, void const *data, int indent) {
	la_miam_core_format_text(vstr, data, indent);
}

static void la_miam_single_transfer_format_json(la_vstring *vstr, void const *data) {
	la_miam_core_format_json(vstr, data);
}

static void la_miam_file_transfer_request_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_miam_file_transfer_request_msg const *msg = data;
	indent++;
	LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	LA_ISPRINTF(vstr, indent, "File size: %zu bytes\n", msg->file_size);
	struct tm const *t = &msg->validity_time;
	LA_ISPRINTF(vstr, indent, "Complete until: %d-%02d-%02d %02d:%02d:%02d\n",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec
			);
	LA_ISPRINTF(vstr, indent, "Reassembly: %s\n", la_reasm_status_name_get(msg->reasm_status));
}

static void la_miam_file_transfer_request_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_miam_file_transfer_request_msg const *msg = data;
	la_json_append_int64(vstr, "file_id", msg->file_id);
	la_json_append_int64(vstr, "file_size", msg->file_size);
	struct tm const *t = &msg->validity_time;
	la_json_object_start(vstr, "complete_until_datetime");
	la_json_object_start(vstr, "date");
	la_json_append_int64(vstr, "year", t->tm_year + 1900);
	la_json_append_int64(vstr, "month", t->tm_mon + 1);
	la_json_append_int64(vstr, "day", t->tm_mday);
	la_json_object_end(vstr);
	la_json_object_start(vstr, "time");
	la_json_append_int64(vstr, "hour", t->tm_hour);
	la_json_append_int64(vstr, "minute", t->tm_min);
	la_json_append_int64(vstr, "second", t->tm_sec);
	la_json_object_end(vstr);
	la_json_object_end(vstr);
}

static void la_miam_file_transfer_accept_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_miam_file_transfer_accept_msg const *msg = data;
	indent++;
	LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	LA_ISPRINTF(vstr, indent, "Segment size: %u\n", msg->segment_size);
	LA_ISPRINTF(vstr, indent, "On-ground segment temporization: %u sec\n", msg->onground_segment_tempo);
	LA_ISPRINTF(vstr, indent, "In-flight segment temporization: %u sec\n", msg->inflight_segment_tempo);
}

static void la_miam_file_transfer_accept_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_miam_file_transfer_accept_msg const *msg = data;
	la_json_append_int64(vstr, "file_id", msg->file_id);
	la_json_append_int64(vstr, "segment_size", msg->segment_size);
	la_json_append_int64(vstr, "on_ground_seg_temp_secs", msg->onground_segment_tempo);
	la_json_append_int64(vstr, "in_flight_seg_temp_secs", msg->inflight_segment_tempo);
}

static void la_miam_file_segment_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_miam_file_segment_msg const *msg = data;
	indent++;
	LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	LA_ISPRINTF(vstr, indent, "Segment ID: %u\n", msg->segment_id);
	LA_ISPRINTF(vstr, indent, "Reassembly: %s\n", la_reasm_status_name_get(msg->reasm_status));
}

static void la_miam_file_segment_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_miam_file_segment_msg const *msg = data;
	la_json_append_int64(vstr, "file_id", msg->file_id);
	la_json_append_int64(vstr, "segment_id", msg->segment_id);
}

static void la_miam_file_transfer_abort_format_text(la_vstring *vstr, void const *data, int indent) {
	static la_dict const abort_reasons[] = {
		{ .id = 0, .val = "File transfer request refused by receiver" },
		{ .id = 1, .val = "File segment out of context" },
		{ .id = 2, .val = "File transfer stopped by sender" },
		{ .id = 3, .val = "File transfer stopped by receiver" },
		{ .id = 4, .val = "File segment transmission failed" },
		{ .id = 0, .val = NULL }
	};
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_miam_file_transfer_abort_msg const *msg = data;
	indent++;
	LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	char *descr = la_dict_search(abort_reasons, msg->reason);
	LA_ISPRINTF(vstr, indent, "Reason: %u (%s)\n", msg->reason,
			(descr != NULL ? descr : "unknown"));
}

static void la_miam_file_transfer_abort_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_miam_file_transfer_abort_msg const *msg = data;
	la_json_append_int64(vstr, "file_id", msg->file_id);
	la_json_append_int64(vstr, "reason", msg->reason);
}

static void la_miam_xoff_ind_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_miam_xoff_ind_msg const *msg = data;
	indent++;
	if(msg->file_id == 0xFFF) {
		LA_ISPRINTF(vstr, indent, "File ID: 0xFFF (all)\n");
	} else {
		LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	}
}

static void la_miam_xoff_ind_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_miam_xoff_ind_msg const *msg = data;
	la_json_append_bool(vstr, "all_files", msg->file_id == 0xFFF);
	if(msg->file_id != 0xFFF) {
		la_json_append_int64(vstr, "file_id", msg->file_id);
	}
}

static void la_miam_xon_ind_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_miam_xon_ind_msg const *msg = data;
	indent++;
	if(msg->file_id == 0xFFF) {
		LA_ISPRINTF(vstr, indent, "File ID: 0xFFF (all)\n");
	} else {
		LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	}
	LA_ISPRINTF(vstr, indent, "On-ground segment temporization: %u sec\n", msg->onground_segment_tempo);
	LA_ISPRINTF(vstr, indent, "In-flight segment temporization: %u sec\n", msg->inflight_segment_tempo);
}

static void la_miam_xon_ind_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_miam_xon_ind_msg const *msg = data;
	la_json_append_bool(vstr, "all_files", msg->file_id == 0xFFF);
	if(msg->file_id != 0xFFF) {
		la_json_append_int64(vstr, "file_id", msg->file_id);
	}
	la_json_append_int64(vstr, "on_ground_seg_temp_secs", msg->onground_segment_tempo);
	la_json_append_int64(vstr, "in_flight_seg_temp_secs", msg->inflight_segment_tempo);
}

void la_miam_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_miam_msg const *msg = data;
	char *frame_name = la_dict_search(la_miam_frame_names, msg->frame_id);
	la_assert(frame_name != NULL);
	LA_ISPRINTF(vstr, indent, "MIAM:\n");
	LA_ISPRINTF(vstr, indent+1, "%s:\n", frame_name);
}

void la_miam_format_json(la_vstring *vstr, void const *data) {
	LA_UNUSED(vstr);
	LA_UNUSED(data);
	// NOOP
}

la_type_descriptor const la_DEF_miam_message = {
	.format_text = la_miam_format_text,
	.format_json = la_miam_format_json,
	.json_key = "miam",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_single_transfer_message = {
	.format_text = la_miam_single_transfer_format_text,
	.format_json = la_miam_single_transfer_format_json,
	.json_key = "single_transfer",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_file_transfer_request_message = {
	.format_text = la_miam_file_transfer_request_format_text,
	.format_json = la_miam_file_transfer_request_format_json,
	.json_key = "file_transfer_request",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_file_transfer_accept_message = {
	.format_text = la_miam_file_transfer_accept_format_text,
	.format_json = la_miam_file_transfer_accept_format_json,
	.json_key = "file_transfer_accept",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_file_segment_message = {
	.format_text = la_miam_file_segment_format_text,
	.format_json = la_miam_file_segment_format_json,
	.json_key = "file_segment",
	.destroy = la_miam_file_segment_destroy
};

la_type_descriptor const la_DEF_miam_file_transfer_abort_message = {
	.format_text = la_miam_file_transfer_abort_format_text,
	.format_json = la_miam_file_transfer_abort_format_json,
	.json_key = "file_transfer_abort",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_xoff_ind_message = {
	.format_text = la_miam_xoff_ind_format_text,
	.format_json = la_miam_xoff_ind_format_json,
	.json_key = "file_xoff_ind",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_xon_ind_message = {
	.format_text = la_miam_xon_ind_format_text,
	.format_json = la_miam_xon_ind_format_json,
	.json_key = "file_xon_ind",
	.destroy = NULL
};

la_proto_node *la_proto_tree_find_miam(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_miam_message);
}
