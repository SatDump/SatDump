/* -*- c++ -*- */
/*
 * Copyright 2018 Ron Economos.
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
 * This file has been adapted from https://github.com/drmpeg/gr-dvbs2rx.
 * A lot has been modified, especially that this now deals with bytes
 */

#include <cstdint>
#include "dvbs2.h"

#define TS_SIZE 188
#define TS_ERROR_INDICATOR 0x80

namespace dvbs2
{
    struct BBHeader
    {
        uint8_t ts_gs;
        bool sis_mis;
        bool ccm_acm;
        bool issyi;
        bool npd;
        bool ro;
        uint8_t isi;
        uint16_t upl;
        uint16_t dfl;
        uint8_t sync;
        uint16_t syncd;

        BBHeader(uint8_t *bbf)
        {
            ts_gs = bbf[0] >> 6;
            sis_mis = (bbf[0] >> 5) & 1;
            ccm_acm = (bbf[0] >> 4) & 1;
            issyi = (bbf[0] >> 3) & 1;
            npd = (bbf[0] >> 2) & 1;
            ro = bbf[0] & 0b11;
            isi = sis_mis == 0 ? bbf[1] : 0;
            upl = bbf[2] << 8 | bbf[3];
            dfl = bbf[4] << 8 | bbf[5];
            sync = bbf[6];
            syncd = bbf[7] << 8 | bbf[8];
        }
    };

    class BBFrameTSParser
    {
    private:
        unsigned int kbch;
        unsigned int max_dfl;
        unsigned int df_remaining;
        unsigned int count;
        unsigned int synched;
        unsigned char crc;
        unsigned int distance;
        unsigned int spanning;
        unsigned int index;
        unsigned char crc_tab[256];
        unsigned char packet[188];

        void build_crc8_table(void);
        unsigned int check_crc8(const unsigned char *, int);

    public:
        BBFrameTSParser(int bbframe_size);
        ~BBFrameTSParser();

        int work(uint8_t *bbframe, int cnt, uint8_t *tsframes, int buffer_outsize); // TSFrame should be at least as big as bbframe to be safe
    };
}