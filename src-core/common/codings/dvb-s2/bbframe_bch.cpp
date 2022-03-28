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

#include "bbframe_bch.h"
#include <stdio.h>
#include <functional>
#include <cstring>

// BCH Code
#define BCH_CODE_N8 0
#define BCH_CODE_N10 1
#define BCH_CODE_N12 2
#define BCH_CODE_S12 3
#define BCH_CODE_M12 4

namespace dvbs2
{
    BBFrameBCH::BBFrameBCH(dvbs2_framesize_t framesize, dvbs2_code_rate_t rate)
    {
        if (framesize == FECFRAME_NORMAL)
        {
            switch (rate)
            {
            case C1_4:
                kbch = 16008;
                nbch = 16200;
                bch_code = BCH_CODE_N12;
                break;
            case C1_3:
                kbch = 21408;
                nbch = 21600;
                bch_code = BCH_CODE_N12;
                break;
            case C2_5:
                kbch = 25728;
                nbch = 25920;
                bch_code = BCH_CODE_N12;
                break;
            case C1_2:
                kbch = 32208;
                nbch = 32400;
                bch_code = BCH_CODE_N12;
                break;
            case C3_5:
                kbch = 38688;
                nbch = 38880;
                bch_code = BCH_CODE_N12;
                break;
            case C2_3:
                kbch = 43040;
                nbch = 43200;
                bch_code = BCH_CODE_N10;
                break;
            case C3_4:
                kbch = 48408;
                nbch = 48600;
                bch_code = BCH_CODE_N12;
                break;
            case C4_5:
                kbch = 51648;
                nbch = 51840;
                bch_code = BCH_CODE_N12;
                break;
            case C5_6:
                kbch = 53840;
                nbch = 54000;
                bch_code = BCH_CODE_N10;
                break;
            case C8_9:
                kbch = 57472;
                nbch = 57600;
                bch_code = BCH_CODE_N8;
                break;
            case C9_10:
                kbch = 58192;
                nbch = 58320;
                bch_code = BCH_CODE_N8;
                break;
            default:
                kbch = 0;
                nbch = 0;
                bch_code = 0;
                break;
            }
        }
        else if (framesize == FECFRAME_SHORT)
        {
            switch (rate)
            {
            case C1_4:
                kbch = 3072;
                nbch = 3240;
                bch_code = BCH_CODE_S12;
                break;
            case C1_3:
                kbch = 5232;
                nbch = 5400;
                bch_code = BCH_CODE_S12;
                break;
            case C2_5:
                kbch = 6312;
                nbch = 6480;
                bch_code = BCH_CODE_S12;
                break;
            case C1_2:
                kbch = 7032;
                nbch = 7200;
                bch_code = BCH_CODE_S12;
                break;
            case C3_5:
                kbch = 9552;
                nbch = 9720;
                bch_code = BCH_CODE_S12;
                break;
            case C2_3:
                kbch = 10632;
                nbch = 10800;
                bch_code = BCH_CODE_S12;
                break;
            case C3_4:
                kbch = 11712;
                nbch = 11880;
                bch_code = BCH_CODE_S12;
                break;
            case C4_5:
                kbch = 12432;
                nbch = 12600;
                bch_code = BCH_CODE_S12;
                break;
            case C5_6:
                kbch = 13152;
                nbch = 13320;
                bch_code = BCH_CODE_S12;
                break;
            case C8_9:
                kbch = 14232;
                nbch = 14400;
                bch_code = BCH_CODE_S12;
                break;
            default:
                kbch = 0;
                nbch = 0;
                bch_code = 0;
                break;
            }
        }
        frame = 0;
        instance_n = new GF_NORMAL();
        instance_m = new GF_MEDIUM();
        instance_s = new GF_SHORT();
        decode_n_12 = new BCH_NORMAL_12();
        decode_n_10 = new BCH_NORMAL_10();
        decode_n_8 = new BCH_NORMAL_8();
        decode_m_12 = new BCH_MEDIUM_12();
        decode_s_12 = new BCH_SHORT_12();
        code = new uint8_t[8192];
        parity = new uint8_t[24];
        for (int i = 0; i < 8192; i++)
        {
            code[i] = 0;
        }
        for (int i = 0; i < 24; i++)
        {
            parity[i] = 0;
        }
    }

    /*
     * Our virtual destructor.
     */
    BBFrameBCH::~BBFrameBCH()
    {
        delete[] parity;
        delete[] code;
        delete decode_s_12;
        delete decode_m_12;
        delete decode_n_8;
        delete decode_n_10;
        delete decode_n_12;
        delete instance_s;
        delete instance_m;
        delete instance_n;
    }

    int BBFrameBCH::work(uint8_t *frame)
    {
        int corrections = 0;

        for (unsigned int j = 0; j < kbch; j++)
            code[j / 8] = (~(1 << (7 - j % 8)) & code[j / 8]) | (((frame[j / 8] >> (7 - j % 8)) & 1) << (7 - j % 8));

        for (unsigned int j = kbch; j < nbch; j++)
            parity[(j - kbch) / 8] = (~(1 << (7 - (j - kbch) % 8)) & parity[(j - kbch) / 8]) | (((frame[j / 8] >> (7 - j % 8)) & 1) << (7 - j % 8));

        switch (bch_code)
        {
        case BCH_CODE_N12:
            corrections = (*decode_n_12)(code, parity, 0, 0, kbch);
            break;
        case BCH_CODE_N10:
            corrections = (*decode_n_10)(code, parity, 0, 0, kbch);
            break;
        case BCH_CODE_N8:
            corrections = (*decode_n_8)(code, parity, 0, 0, kbch);
            break;
        case BCH_CODE_S12:
            corrections = (*decode_s_12)(code, parity, 0, 0, kbch);
            break;
        case BCH_CODE_M12:
            corrections = (*decode_m_12)(code, parity, 0, 0, kbch);
            break;
        }

        for (unsigned int j = 8; j < kbch; j++)
        {
            frame[j / 8] = (~(1 << (7 - j % 8)) & frame[j / 8]) | (((code[j / 8] >> (7 - j % 8)) & 1) << (7 - (j - 8) % 8));
        }

        return corrections;
    }
}