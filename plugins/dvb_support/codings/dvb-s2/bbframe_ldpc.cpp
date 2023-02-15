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

#include "bbframe_ldpc.h"
#include "codings/dvb-s2/ldpc/dvb_s2_tables.hh"
#include <cstring>

namespace dvbs2
{
    BBFrameLDPC::BBFrameLDPC(dvbs2_framesize_t framesize, dvbs2_code_rate_t rate)
    {
        if (framesize == FECFRAME_NORMAL)
        {
            switch (rate)
            {
            case C1_4:
                ldpc = new LDPC<DVB_S2_TABLE_B1>();
                break;
            case C1_3:
                ldpc = new LDPC<DVB_S2_TABLE_B2>();
                break;
            case C2_5:
                ldpc = new LDPC<DVB_S2_TABLE_B3>();
                break;
            case C1_2:
                ldpc = new LDPC<DVB_S2_TABLE_B4>();
                break;
            case C3_5:
                ldpc = new LDPC<DVB_S2_TABLE_B5>();
                break;
            case C2_3:
                ldpc = new LDPC<DVB_S2_TABLE_B6>();
                break;
            case C3_4:
                ldpc = new LDPC<DVB_S2_TABLE_B7>();
                break;
            case C4_5:
                ldpc = new LDPC<DVB_S2_TABLE_B8>();
                break;
            case C5_6:
                ldpc = new LDPC<DVB_S2_TABLE_B9>();
                break;
            case C8_9:
                ldpc = new LDPC<DVB_S2_TABLE_B10>();
                break;
            case C9_10:
                ldpc = new LDPC<DVB_S2_TABLE_B11>();
                break;
            default:
                break;
            }
        }
        else if (framesize == FECFRAME_SHORT)
        {
            switch (rate)
            {
            case C1_4:
                ldpc = new LDPC<DVB_S2_TABLE_C1>();
                break;
            case C1_3:
                ldpc = new LDPC<DVB_S2_TABLE_C2>();
                break;
            case C2_5:
                ldpc = new LDPC<DVB_S2_TABLE_C3>();
                break;
            case C1_2:
                ldpc = new LDPC<DVB_S2_TABLE_C4>();
                break;
            case C3_5:
                ldpc = new LDPC<DVB_S2_TABLE_C5>();
                break;
            case C2_3:
                ldpc = new LDPC<DVB_S2_TABLE_C6>();
                break;
            case C3_4:
                ldpc = new LDPC<DVB_S2_TABLE_C7>();
                break;
            case C4_5:
                ldpc = new LDPC<DVB_S2_TABLE_C8>();
                break;
            case C5_6:
                ldpc = new LDPC<DVB_S2_TABLE_C9>();
                break;
            case C8_9:
                ldpc = new LDPC<DVB_S2_TABLE_C10>();
                break;
            default:
                break;
            }
        }

        decoder.init(ldpc);
        encoder.init(ldpc);

        aligned_buffer = new simd_type[68400];
    }

    BBFrameLDPC::~BBFrameLDPC()
    {
        delete ldpc;
    }

    int BBFrameLDPC::decode(int8_t *frame, int max_trials)
    {
        int trials = decoder((void *)aligned_buffer, (code_type *)frame, max_trials);

        if (trials < 0)
            return trials;
        else
            return max_trials - trials;
    }

    void BBFrameLDPC::encode(uint8_t *frame)
    {
        int8_t *soft_buffer = new int8_t[ldpc->code_len()];

        // Convert to soft
        for (int i = 0; i < ldpc->data_len(); i++)
            soft_buffer[i] = ((frame[i / 8] >> (7 - (i % 8))) & 1) ? 127 : -127;

        encoder(soft_buffer, &soft_buffer[ldpc->data_len()]);

        // Repack to bytes
        memset(&frame[ldpc->data_len() / 8], 0, (ldpc->code_len() - ldpc->data_len()) / 8);
        for (int i = 0; i < (ldpc->code_len() - ldpc->data_len()); i++)
            frame[(ldpc->data_len() / 8) + i / 8] = frame[(ldpc->data_len() / 8) + i / 8] << 1 | (soft_buffer[ldpc->data_len() + i] < 0);

        delete[] soft_buffer;
    }
}