/**
 * @file encode.h
 *
 * @section LICENSE
 * Copyright 2012 - 2016
 *
 * Mathis Rosenhauer, Moritz Hanke, Joerg Behrens
 * Deutsches Klimarechenzentrum GmbH
 * Bundesstr. 45a
 * 20146 Hamburg Germany
 *
 * Luis Kornblueh
 * Max-Planck-Institut fuer Meteorologie
 * Bundesstr. 53
 * 20146 Hamburg
 * Germany
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 *
 * Adaptive Entropy Encoder
 * Based on CCSDS documents 121.0-B-2 and 120.0-G-3
 *
 */

#ifndef ENCODE_H
#define ENCODE_H 1

#include "config.h"

#if HAVE_STDINT_H
#  include <stdint.h>
#endif

#define M_CONTINUE 1
#define M_EXIT 0
#define MIN(a, b) (((a) < (b))? (a): (b))

/* Maximum CDS length in bytes: 5 bits ID, 64 * 32 bits samples, 7
 * bits carry from previous CDS */
#define CDSLEN ((5 + 64 * 32 + 7 + 7) / 8)

/* Marker for Remainder Of Segment condition in zero block encoding */
#define ROS -1

struct aec_stream;

struct internal_state {
    int (*mode)(struct aec_stream *);
    uint32_t (*get_sample)(struct aec_stream *);
    void (*get_rsi)(struct aec_stream *);
    void (*preprocess)(struct aec_stream *);

    /* bit length of code option identification key */
    int id_len;

    /* minimum integer for preprocessing */
    uint32_t xmin;

    /* maximum integer for preprocessing */
    uint32_t xmax;

    uint32_t i;

    /* RSI blocks of preprocessed input */
    uint32_t *data_pp;

    /* RSI blocks of input */
    uint32_t *data_raw;

    /* remaining blocks in buffer */
    int blocks_avail;

    /* blocks encoded so far in RSI */
    int blocks_dispensed;

    /* current (preprocessed) input block */
    uint32_t *block;

    /* reference sample interval in byte */
    uint32_t rsi_len;

    /* current Coded Data Set output */
    uint8_t *cds;

    /* buffer for one CDS (only used if strm->next_out cannot hold
     * full CDS) */
    uint8_t cds_buf[CDSLEN];

    /* cds points to strm->next_out (1) or cds_buf (0) */
    int direct_out;

    /* Free bits (LSB) in output buffer or accumulator */
    int bits;

    /* length of reference sample in current block i.e. 0 or 1
     * depending on whether the block has a reference sample or not */
    int ref;

    /* reference sample stored here for performance reasons */
    uint32_t ref_sample;

    /* current zero block has a reference sample */
    int zero_ref;

    /* reference sample of zero block */
    uint32_t zero_ref_sample;

    /* storage size of samples in bytes */
    uint32_t bytes_per_sample;

    /* number of contiguous zero blocks */
    int zero_blocks;

    /* 1 if this is the first non-zero block after one or more zero
     * blocks */
    int block_nonzero;

    /* splitting position */
    int k;

    /* maximum number for k depending on id_len */
    int kmax;

    /* flush option copied from argument */
    int flush;

    /* 1 if flushing was successful */
    int flushed;

    /* length of uncompressed CDS */
    uint32_t uncomp_len;
};

#endif /* ENCODE_H */
