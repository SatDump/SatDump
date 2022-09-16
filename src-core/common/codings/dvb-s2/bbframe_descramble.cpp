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

#include "bbframe_descramble.h"
#include <algorithm>

namespace dvbs2
{
    BBFrameDescrambler::BBFrameDescrambler(dvbs2_framesize_t framesize, dvbs2_code_rate_t rate)
    {
        if (framesize == FECFRAME_NORMAL)
        {
            switch (rate)
            {
            case C1_4:
                kbch = 16008;
                break;
            case C1_3:
                kbch = 21408;
                break;
            case C2_5:
                kbch = 25728;
                break;
            case C1_2:
                kbch = 32208;
                break;
            case C3_5:
                kbch = 38688;
                break;
            case C2_3:
                kbch = 43040;
                break;
            case C3_4:
                kbch = 48408;
                break;
            case C4_5:
                kbch = 51648;
                break;
            case C5_6:
                kbch = 53840;
                break;
            case C8_9:
                kbch = 57472;
                break;
            case C9_10:
                kbch = 58192;
                break;
            default:
                kbch = 0;
                break;
            }
        }
        else if (framesize == FECFRAME_SHORT)
        {
            switch (rate)
            {
            case C1_4:
                kbch = 3072;
                break;
            case C1_3:
                kbch = 5232;
                break;
            case C2_5:
                kbch = 6312;
                break;
            case C1_2:
                kbch = 7032;
                break;
            case C3_5:
                kbch = 9552;
                break;
            case C2_3:
                kbch = 10632;
                break;
            case C3_4:
                kbch = 11712;
                break;
            case C4_5:
                kbch = 12432;
                break;
            case C5_6:
                kbch = 13152;
                break;
            case C8_9:
                kbch = 14232;
                break;
            default:
                kbch = 0;
                break;
            }
        }

        init();
    }

    BBFrameDescrambler::~BBFrameDescrambler()
    {
    }

    void BBFrameDescrambler::init()
    {
        std::fill(bb_derandomise, &bb_derandomise[FRAME_SIZE_NORMAL / 8], 0);

        int sr = 0x4A80;
        for (int i = 0; i < FRAME_SIZE_NORMAL; i++)
        {
            int b = ((sr) ^ (sr >> 1)) & 1;
            bb_derandomise[i / 8] = b << (7 - (i % 8)) | bb_derandomise[i / 8];

            sr >>= 1;
            if (b)
                sr |= 0x4000;
        }
    }

    int BBFrameDescrambler::work(uint8_t *frame)
    {
        for (int j = 0; j < (int)kbch / 8; ++j)
            frame[j] = frame[j] ^ bb_derandomise[j];
        return 0;
    }
}