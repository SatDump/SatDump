/*
 * DifferentialDecoder.h
 *
 *  Created on: 25/01/2017
 *      Author: Lucas Teske
 *      Modified by Aang23
 */

#include <cstdint>

class NRZMDiff
{
private:
    uint8_t mask;
    uint8_t lastBit = 0;

public:
    void decode(uint8_t *data, int length);
};