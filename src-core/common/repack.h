#pragma once

#include <cstdint>

// Repack bytes to 10-bits words, returns word count
int repackBytesTo10bits(uint8_t *bytes, int byte_length, uint16_t *words);

// Repack bytes to 12-bits words, returns word count
int repackBytesTo12bits(uint8_t *bytes, int byte_length, uint16_t *words);

// Repack bytes to 13-bits words, returns word count
int repackBytesTo13bits(uint8_t *bytes, int byte_length, uint16_t *words);

// Repack bytes to 14-bits words, returns word count
int repackBytesTo14bits(uint8_t *bytes, int byte_length, uint16_t *words);

// Repack bytes to 16-bits words, returns word count
int repackBytesTo16bits(uint8_t *bytes, int byte_length, uint16_t *words);

// Repack bytes to 17-bits words, returns word count
int repackBytesTo17bits(uint8_t *bytes, int byte_length, uint32_t *words);

// Repack bytes to 18-bits words, returns word count
int repackBytesTo18bits(uint8_t *bytes, int byte_length, uint32_t *words);

// Repack bytes to 20-bits words, returns word count
int repackBytesTo20bits(uint8_t *bytes, int byte_length, uint32_t *words);

// Shift array left by a a number of bits
void shift_array_left(uint8_t *in, int byte_length, int shift, uint8_t *out);