/*
 * DifferentialDecoder.cpp
 *
 *  Created on: 25/01/2017
 *      Author: Lucas Teske
 *      Modified by Aang23
 */

#include "nrzm.h"

namespace diff
{
    void NRZMDiff::decode(uint8_t *data, int length)
    {
        uint8_t mask;
        for (int i = 0; i < length; i++)
        {
            mask = ((data[i] >> 1) & 0x7F) | (lastBit << 7);
            lastBit = data[i] & 1;
            data[i] ^= mask;
        }
    }

    void nrzm_decode(uint8_t *data, int length)
    {
        uint8_t mask, lastBit;
        for (int i = 0; i < length; i++)
        {
            mask = ((data[i] >> 1) & 0x7F) | (lastBit << 7);
            lastBit = data[i] & 1;
            data[i] ^= mask;
        }
    }
} // namespace diff