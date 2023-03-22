/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <inttypes.h>                   // PRI* macros
#include <stdbool.h>
#include <stdint.h>
#include <string.h>                     // strlen()
#include <aero/libacars/macros.h>            // la_assert()
#include <aero/libacars/vstring.h>           // la_vstring
#include <aero/libacars/util.h>              // LA_XCALLOC(), LA_XFREE()

static void la_json_trim_comma(la_vstring *vstr) {
	la_assert(vstr != NULL);
	size_t len = vstr->len;
	if(len > 0 && vstr->str[len-1] == ',') {
		vstr->str[len-1] = '\0';
		vstr->len--;
	}
}

// Note: this function escapes raw ASCII bytes. It does not assume that the input
// is valid Unicode, so it does not attempt to guess Unicode codepoints from
// the input byte string.
static char *la_json_escapechars(uint8_t const *buf, size_t len) {
	la_assert(buf != NULL);
	size_t new_len = len;
	if(len > 0) {
		for(size_t i = 0; i < len; i++) {
			la_debug_print(D_INFO, "char: 0x%x\n", buf[i]);
			if(buf[i] < ' ' || buf[i] > 0x7e || buf[i] == '\"' || buf[i] == '\\') {
				la_debug_print(D_INFO, "char: 0x%x needs escaping\n", buf[i]);
				new_len += 5;           // to fit the \uNNNN form
			}
		}
	}
	char *out = LA_XCALLOC(new_len + 1, sizeof(uint8_t));   // Add space for NULL terminator
	if(new_len == len) {
		memcpy(out, buf, len);
		goto end;
	}
	char *outptr = out;
	for(size_t i = 0; i < len; i++) {
		if(buf[i] < ' ' || buf[i] > 0x7e || buf[i] == '\"' || buf[i] == '\\') {
			*outptr++ = '\\';
			switch(buf[i]) {
				case '\b':
					*outptr++ = 'b';
					break;
				case '\t':
					*outptr++ = 't';
					break;
				case '\n':
					*outptr++ = 'n';
					break;
				case '\f':
					*outptr++ = 'f';
					break;
				case '\r':
					*outptr++ = 'r';
					break;
				case '\"':
					*outptr++ = '\"';
					break;
				case '\\':
					*outptr++ = '\\';
					break;
				default:
					sprintf(outptr, "u%04x", buf[i]);
					outptr += 5;
			}
		} else {
			*outptr++ = buf[i];
		}
	}
end:
	out[new_len] = '\0';
	return out;
}

static inline void la_json_print_key(la_vstring *vstr, char const *key) {
	la_assert(vstr != NULL);
	if(key != NULL && key[0] != '\0') {
		// Warning: no character escaping is performed here. For libacars this is fine
		// as all key names are static. Escaping them would add unnecessary overhead.
		la_vstring_append_sprintf(vstr, "\"%s\":", key);
	}
}

void la_json_append_bool(la_vstring *vstr, char const *key, bool val) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%s,", (val == true ? "true" : "false"));
}

void la_json_append_double(la_vstring *vstr, char const *key, double val) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%f,", val);
}

void la_json_append_int64(la_vstring *vstr, char const *key, int64_t val) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%" PRId64 ",", val);
}

void la_json_append_long(la_vstring *vstr, char const *key, long val) {
	la_json_append_int64(vstr, key, val);
}

void la_json_append_octet_string_as_string(la_vstring *vstr, char const *key,
		uint8_t const *buf, size_t len) {
	la_assert(vstr != NULL);
	if(buf == NULL) {
		return;
	}
	la_json_print_key(vstr, key);
	char *escaped = la_json_escapechars(buf, len);
	la_vstring_append_sprintf(vstr, "\"%s\",", escaped);
	LA_XFREE(escaped);
}

// Note: this function does not handle NULL characters inside the string.
// Whenever this might be an issue, la_json_append_octet_string_as_string shall be used instead.
void la_json_append_string(la_vstring *vstr, char const *key, char const *val) {
	la_json_append_octet_string_as_string(vstr, key, (uint8_t const *)val, strlen(val));
}

void la_json_append_char(la_vstring *vstr, char const *key, char val) {
	la_json_append_octet_string_as_string(vstr, key, (uint8_t * const)&val, 1);
}

void la_json_object_start(la_vstring *vstr, char const *key) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%s", "{");
}

void la_json_object_end(la_vstring *vstr) {
	la_assert(vstr != NULL);
	la_json_trim_comma(vstr);
	la_vstring_append_sprintf(vstr, "%s", "},");
}

void la_json_array_start(la_vstring *vstr, char const *key) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%s", "[");
}

void la_json_array_end(la_vstring *vstr) {
	la_assert(vstr != NULL);
	la_json_trim_comma(vstr);
	la_vstring_append_sprintf(vstr, "%s", "],");
}

void la_json_append_octet_string(la_vstring *vstr, char const *key,
		uint8_t const *buf, size_t len) {
	la_assert(vstr != NULL);
	la_json_array_start(vstr, key);
	if(buf != NULL && len > 0) {
		for(size_t i = 0; i < len; i++) {
			la_json_append_long(vstr, NULL, buf[i]);
		}
	}
	la_json_array_end(vstr);
}

void la_json_start(la_vstring *vstr) {
	la_assert(vstr != NULL);
	la_json_object_start(vstr, NULL);
}

void la_json_end(la_vstring *vstr) {
	la_assert(vstr != NULL);
	la_json_object_end(vstr);
	la_json_trim_comma(vstr);
}
