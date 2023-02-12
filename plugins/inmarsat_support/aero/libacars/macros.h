/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_MACROS_H
#define LA_MACROS_H 1

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __GNUC__
#define LA_LIKELY(x) (__builtin_expect(!!(x),1))
#define LA_UNLIKELY(x) (__builtin_expect(!!(x),0))
#else
#define LA_LIKELY(x) (x)
#define LA_UNLIKELY(x) (x)
#endif

#ifdef __GNUC__
#define LA_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define LA_PRETTY_FUNCTION ""
#endif

#define la_assert_se(expr) \
	do { \
		if (LA_UNLIKELY(!(expr))) { \
			fprintf(stderr, "Assertion '%s' failed at %s:%u, function %s(). Aborting.\n", \
					#expr , __FILE__, __LINE__, LA_PRETTY_FUNCTION); \
			abort(); \
		} \
	} while (false)

#define la_nop() do {} while (false)

#ifdef NDEBUG
#define la_assert(expr) la_nop()
#else
#define la_assert(expr) la_assert_se(expr)
#endif

#define LA_MAX(a, b) ((a) > (b) ? (a) : (b))
#define LA_MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef DEBUG
#include <stdint.h>

extern uint32_t Debug;

// debug levels
#define D_NONE 0
#define D_ERROR 1
#define D_INFO 2
#define D_VERBOSE 3

#define la_debug_print(level, fmt, ...) \
	do { \
		if(Debug >= (level)) { \
			fprintf(stderr, "%s(): " fmt, __func__, ##__VA_ARGS__); \
		} \
	} while (0)

#define la_debug_print_buf_hex(level, buf, len, fmt, ...) \
	do { \
		if(Debug >= (level)) { \
			fprintf(stderr, "%s(): " fmt, __func__, ##__VA_ARGS__); \
			fprintf(stderr, "%s(): ", __func__); \
			for(int zz = 0; zz < (len); zz++) { \
				fprintf(stderr, "%02x ", buf[zz]); \
				if(zz && (zz+1) % 32 == 0) fprintf(stderr, "\n%s(): ", __func__); \
			} \
			fprintf(stderr, "\n"); \
		} \
	} while(0)
#else
#define la_debug_print(level, fmt, ...) la_nop()
#define la_debug_print_buf_hex(level, buf, len, fmt, ...) la_nop()
#endif

#define LA_NEW(type, x) type *(x) = LA_XCALLOC(1, sizeof(type))
#define LA_UNUSED(x) (void)(x)

#endif // !LA_MACROS_H
