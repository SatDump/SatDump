#pragma once

#include <vector>
#include <cstdint>

std::vector<uint16_t> bytesTo14bits(uint8_t *in, int skip, int lengthToConvert);
std::vector<uint16_t> bytesTo13bits(uint8_t *in, int skip, int lengthToConvert);
std::vector<uint16_t> bytesTo12bits(uint8_t *in, int skip, int lengthToConvert);