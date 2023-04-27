/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_ARINC_H
#define LA_ARINC_H 1

#include <stdint.h>
#include <aero/libacars/libacars.h>      // la_type_descriptor, la_proto_node
#include <aero/libacars/vstring.h>       // la_vstring

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ARINC_MSG_UNKNOWN = 0,
	ARINC_MSG_CR1,
	ARINC_MSG_CC1,
	ARINC_MSG_DR1,
	ARINC_MSG_AT1,
	ARINC_MSG_ADS,
	ARINC_MSG_DIS
} la_arinc_imi;
#define LA_ARINC_IMI_CNT 7

typedef struct {
	char gs_addr[8];
	char air_reg[8];
	la_arinc_imi imi;
	bool crc_ok;
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_arinc_msg;

la_proto_node *la_arinc_parse(char const *txt, la_msg_dir msg_dir);
void la_arinc_format_text(la_vstring *vstr, void const *data, int indent);
void la_arinc_format_json(la_vstring *vstr, void const *data);
extern la_type_descriptor const la_DEF_arinc_message;
la_proto_node *la_proto_tree_find_arinc(la_proto_node *root);

#ifdef __cplusplus
}
#endif

#endif // !LA_ARINC_H
