/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_VSTRING_H
#define LA_VSTRING_H 1

#include <stddef.h>         // size_t
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#ifdef __MINGW32__
#define LA_GCC_PRINTF_ATTR(a,b) __attribute__ ((format (gnu_printf, a, b)))
#else
#define LA_GCC_PRINTF_ATTR(a,b) __attribute__ ((format (printf, a, b)))
#endif
#else
#define LA_GCC_PRINTF_ATTR(a,b)
#endif

// la_vstring_append_sprintf with variable indentation
#define LA_ISPRINTF(vstr, i, f, ...) la_vstring_append_sprintf(vstr, "%*s" f, i, "", ##__VA_ARGS__)

#define LA_EOL(x) la_vstring_append_sprintf((x), "%s", "\n")

typedef struct {
	char *str;              // string buffer pointer
	size_t len;             // current length of the string (excl. '\0')
	size_t allocated_size;  // current allocated buffer size (ie. max len = allocated_len - 1)
} la_vstring;

la_vstring *la_vstring_new();
void la_vstring_destroy(la_vstring *vstr, bool destroy_buffer);
void la_vstring_append_sprintf(la_vstring *vstr, char const *fmt, ...) LA_GCC_PRINTF_ATTR(2, 3);
void la_vstring_append_buffer(la_vstring *vstr, void const *buffer, size_t size);
void la_isprintf_multiline_text(la_vstring *vstr, int indent, char const *txt);

#ifdef __cplusplus
}
#endif

#endif // !LA_VSTRING_H
