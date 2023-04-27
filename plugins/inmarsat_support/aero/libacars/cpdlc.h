/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_CPDLC_H
#define LA_CPDLC_H 1

#include <stdbool.h>
#include <stdint.h>
#include <aero/libacars/libacars.h>              // la_type_descriptor, la_proto_node
#include <aero/libacars/vstring.h>               // la_vstring
#include <aero/libacars/asn1/asn_application.h>  // asn_TYPE_descriptor_t

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	asn_TYPE_descriptor_t *asn_type;
	void *data;
	bool err;
	// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_cpdlc_msg;

// cpdlc.c
extern la_type_descriptor const la_DEF_cpdlc_message;
la_proto_node *la_cpdlc_parse(uint8_t const *buf, int len, la_msg_dir msg_dir);
void la_cpdlc_format_text(la_vstring *vstr, void const *data, int indent);
void la_cpdlc_format_json(la_vstring *vstr, void const *data);
void la_cpdlc_destroy(void *data);
la_proto_node *la_proto_tree_find_cpdlc(la_proto_node *root);

#ifdef __cplusplus
}
#endif

#endif // !LA_CPDLC_H
