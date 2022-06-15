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
#include <cstring>

namespace dvbs2
{
    BBFrameBCH::BBFrameBCH()
    {
        leansdr::bitvect<uint32_t, 17> bch_polys[2][12]; // [shortframes][polyindex]
        // EN 302 307-1 5.3.1 Table 6a (polynomials for normal frames)
        bch_polys[0][0] = leansdr::bitvect<uint32_t, 17>(0x1002d);  // g1
        bch_polys[0][1] = leansdr::bitvect<uint32_t, 17>(0x10173);  // g2
        bch_polys[0][2] = leansdr::bitvect<uint32_t, 17>(0x10fbd);  // g3
        bch_polys[0][3] = leansdr::bitvect<uint32_t, 17>(0x15a55);  // g4
        bch_polys[0][4] = leansdr::bitvect<uint32_t, 17>(0x11f2f);  // g5
        bch_polys[0][5] = leansdr::bitvect<uint32_t, 17>(0x1f7b5);  // g6
        bch_polys[0][6] = leansdr::bitvect<uint32_t, 17>(0x1af65);  // g7
        bch_polys[0][7] = leansdr::bitvect<uint32_t, 17>(0x17367);  // g8
        bch_polys[0][8] = leansdr::bitvect<uint32_t, 17>(0x10ea1);  // g9
        bch_polys[0][9] = leansdr::bitvect<uint32_t, 17>(0x175a7);  // g10
        bch_polys[0][10] = leansdr::bitvect<uint32_t, 17>(0x13a2d); // g11
        bch_polys[0][11] = leansdr::bitvect<uint32_t, 17>(0x11ae3); // g12
        // EN 302 307-1 5.3.1 Table 6b (polynomials for short frames)
        bch_polys[1][0] = leansdr::bitvect<uint32_t, 17>(0x402b);  // g1
        bch_polys[1][1] = leansdr::bitvect<uint32_t, 17>(0x4941);  // g2
        bch_polys[1][2] = leansdr::bitvect<uint32_t, 17>(0x4647);  // g3
        bch_polys[1][3] = leansdr::bitvect<uint32_t, 17>(0x5591);  // g4
        bch_polys[1][4] = leansdr::bitvect<uint32_t, 17>(0x6b55);  // g5
        bch_polys[1][5] = leansdr::bitvect<uint32_t, 17>(0x6389);  // g6
        bch_polys[1][6] = leansdr::bitvect<uint32_t, 17>(0x6ce5);  // g7
        bch_polys[1][7] = leansdr::bitvect<uint32_t, 17>(0x4f21);  // g8
        bch_polys[1][8] = leansdr::bitvect<uint32_t, 17>(0x460f);  // g9
        bch_polys[1][9] = leansdr::bitvect<uint32_t, 17>(0x5a49);  // g10
        bch_polys[1][10] = leansdr::bitvect<uint32_t, 17>(0x5811); // g11
        bch_polys[1][11] = leansdr::bitvect<uint32_t, 17>(0x65ef); // g12
        // Redundant with fec_infos[], but needs static template argument.
        memset(bchs, 0, sizeof(bchs));
        bchs[0][C1_2] = new s2_bch_engine_nf12(bch_polys[0], 12);
        bchs[0][C2_3] = new s2_bch_engine_nf10(bch_polys[0], 10);
        bchs[0][C3_4] = new s2_bch_engine_nf12(bch_polys[0], 12);
        bchs[0][C5_6] = new s2_bch_engine_nf10(bch_polys[0], 10);
        bchs[0][C4_5] = new s2_bch_engine_nf12(bch_polys[0], 12);
        bchs[0][C8_9] = new s2_bch_engine_nf8(bch_polys[0], 8);
        bchs[0][C9_10] = new s2_bch_engine_nf8(bch_polys[0], 8);
        bchs[0][C1_4] = new s2_bch_engine_nf12(bch_polys[0], 12);
        bchs[0][C1_3] = new s2_bch_engine_nf12(bch_polys[0], 12);
        bchs[0][C2_5] = new s2_bch_engine_nf12(bch_polys[0], 12);
        bchs[0][C3_5] = new s2_bch_engine_nf12(bch_polys[0], 12);
        bchs[1][C1_2] = new s2_bch_engine_sf12(bch_polys[1], 12);
        bchs[1][C2_3] = new s2_bch_engine_sf12(bch_polys[1], 12);
        bchs[1][C3_4] = new s2_bch_engine_sf12(bch_polys[1], 12);
        bchs[1][C5_6] = new s2_bch_engine_sf12(bch_polys[1], 12);
        bchs[1][C4_5] = new s2_bch_engine_sf12(bch_polys[1], 12);
        bchs[1][C8_9] = new s2_bch_engine_sf12(bch_polys[1], 12);
        bchs[1][C1_4] = new s2_bch_engine_sf12(bch_polys[1], 12);
        bchs[1][C1_3] = new s2_bch_engine_sf12(bch_polys[1], 12);
        bchs[1][C2_5] = new s2_bch_engine_sf12(bch_polys[1], 12);
        bchs[1][C3_5] = new s2_bch_engine_sf12(bch_polys[1], 12);
    }

    BBFrameBCH::~BBFrameBCH()
    {
    }
}