/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_MEDIA_ADV_H
#define LA_MEDIA_ADV_H 1

#include <stdint.h>
#include <aero/libacars/libacars.h>      // la_type_descriptor, la_proto_node
#include <aero/libacars/vstring.h>       // la_vstring

#ifdef __cplusplus
extern "C" {
#endif

#define LA_MEDIA_ADV_LINK_TYPE_CNT 8

typedef struct {
	bool err;
	uint8_t version;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	char state;
	char current_link;
	la_vstring *available_links;
	char *text;
// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
} la_media_adv_msg;

la_proto_node *la_media_adv_parse(char const *txt);
void la_media_adv_format_text(la_vstring *vstr, void const *data, int indent);
void la_media_adv_format_json(la_vstring *vstr, void const *data);
extern la_type_descriptor const la_DEF_media_adv_message;
la_proto_node *la_proto_tree_find_media_adv(la_proto_node *root);

#ifdef __cplusplus
}
#endif

#endif // !LA_MEDIA_ADV_H
