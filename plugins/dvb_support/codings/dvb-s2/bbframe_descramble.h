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
 * This file has been adapted from https://github.com/drmpeg/gr-dvbs2rx
 */

#include <cstdint>
#include "common/codings/dvb-s2/dvbs2.h"

namespace dvbs2
{
    class BBFrameDescrambler
    {
    private:
        unsigned int kbch;
        unsigned char bb_derandomise[FRAME_SIZE_NORMAL / 8];
        void init();

    public:
        BBFrameDescrambler(dvbs2_framesize_t framesize, dvbs2_code_rate_t rate);
        ~BBFrameDescrambler();

        int work(uint8_t *frame);
    };
}