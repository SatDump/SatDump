/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdio.h>              // fprintf
#include <stdint.h>
#include <stdlib.h>             // calloc, realloc, free
#include <string.h>             // strerror, strlen, strdup, strnlen, strspn, strpbrk
#include <time.h>               // struct tm
#include <limits.h>             // CHAR_BIT
#include <errno.h>              // errno
#ifdef WIN32
#else
#include <unistd.h>             // _exit
#endif
#include "config.h"             // HAVE_STRSEP, WITH_LIBXML2
#ifdef WITH_LIBXML2
#include <libxml/parser.h>      // xmlParseDoc
#include <libxml/tree.h>        // xmlBuffer.*, xmlNodeDump, xmlDocGetRootElement, xmlFreeDoc
#include <libxml/xmlerror.h>    // initGenericErrorDefaultFunc, xmlGenericError
#endif
#include <aero/libacars/macros.h>    // la_debug_print()
#include <aero/libacars/util.h>

void *la_xcalloc(size_t nmemb, size_t size, char const *file, int line, char const *func) {
	void *ptr = calloc(nmemb, size);
	if(ptr == NULL) {
		fprintf(stderr, "%s:%d: %s(): calloc(%zu, %zu) failed: %s\n",
				file, line, func, nmemb, size, strerror(errno));
		_exit(1);
	}
	return ptr;
}

void *la_xrealloc(void *ptr, size_t size, char const *file, int line, char const *func) {
	ptr = realloc(ptr, size);
	if(ptr == NULL) {
		fprintf(stderr, "%s:%d: %s(): realloc(%zu) failed: %s\n",
				file, line, func, size, strerror(errno));
		_exit(1);
	}
	return ptr;
}

size_t la_slurp_hexstring(char* string, uint8_t **buf) {
	if(string == NULL)
		return 0;
	size_t slen = strlen(string);
	if(slen & 1)
		slen--;
	size_t dlen = slen / 2;
	if(dlen == 0)
		return 0;
	*buf = LA_XCALLOC(dlen, sizeof(uint8_t));

	for(size_t i = 0; i < slen; i++) {
		char c = string[i];
		int value = 0;
		if(c >= '0' && c <= '9') {
			value = (c - '0');
		} else if (c >= 'A' && c <= 'F') {
			value = (10 + (c - 'A'));
		} else if (c >= 'a' && c <= 'f') {
			value = (10 + (c - 'a'));
		} else {
			la_debug_print(D_ERROR, "stopped at invalid char %u at pos %zu\n", c, i);
			return i/2;
		}
		(*buf)[(i/2)] |= value << (((i + 1) % 2) * 4);
	}
	return dlen;
}

char *la_hexdump(uint8_t *data, size_t len) {
	static char const hex[] = "0123456789abcdef";
	if(data == NULL) return strdup("<undef>");
	if(len == 0) return strdup("<none>");

	size_t rows = len / 16;
	if((len & 0xf) != 0) {
		rows++;
	}
	size_t rowlen = 16 * 2 + 16;            // 32 hex digits + 16 spaces per row
	rowlen += 16;                           // ASCII characters per row
	rowlen += 10;                           // extra space for separators
	size_t alloc_size = rows * rowlen + 1;  // terminating NULL
	char *buf = LA_XCALLOC(alloc_size, sizeof(char));
	char *ptr = buf;
	size_t i = 0, j = 0;
	while(i < len) {
		for(j = i; j < i + 16; j++) {
			if(j < len) {
				*ptr++ = hex[((data[j] >> 4) & 0xf)];
				*ptr++ = hex[data[j] & 0xf];
			} else {
				*ptr++ = ' ';
				*ptr++ = ' ';
			}
			*ptr++ = ' ';
			if(j == i + 7) {
				*ptr++ = ' ';
			}
		}
		*ptr++ = ' ';
		*ptr++ = '|';
		for(j = i; j < i + 16; j++) {
			if(j < len) {
				if(data[j] < 32 || data[j] > 126) {
					*ptr++ = '.';
				} else {
					*ptr++ = data[j];
				}
			} else {
				*ptr++ = ' ';
			}
			if(j == i + 7) {
				*ptr++ = ' ';
			}
		}
		*ptr++ = '|';
		*ptr++ = '\n';
		i += 16;
	}
	return buf;
}

int la_strntouint16_t(char const *txt, int charcnt) {
	if(txt == NULL ||
			charcnt < 1 ||
			charcnt > 9 ||      // prevent overflowing int
			strnlen(txt, charcnt) < (size_t)charcnt)
	{
		return -1;
	}
	int ret = 0;
	int base = 1;
	int j;
	for(int i = 0; i < charcnt; i++) {
		j = charcnt - 1 - i;
		if(txt[j] < '0' || txt[j] > '9') {
			return -2;
		}
		ret += (txt[j] - '0') * base;
		base *= 10;
	}
	return ret;
}

// returns the length of the string ignoring the terminating
// newline characters
size_t chomped_strlen(char const *s) {
	char *p = strchr(s, '\0');
	size_t ret = p - s;
	while(--p >= s) {
		if(*p != '\n' && *p != '\r') {
			break;
		}
		ret--;
	}
	return ret;
}

// parse and perform basic sanitization of timestamp
// in YYMMDDHHMMSS format.
// Do not use strptime() - it's not available on WIN32
// Do not use sscanf() - it's too liberal on the input
// (we do not want whitespaces between fields, for example)
char *la_simple_strptime(char const *s, struct tm *t) {
	if(strspn(s, "0123456789") < 12) {
		return NULL;
	}
	t->tm_year = ATOI2(s[0],  s[1]) + 100;
	t->tm_mon  = ATOI2(s[2],  s[3]) - 1;
	t->tm_mday = ATOI2(s[4],  s[5]);
	t->tm_hour = ATOI2(s[6],  s[7]);
	t->tm_min  = ATOI2(s[8],  s[9]);
	t->tm_sec  = ATOI2(s[10], s[11]);
	t->tm_isdst = -1;
	if(t->tm_mon > 11 || t->tm_mday > 31 || t->tm_hour > 23 || t->tm_min > 59 || t->tm_sec > 59) {
		return NULL;
	}
	return (char *)s + 12;
}

la_octet_string *la_octet_string_new(void *buf, size_t len) {
	LA_NEW(la_octet_string, ostring);
	ostring->buf = buf;
	ostring->len = len;
	return ostring;
}

void la_octet_string_destroy(void *ostring_ptr) {
	if(ostring_ptr == NULL) {
		return;
	}
	la_octet_string *ostring = ostring_ptr;
	LA_XFREE(ostring->buf);
	LA_XFREE(ostring);
}

#ifndef HAVE_STRSEP
char *la_strsep(char **stringp, char const *delim) {
	char *start = *stringp;
	char *p;

	p = (start != NULL) ? strpbrk(start, delim) : NULL;
	if (p == NULL) {
		*stringp = NULL;
	} else {
		*p = '\0';
		*stringp = p + 1;
	}
	return start;
}
#endif

#ifdef WITH_LIBXML2
void la_xml_errfunc_noop(void * ctx, char const *msg, ...) {
	(void)ctx;
	(void)msg;
}

xmlBufferPtr la_prettify_xml(char const *buf) {
	if(buf == NULL) {
		return NULL;
	}
	// Disables printing XML parser errors to stderr by setting error handler to noop.
	// Can't do this once in library constructor, because this is a per-thread setting.
	if(xmlGenericError != la_xml_errfunc_noop) {
		xmlGenericErrorFunc errfuncptr = la_xml_errfunc_noop;
		initGenericErrorDefaultFunc(&errfuncptr);
	}
	xmlDocPtr doc = xmlParseDoc((uint8_t const *)buf);
	if(doc == NULL) {
		return NULL;
	}
	xmlBufferPtr outbufptr = xmlBufferCreate();
	int result_len = xmlNodeDump(outbufptr, doc, xmlDocGetRootElement(doc), 0, 1);
	xmlFreeDoc(doc);
	if(result_len > 0) {
		return outbufptr;
	}
	xmlBufferFree(outbufptr);
	return NULL;
}
#endif

uint32_t la_reverse(uint32_t v, int numbits) {
	uint32_t r = v;                         // r will be reversed bits of v; first get LSB of v
	int s = sizeof(v) * CHAR_BIT - 1;       // extra shift needed at end

	for (v >>= 1; v; v >>= 1) {
		r <<= 1;
		r |= v & 1;
		s--;
	}
	r <<= s;                                // shift when v's highest bits are zero
	r >>= 32 - numbits;
	return r;
}
