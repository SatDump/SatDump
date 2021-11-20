#pragma once

#include <cstdint>

/*
CCSDS Derandomizer, adapted from SatHelper
*/

void derand_ccsds(uint8_t *data, int length);
