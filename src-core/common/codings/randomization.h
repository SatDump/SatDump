#pragma once

#include <cstdint>

/*
CCSDS Derandomizer, adapted from SatHelper
*/

// Byte (bit)-based derandomizer
void derand_ccsds(uint8_t *data, int length);

// Soft symbol derandomizer
void derand_ccsds_soft(int8_t *data, int length);

// Bits
void derand_ccsds_bits(uint8_t *data, int length);
