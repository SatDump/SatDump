/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_LIBACARS_H
#define LA_LIBACARS_H 1
#include <stdbool.h>
#include <aero/libacars/version.h>
#include <aero/libacars/vstring.h>       // la_vstring

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	LA_MSG_DIR_UNKNOWN,
	LA_MSG_DIR_GND2AIR,
	LA_MSG_DIR_AIR2GND
} la_msg_dir;

typedef void (la_format_text_func)(la_vstring *vstr, void const *data, int indent);
typedef void (la_format_json_func)(la_vstring *vstr, void const *data);
typedef void (la_destroy_type_f)(void *data);

typedef struct {
	la_format_text_func *format_text;
	la_destroy_type_f *destroy;
	la_format_json_func *format_json;
	char *json_key;
// reserved for future use
	void (*reserved2)(void);
	void (*reserved3)(void);
	void (*reserved4)(void);
	void (*reserved5)(void);
	void (*reserved6)(void);
	void (*reserved7)(void);
	void (*reserved8)(void);
	void (*reserved9)(void);
} la_type_descriptor;

typedef struct la_proto_node la_proto_node;

struct la_proto_node {
	la_type_descriptor const *td;
	void *data;
	la_proto_node *next;
// reserved for future use
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
};

// libacars.c
la_proto_node *la_proto_node_new();
la_vstring *la_proto_tree_format_text(la_vstring *vstr, la_proto_node const *root);
la_vstring *la_proto_tree_format_json(la_vstring *vstr, la_proto_node const *root);
void la_proto_tree_destroy(la_proto_node *root);
la_proto_node *la_proto_tree_find_protocol(la_proto_node *root, la_type_descriptor const *td);

// configuration.c
void la_config_init();
void la_config_destroy();
bool la_config_set_bool(char const *name, bool value);
bool la_config_set_int(char const *name, long int value);
bool la_config_set_double(char const *name, double value);
bool la_config_set_str(char const *name, char const *value);
bool la_config_get_bool(char const *name, bool *result);
bool la_config_get_int(char const *name, long int *result);
bool la_config_get_double(char const *name, double *result);
bool la_config_get_str(char const *name, char **result);
bool la_config_unset(char *name);

// configuration.c

#ifdef __cplusplus
}
#endif

#endif // !LA_LIBACARS_H
