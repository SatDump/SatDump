/* -*- c++ -*- */
/*
 * Copyright 2018 Ahmet Inan, Ron Economos.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * This file has been adapted from https://github.com/drmpeg/gr-dvbs2rx
 */

#pragma once

#include "bch/bose_chaudhuri_hocquenghem_decoder.hh"
#include "bch/galois_field.hh"
#include "common/codings/dvb-s2/dvbs2.h"
#include <cstdint>
#include <bitset>

#define MAX_BCH_PARITY_BITS 192

namespace dvbs2
{
    class BBFrameBCH
    {
    private:
        unsigned int kbch;
        unsigned int nbch;
        unsigned int bch_code;
        unsigned int frame;

        // Decode
        typedef CODE::GaloisField<16, 0b10000000000101101, uint16_t> GF_NORMAL;
        typedef CODE::GaloisField<15, 0b1000000000101101, uint16_t> GF_MEDIUM;
        typedef CODE::GaloisField<14, 0b100000000101011, uint16_t> GF_SHORT;
        typedef CODE::BoseChaudhuriHocquenghemDecoder<24, 1, 65343, GF_NORMAL> BCH_NORMAL_12;
        typedef CODE::BoseChaudhuriHocquenghemDecoder<20, 1, 65375, GF_NORMAL> BCH_NORMAL_10;
        typedef CODE::BoseChaudhuriHocquenghemDecoder<16, 1, 65407, GF_NORMAL> BCH_NORMAL_8;
        typedef CODE::BoseChaudhuriHocquenghemDecoder<24, 1, 32587, GF_MEDIUM> BCH_MEDIUM_12;
        typedef CODE::BoseChaudhuriHocquenghemDecoder<24, 1, 16215, GF_SHORT> BCH_SHORT_12;
        GF_NORMAL *instance_n;
        GF_MEDIUM *instance_m;
        GF_SHORT *instance_s;
        BCH_NORMAL_12 *decode_n_12;
        BCH_NORMAL_10 *decode_n_10;
        BCH_NORMAL_8 *decode_n_8;
        BCH_MEDIUM_12 *decode_m_12;
        BCH_SHORT_12 *decode_s_12;
        uint8_t *code;
        uint8_t *parity;

        // Encode
        uint8_t encode_buffer[64800];

        std::bitset<MAX_BCH_PARITY_BITS> crc_table[256];
        std::bitset<MAX_BCH_PARITY_BITS> crc_medium_table[16];
        unsigned int num_parity_bits;
        std::bitset<MAX_BCH_PARITY_BITS> polynome;

        void calculate_crc_table();
        void calculate_medium_crc_table();
        int poly_mult(const int *, int, const int *, int, int *);
        void bch_poly_build_tables(void);

    public:
        BBFrameBCH(dvbs2_framesize_t framesize, dvbs2_code_rate_t rate);
        ~BBFrameBCH();

        int dataSize()
        {
            return kbch;
        }

        int decode(uint8_t *frame);
        int encode(uint8_t *frame);
    };
}