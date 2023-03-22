/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdbool.h>
#include <ctype.h>                  // isupper(), isdigit()
#include <string.h>                 // strchr(), strcpy(), strncpy(), strlen()
#include <aero/libacars/libacars.h>      // la_proto_node, la_proto_tree_find_protocol()
#include <aero/libacars/media-adv.h>     // la_arinc_msg, LA_ARINC_IMI_CNT
#include <aero/libacars/macros.h>        // la_assert()
#include <aero/libacars/vstring.h>       // la_vstring, la_vstring_append_sprintf(), LA_ISPRINTF()
#include <aero/libacars/json.h>          // la_json_*()
#include <aero/libacars/util.h>          // LA_XCALLOC(), ATOI2()

typedef struct {
	char code;
	char const *description;
} la_media_adv_link_type_map;

static la_media_adv_link_type_map const link_type_map[LA_MEDIA_ADV_LINK_TYPE_CNT] = {
	{ .code = 'V', .description = "VHF ACARS" },
	{ .code = 'S', .description = "Default SATCOM" },
	{ .code = 'H', .description = "HF" },
	{ .code = 'G', .description = "Global Star Satcom" },
	{ .code = 'C', .description = "ICO Satcom" },
	{ .code = '2', .description = "VDL2" },
	{ .code = 'X', .description = "Inmarsat Aero H/H+/I/L" },
	{ .code = 'I', .description = "Iridium Satcom" }
};

static char const *get_link_description(char code) {
	for(int k = 0; k < LA_MEDIA_ADV_LINK_TYPE_CNT; k++) {
		if(link_type_map[k].code == code) {
			return link_type_map[k].description;
		}
	}
	return NULL;
}

bool is_valid_link(char link) {
	return strchr("VSHGC2XI", link) != NULL;
}

la_proto_node *la_media_adv_parse(char const *txt) {
	if(txt == NULL) {
		return NULL;
	}

	LA_NEW(la_media_adv_msg, msg);
	msg->err = true;

	la_proto_node *node = la_proto_node_new();
	node->data = msg;
	node->td = &la_DEF_media_adv_message;
	node->next = NULL;

	if(strlen(txt) < 10) {
		goto end;
	}
	msg->version = txt[0] - '0';
	if(msg->version != 0) {
		goto end;
	}
	msg->state = txt[1];
	if(msg->state != 'E' && msg->state != 'L') {
		goto end;
	}
	msg->current_link = txt[2];
	if(!is_valid_link(msg->current_link)) {
		goto end;
	}
	for(size_t i = 3; i < 9; i++) {
		if(!isdigit(txt[i])) {
			goto end;
		}
	}
	msg->hour = ATOI2(txt[3], txt[4]);
	msg->minute = ATOI2(txt[5], txt[6]);
	msg->second = ATOI2(txt[7], txt[8]);
	if(msg->hour > 23 || msg->minute > 59 || msg->second > 59) {
		goto end;
	}
	txt += 9;
	msg->available_links = la_vstring_new();
	// Copy all link until / character or end of string
	for(; *txt != '/' && *txt != '\0'; txt++) {
		if(is_valid_link(*txt)) {
			la_vstring_append_buffer(msg->available_links, txt, 1);
		} else {
			goto end;
		}
	}
	if(txt[0] == '/' && txt[1] != '\0') {
		msg->text = strdup(txt + 1);
	}
	msg->err = false;
end:
	return node;
}

void la_media_adv_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_media_adv_msg const *msg = data;

	if(msg->err == true) {
		LA_ISPRINTF(vstr, indent, "-- Unparseable Media Advisory message\n");
		return;
	}

	LA_ISPRINTF(vstr, indent, "Media Advisory, version %d:\n", msg->version);
	indent++;

	LA_ISPRINTF(vstr, indent, "Link %s %s at %02d:%02d:%02d UTC\n",
			get_link_description(msg->current_link),
			(msg->state == 'E') ? "established" : "lost",
			msg->hour, msg->minute, msg->second
			);

	LA_ISPRINTF(vstr, indent, "Available links: ");
	size_t count = strlen(msg->available_links->str);
	for(size_t i = 0; i < count; i++) {
		char const *link = get_link_description(msg->available_links->str[i]);
		if(i == count - 1) {
			la_vstring_append_sprintf(vstr, "%s\n", link);
		} else {
			la_vstring_append_sprintf(vstr, "%s, ", link);
		}
	}

	if(msg->text != NULL && msg->text[0] != '\0') {
		LA_ISPRINTF(vstr, indent, "Text: %s\n", msg->text);
	}
}

void la_media_adv_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_media_adv_msg const *msg = data;

	la_json_append_bool(vstr, "err", msg->err);
	if(msg->err == true) {
		return;
	}
	la_json_append_int64(vstr, "version", msg->version);
	la_json_object_start(vstr, "current_link");
	la_json_append_char(vstr, "code", msg->current_link);
	la_json_append_string(vstr, "descr", get_link_description(msg->current_link));
	la_json_append_bool(vstr, "established", (msg->state == 'E') ? true : false);
	la_json_object_start(vstr, "time");
	la_json_append_int64(vstr, "hour", msg->hour);
	la_json_append_int64(vstr, "min", msg->minute);
	la_json_append_int64(vstr, "sec", msg->second);
	la_json_object_end(vstr);
	la_json_object_end(vstr);

	la_json_array_start(vstr, "links_avail");
	size_t count = strlen(msg->available_links->str);
	for(size_t i = 0; i < count; i++) {
		la_json_object_start(vstr, NULL);
		la_json_append_char(vstr, "code", msg->available_links->str[i]);
		la_json_append_string(vstr, "descr", get_link_description(msg->available_links->str[i]));
		la_json_object_end(vstr);
	}
	la_json_array_end(vstr);
	if(msg->text != NULL && msg->text[0] != '\0') {
		la_json_append_string(vstr, "text", msg->text);
	}
}

void la_media_adv_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	la_media_adv_msg *msg = data;
	la_vstring_destroy(msg->available_links, true);
	LA_XFREE(msg->text);
	LA_XFREE(msg);
}

la_type_descriptor const la_DEF_media_adv_message = {
	.format_text = la_media_adv_format_text,
	.format_json = la_media_adv_format_json,
	.json_key = "media-adv",
	.destroy = la_media_adv_destroy
};

la_proto_node *la_proto_tree_find_media_adv(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_media_adv_message);
}
