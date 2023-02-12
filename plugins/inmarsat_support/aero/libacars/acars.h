/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_ACARS_H
#define LA_ACARS_H 1
#include <stdint.h>
#include <stdbool.h>
#include <aero/libacars/libacars.h>              // la_proto_node, la_type_descriptor
#include <aero/libacars/vstring.h>               // la_vstring
#include <aero/libacars/reassembly.h>            // la_reasm_ctx, la_reasm_status

#ifdef __cplusplus
extern "C" {
#endif

// Supported radio link types
// These are the allowed values of "acars_bearer" configuration option

#define LA_ACARS_BEARER_INVALID   0         // do not use
#define LA_ACARS_BEARER_VHF       1
#define LA_ACARS_BEARER_HFDL      2
#define LA_ACARS_BEARER_SATCOM    3

#define LA_ACARS_BEARER_MIN       0
#define LA_ACARS_BEARER_MAX       3

typedef struct {
	bool crc_ok;
	bool err;
	bool final_block;
	char mode;
	char reg[8];
	char ack;
	char label[3];
	char sublabel[3];
	char mfi[3];
	char block_id;
	char msg_num[4];
	char msg_num_seq;
	char flight_id[7];
	la_reasm_status reasm_status;
	char *txt;
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
	void (*reserved4)(void);
	void (*reserved5)(void);
	void (*reserved6)(void);
	void (*reserved7)(void);
	void (*reserved8)(void);
	void (*reserved9)(void);
} la_acars_msg;

// acars.c
extern la_type_descriptor const la_DEF_acars_message;
la_proto_node *la_acars_decode_apps(char const *label,
		char const *txt, la_msg_dir msg_dir);
la_proto_node *la_acars_apps_parse_and_reassemble(char const *reg,
		char const *label, char const *txt, la_msg_dir msg_dir,
		la_reasm_ctx *rtables, struct timeval rx_time);
la_proto_node *la_acars_parse_and_reassemble(uint8_t const *buf, int len, la_msg_dir msg_dir,
		la_reasm_ctx *rtables, struct timeval rx_time);
la_proto_node *la_acars_parse(uint8_t const *buf, int len, la_msg_dir msg_dir);
int la_acars_extract_sublabel_and_mfi(char const *label, la_msg_dir msg_dir,
		char const *txt, int len, char *sublabel, char *mfi);
void la_acars_format_text(la_vstring *vstr, void const *data, int indent);
void la_acars_format_json(la_vstring *vstr, void const *data);
la_proto_node *la_proto_tree_find_acars(la_proto_node *root);
#ifdef __cplusplus
}
#endif
#endif // !LA_ACARS_H
