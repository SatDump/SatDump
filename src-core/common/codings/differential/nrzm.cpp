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

    void NRZMDiff::decode_bits(uint8_t *data, int length)
    {
        uint8_t currentBit = 0;
        for (int i = 0; i < length; i++)
        {
            currentBit = data[i];
            data[i] = currentBit ^ lastBit;
            lastBit = currentBit;
        }
    }

    void nrzm_decode(uint8_t *data, int length)
    {
        uint8_t mask, lastBit = 0;
        for (int i = 0; i < length; i++)
        {
            mask = ((data[i] >> 1) & 0x7F) | (lastBit << 7);
            lastBit = data[i] & 1;
            data[i] ^= mask;
        }
    }
} // namespace diff