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

#include "bbframe_ts_parser.h"
#include <cstring>
//#include "logger.h"

namespace dvbs2
{
    BBFrameTSParser::BBFrameTSParser(int bbframe_size)
    {
        kbch = bbframe_size;

        max_dfl = kbch - 80;
        build_crc8_table();

        count = 0;
        index = 0;
        synched = false;
        spanning = false;
    }

    BBFrameTSParser::~BBFrameTSParser()
    {
    }

#define CRC_POLY 0xAB
// Reversed
#define CRC_POLYR 0xD5

    void BBFrameTSParser::build_crc8_table(void)
    {
        int r, crc;

        for (int i = 0; i < 256; i++)
        {
            r = i;
            crc = 0;
            for (int j = 7; j >= 0; j--)
            {
                if ((r & (1 << j) ? 1 : 0) ^ ((crc & 0x80) ? 1 : 0))
                {
                    crc = (crc << 1) ^ CRC_POLYR;
                }
                else
                {
                    crc <<= 1;
                }
            }
            crc_tab[i] = crc;
        }
    }

    /*
     * MSB is sent first
     *
     * The polynomial has been reversed
     */
    unsigned int BBFrameTSParser::check_crc8(const uint8_t *in, int length)
    {
        int crc = 0;
        int b;

        for (int n = 0; n < length; n++)
        {
            b = ((in[n / 8] >> (7 - (n % 8))) & 1) ^ (crc & 0x01);
            crc >>= 1;
            if (b)
            {
                crc ^= CRC_POLY;
            }
        }

        return (crc);
    }

    int BBFrameTSParser::work(uint8_t *bbframe, int cnt, uint8_t *tsframes)
    {
        int in_p = 0;
        int out_p = 0;
        int tei_p = 0;
        uint8_t tmp;

        int errors = 0;

        int produced = 0;

        for (int currentBBIndex = 0; currentBBIndex < cnt; currentBBIndex++)
        {
            uint8_t *bbf = &bbframe[(kbch / 8) * currentBBIndex]; // Current bbframe

            // Check CRC
            unsigned int crc_check = check_crc8(bbf, 80);

            if (crc_check == 0)
                crc_check = true;
            else
                crc_check = false;

            if (crc_check != true)
            {
                synched = false;
                // logger->info("BBHeader CRC Fail");
                continue;
            }

            // Parse header
            BBHeader header(bbf);

            // Validate header
            if (header.dfl > max_dfl)
            {
                synched = false;
                // logger->info("DFL Too long!");
                continue;
            }

            if (header.dfl % 8 != 0)
            {
                synched = false;
                // logger->info("DFL Not a multiple of 8!");
                continue;
            }

            df_remaining = header.dfl / 8;

            // Skip header to datafield
            bbf += 10;

            if (!synched)
            {
                // logger->info("Resynchronizing...");

                bbf += header.syncd / 8 + 1;
                df_remaining -= header.syncd / 8 + 1;

                count = 0;
                synched = true;
                index = 0;
                spanning = false;
                distance = header.syncd / 8;
            }

            while (df_remaining)
            {
                if (count == 0)
                {
                    crc = 0;
                    if (index == TS_SIZE)
                    {
                        memcpy(&tsframes[out_p], packet, TS_SIZE);
                        produced += TS_SIZE;
                        out_p += TS_SIZE;

                        index = 0;
                        spanning = false;
                    }
                    if (df_remaining < (TS_SIZE - 1))
                    {
                        index = 0;
                        packet[index++] = 0x47;
                        spanning = true;
                    }
                    else
                    {
                        tsframes[out_p++] = 0x47;
                        produced++;
                        tei_p = out_p;
                    }

                    count++;

                    if (crc_check == true)
                    {
                        if (distance != (unsigned int)header.syncd / 8)
                            synched = false;

                        crc_check = false;
                    }
                }
                else if (count == TS_SIZE)
                {
                    tmp = bbf[in_p++];

                    if (tmp != crc)
                    {
                        errors++;

                        if (spanning)
                            packet[1] |= TS_ERROR_INDICATOR;
                        else
                            tsframes[tei_p] |= TS_ERROR_INDICATOR;
                    }

                    count = 0;
                    df_remaining -= 1;

                    if (df_remaining == 0)
                        distance = (TS_SIZE - 1);
                }
                if (df_remaining >= 1 && count > 0)
                {
                    tmp = bbf[in_p++];
                    distance += 1;

                    crc = crc_tab[tmp ^ crc];
                    if (spanning == true)
                    {
                        packet[index++] = tmp;
                    }
                    else
                    {
                        tsframes[out_p++] = tmp;
                        produced++;
                    }

                    count++;
                    df_remaining -= 1;

                    if (df_remaining == 0)
                        distance = 0;
                }
            }
        }

        return produced / TS_SIZE;
    }
}