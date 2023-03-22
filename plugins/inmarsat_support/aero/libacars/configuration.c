/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdbool.h>
#include <string.h>                 // strdup
#include <aero/libacars/macros.h>        // la_assert
#include <aero/libacars/util.h>          // LA_XCALLOC, LA_XFREE
#include <aero/libacars/hash.h>          // la_hash

typedef enum {
	LA_CONFVAR_UNKNOWN = 0,
	LA_CONFVAR_BOOLEAN = 1,
	LA_CONFVAR_INTEGER = 2,
	LA_CONFVAR_DOUBLE  = 3,
	LA_CONFVAR_STRING  = 4
} la_config_item_type;
#define LA_CONFIG_ITEM_TYPE_MAX 4

typedef union {
	bool _bool;
	long int _int;
	double _double;
	char *_str;
} la_config_item_value;

typedef struct {
	la_config_item_type type;
	la_config_item_value value;
} la_config_item;

typedef struct {
	char const *name;
	la_config_item value;
} la_config_option;

#define LA_CONFIG_ITEM_VALUE(_member, _value) \
{ ._member = (_value)}
#define LA_CONFIG_ITEM(_type, _member, _value) \
{ .type = _type, .value = LA_CONFIG_ITEM_VALUE(_member, _value) }
#define LA_CONFIG_SETTING_BOOLEAN(_name, _value) \
{ .name = _name, .value = LA_CONFIG_ITEM(LA_CONFVAR_BOOLEAN, _bool, _value) }
#define LA_CONFIG_SETTING_DOUBLE(_name, _value) \
{ .name = _name, .value = LA_CONFIG_ITEM(LA_CONFVAR_DOUBLE, _double, _value) }
#define LA_CONFIG_SETTING_INTEGER(_name, _value) \
{ .name = _name, .value = LA_CONFIG_ITEM(LA_CONFVAR_INTEGER, _int, _value) }
#define LA_CONFIG_SETTING_STRING(_name, _value) \
{ .name = _name, .value = LA_CONFIG_ITEM(LA_CONFVAR_STRING, _str, _value) }

static const la_config_option config_defaults[] =
#include <aero/libacars/config_defaults.h>

#define CONFIG_DEFAULTS_COUNT (sizeof(config_defaults) / sizeof(la_config_option))

static la_hash *config = NULL;
static void la_config_init();

// Sets the given config option to the given type/value.
// Returns true if the option already existed in the config, false otherwise.
static bool la_config_option_set(char const *name, la_config_item const item) {
	la_assert(name != NULL);
	la_assert(item.type >= 0);
	la_assert(item.type <= LA_CONFIG_ITEM_TYPE_MAX);

	if(config == NULL) {
		la_config_init();
	}
	LA_NEW(la_config_item, new_item);
	char *new_name = strdup(name);
	new_item->type = item.type;
	new_item->value = item.value;
	return la_hash_insert(config, new_name, new_item);
}

bool la_config_set_bool(char const *name, bool value) {
	if(name == NULL) {
		return false;
	}
	(void)la_config_option_set(name, (la_config_item){
			.type = LA_CONFVAR_BOOLEAN,
			.value = { ._bool = value }
			});
	return true;
}

bool la_config_set_int(char const *name, long int value) {
	if(name == NULL) {
		return false;
	}
	(void)la_config_option_set(name, (la_config_item){
			.type = LA_CONFVAR_INTEGER,
			.value = { ._int = value }
			});
	return true;
}

bool la_config_set_double(char const *name, double value) {
	if(name == NULL) {
		return false;
	}
	(void)la_config_option_set(name, (la_config_item){
			.type = LA_CONFVAR_DOUBLE,
			.value = { ._double = value }
			});
	return true;
}

bool la_config_set_str(char const *name, char const *value) {
	if(name == NULL) {
		return false;
	}
	(void)la_config_option_set(name, (la_config_item){
			.type = LA_CONFVAR_STRING,
			.value = { ._str = value ? strdup(value) : NULL }
			});
	return true;
}

static la_config_item *la_config_option_get(char const *name) {
	if(name == NULL) {
		return false;
	}
	if(config == NULL) {
		la_config_init();
	}
	return la_hash_lookup(config, name);
}

bool la_config_get_bool(char const *name, bool *result) {
	la_config_item *item = la_config_option_get(name);
	if(item && item->type == LA_CONFVAR_BOOLEAN) {
		*result = item->value._bool;
		return true;
	}
	return false;
}

bool la_config_get_int(char const *name, long int *result) {
	la_config_item *item = la_config_option_get(name);
	if(item && item->type == LA_CONFVAR_INTEGER) {
		*result = item->value._int;
		return true;
	}
	return false;
}

bool la_config_get_double(char const *name, double *result) {
	la_config_item *item = la_config_option_get(name);
	if(item && item->type == LA_CONFVAR_DOUBLE) {
		*result = item->value._double;
		return true;
	}
	return false;
}

bool la_config_get_str(char const *name, char **result) {
	la_config_item *item = la_config_option_get(name);
	if(item && item->type == LA_CONFVAR_STRING) {
		*result = item->value._str;
		return true;
	}
	return false;
}

bool la_config_unset(char *name) {
	if(config == NULL) {
		la_config_init();
	}
	return la_hash_remove(config, name);
}

static void la_config_item_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	la_config_item *item = data;
	if(item->type == LA_CONFVAR_STRING) {
		LA_XFREE(item->value._str);
	}
	LA_XFREE(item);
}

void la_config_destroy() {
	la_hash_destroy(config);
}

void la_config_init() {
	if(config != NULL) {
		la_config_destroy();
	}
	config = la_hash_new(la_hash_key_str, la_hash_compare_keys_str,
			la_simple_free, la_config_item_destroy);
	la_assert(config != NULL);
	for(size_t i = 0; i < CONFIG_DEFAULTS_COUNT; i++) {
		la_config_option const *opt = config_defaults + i;
		if(opt->value.type == LA_CONFVAR_STRING) {
			(void)la_config_set_str(opt->name, opt->value.value._str);
		} else {
			(void)la_config_option_set(opt->name, opt->value);
		}
	}
}
