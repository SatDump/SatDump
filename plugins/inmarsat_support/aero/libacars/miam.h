/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_MIAM_H
#define LA_MIAM_H 1

#include <stdint.h>
#include <time.h>                   // struct tm
#include <aero/libacars/libacars.h>      // la_type_descriptor, la_proto_node
#include <aero/libacars/vstring.h>       // la_vstring
#include <aero/libacars/reassembly.h>    // la_reasm_ctx, la_reasm_status reasm_status

#ifdef __cplusplus
extern "C" {
#endif

// MIAM frame identifier
typedef enum {
	LA_MIAM_FID_UNKNOWN = 0,
	LA_MIAM_FID_SINGLE_TRANSFER,
	LA_MIAM_FID_FILE_TRANSFER_REQ,
	LA_MIAM_FID_FILE_TRANSFER_ACCEPT,
	LA_MIAM_FID_FILE_SEGMENT,
	LA_MIAM_FID_FILE_TRANSFER_ABORT,
	LA_MIAM_FID_XOFF_IND,
	LA_MIAM_FID_XON_IND
} la_miam_frame_id;
#define LA_MIAM_FRAME_ID_CNT 8

// MIAM ACARS CF frame
typedef struct {
	la_miam_frame_id frame_id;
// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_miam_msg;

// MIAM File Transfer Request
typedef struct {
	size_t file_size;
	uint16_t file_id;
	struct tm validity_time;
	la_reasm_status reasm_status;
// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_miam_file_transfer_request_msg;

// MIAM File Transfer Accept
typedef struct {
	uint16_t file_id;
	uint16_t segment_size;
	uint16_t onground_segment_tempo;
	uint16_t inflight_segment_tempo;
// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_miam_file_transfer_accept_msg;

// MIAM File Segment
typedef struct {
	char *txt;
	uint16_t file_id;
	uint16_t segment_id;
	la_reasm_status reasm_status;
// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_miam_file_segment_msg;

// MIAM File Transfer Abort
typedef struct {
	uint16_t file_id;
	uint16_t reason;
// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_miam_file_transfer_abort_msg;

// MIAM XOFF IND
typedef struct {
	uint16_t file_id;	// 0-127 or 0xFFF = pause all transfers
// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_miam_xoff_ind_msg;

// MIAM XON IND
typedef struct {
	uint16_t file_id;	// 0-127 or 0xFFF = resume all transfers
	uint16_t onground_segment_tempo;
	uint16_t inflight_segment_tempo;
// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_miam_xon_ind_msg;

la_proto_node *la_miam_parse(char const *txt);
la_proto_node *la_miam_parse_and_reassemble(char const *reg, char const *txt,
		la_reasm_ctx *rtables, struct timeval rx_time);
void la_miam_format_text(la_vstring *vstr, void const *data, int indent);
void la_miam_format_json(la_vstring *vstr, void const *data);

extern la_type_descriptor const la_DEF_miam_message;
extern la_type_descriptor const la_DEF_miam_single_transfer_message;
extern la_type_descriptor const la_DEF_miam_file_transfer_request_message;
extern la_type_descriptor const la_DEF_miam_file_transfer_accept_message;
extern la_type_descriptor const la_DEF_miam_file_segment_message;
extern la_type_descriptor const la_DEF_miam_file_transfer_abort_message;
extern la_type_descriptor const la_DEF_miam_xoff_ind_message;
extern la_type_descriptor const la_DEF_miam_xon_ind_message;
la_proto_node *la_proto_tree_find_miam(la_proto_node *root);

#ifdef __cplusplus
}
#endif

#endif // !LA_MIAM_H
