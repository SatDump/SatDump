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
 * This file was originally adapted from https://github.com/drmpeg/gr-dvbs2rx,
 * but testing revealed LeanDVB's BCH performed significantly better overall.
 */

#pragma once

#include <cstdint>
#include "bch/bch.h"
#include "dvbs2.h"
#include <utility>

namespace dvbs2
{
    class BBFrameBCH
    {
    private:
        leansdr::bch_interface *bchs[2][12];
        // N=t*m
        // The generator of GF(2^m) is always g1.
        // Normal frames with 8, 10 or 12 polynomials.
        typedef leansdr::bch_engine<uint32_t, 192, 17, 16, uint16_t, 0x002d> s2_bch_engine_nf12;
        typedef leansdr::bch_engine<uint32_t, 160, 17, 16, uint16_t, 0x002d> s2_bch_engine_nf10;
        typedef leansdr::bch_engine<uint32_t, 128, 17, 16, uint16_t, 0x002d> s2_bch_engine_nf8;
        // Short frames with 12 polynomials.
        typedef leansdr::bch_engine<uint32_t, 168, 17, 14, uint16_t, 0x002b> s2_bch_engine_sf12;

    public:
        BBFrameBCH();
        ~BBFrameBCH();

        std::pair<int, int> frame_params(dvbs2_framesize_t framesize, dvbs2_code_rate_t rate)
        {
            if (framesize == FECFRAME_NORMAL)
            {
                switch (rate)
                {
                case C1_4:
                    return {16008, 16200};
                case C1_3:
                    return {21408, 21600};
                case C2_5:
                    return {25728, 25920};
                case C1_2:
                    return {32208, 32400};
                case C3_5:
                    return {38688, 38880};
                case C2_3:
                    return {43040, 43200};
                case C3_4:
                    return {48408, 48600};
                case C4_5:
                    return {51648, 51840};
                case C5_6:
                    return {53840, 54000};
                case C8_9:
                    return {57472, 57600};
                case C9_10:
                    return {58192, 58320};
                default:
                    return {0, 0};
                }
            }
            else if (framesize == FECFRAME_SHORT)
            {
                switch (rate)
                {
                case C1_4:
                    return {3072, 3240};
                case C1_3:
                    return {5232, 5400};
                case C2_5:
                    return {6312, 6480};
                case C1_2:
                    return {7032, 7200};
                case C3_5:
                    return {9552, 9720};
                case C2_3:
                    return {10632, 10800};
                case C3_4:
                    return {11712, 11880};
                case C4_5:
                    return {12432, 12600};
                case C5_6:
                    return {13152, 13320};
                case C8_9:
                    return {14232, 14400};
                default:
                    return {0, 0};
                }
            }
            else
            {
                return {0, 0};
            }
        }

        inline int decode(uint8_t *frame, dvbs2_framesize_t framesize, dvbs2_code_rate_t rate)
        {
            return bchs[framesize == FECFRAME_SHORT][rate]->decode(frame, frame_params(framesize, rate).second / 8);
        }

        inline void encode(uint8_t *frame, dvbs2_framesize_t framesize, dvbs2_code_rate_t rate)
        {
            int data_s = frame_params(framesize, rate).first / 8;
            bchs[framesize == FECFRAME_SHORT][rate]->encode(frame, data_s, &frame[data_s]);
        }
    };
}