/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_JSON_H
#define LA_JSON_H 1

#include <stdbool.h>
#include <stdint.h>
#include <aero/libacars/vstring.h>           // la_vstring

#ifdef __cplusplus
extern "C" {
#endif

// json.c
void la_json_object_start(la_vstring *vstr, char const *key);
void la_json_object_end(la_vstring *vstr);
void la_json_array_start(la_vstring *vstr, char const *key);
void la_json_array_end(la_vstring *vstr);
void la_json_append_bool(la_vstring *vstr, char const *key, bool val);
void la_json_append_double(la_vstring *vstr, char const *key, double val);
void la_json_append_int64(la_vstring *vstr, char const *key, int64_t val);
void la_json_append_long(la_vstring *vstr, char const *key, long val);
//	__attribute__ ((deprecated("This function is not portable; use la_json_append_int64 instead")));
void la_json_append_char(la_vstring *vstr, char const *key, char val);
void la_json_append_string(la_vstring *vstr, char const *key, char const *val);
void la_json_append_octet_string(la_vstring *vstr, char const *key,
		uint8_t const *buf, size_t len);
void la_json_append_octet_string_as_string(la_vstring *vstr, char const *key,
		uint8_t const *buf, size_t len);
void la_json_start(la_vstring *vstr);
void la_json_end(la_vstring *vstr);

#ifdef __cplusplus
}
#endif

#endif // !LA_JSON_H
