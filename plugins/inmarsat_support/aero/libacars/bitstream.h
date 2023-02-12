/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef _LA_BITSTREAM_H
#define _LA_BITSTREAM_H 1
#include <stdint.h>

typedef struct {
	uint8_t *buf;
	uint32_t start;
	uint32_t end;
	uint32_t len;
} la_bitstream_t;

la_bitstream_t *la_bitstream_init(uint32_t len);
void la_bitstream_destroy(la_bitstream_t *bs);
int la_bitstream_append_msbfirst(la_bitstream_t *bs, uint8_t const *v, uint32_t numbytes, uint32_t numbits);
int la_bitstream_read_word_msbfirst(la_bitstream_t *bs, uint32_t *ret, uint32_t numbits);
#endif // !_LA_BITSTREAM_H
