#include "common/codings/differential/nrzm.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace
{
    std::vector<uint8_t> makeBits(int count, uint32_t seed)
    {
        std::vector<uint8_t> bits(count);
        for (auto &b : bits)
        {
            seed = seed * 1664525u + 1013904223u;
            b = seed >> 31;
        }
        return bits;
    }

    // NRZ-M: a 1 is sent as a level change, a 0 as no change; starting level 0
    std::vector<uint8_t> nrzmEncodeBits(const std::vector<uint8_t> &data)
    {
        std::vector<uint8_t> out(data.size());
        uint8_t level = 0;
        for (size_t i = 0; i < data.size(); i++)
            out[i] = level ^= data[i];
        return out;
    }

    std::vector<uint8_t> packBits(const std::vector<uint8_t> &bits)
    {
        std::vector<uint8_t> bytes(bits.size() / 8, 0);
        for (size_t i = 0; i < bytes.size() * 8; i++)
            bytes[i / 8] = bytes[i / 8] << 1 | bits[i];
        return bytes;
    }
} // namespace

TEST_CASE("NRZMDiff decode_bits inverts NRZ-M encoding", "[common][codings][differential]")
{
    std::vector<uint8_t> data = makeBits(1000, 1);
    std::vector<uint8_t> coded = nrzmEncodeBits(data);

    diff::NRZMDiff dec;
    dec.decode_bits(coded.data(), coded.size());
    CHECK(coded == data);
}

TEST_CASE("NRZMDiff decode on packed bytes inverts NRZ-M encoding", "[common][codings][differential]")
{
    std::vector<uint8_t> data = makeBits(8 * 128, 2);
    std::vector<uint8_t> coded = packBits(nrzmEncodeBits(data));

    diff::NRZMDiff dec;
    dec.decode(coded.data(), coded.size());
    CHECK(coded == packBits(data));
}

TEST_CASE("NRZMDiff decode known byte values", "[common][codings][differential]")
{
    // levels 1011 0000, previous levels (starting at 0) 0101 1000 -> XOR 1110 1000
    uint8_t b = 0b10110000;
    diff::NRZMDiff dec;
    dec.decode(&b, 1);
    CHECK(b == 0xE8);

    // Constant high level: only the initial transition is a 1, and the history carries over
    uint8_t ones[2] = {0xFF, 0xFF};
    diff::NRZMDiff dec2;
    dec2.decode(ones, 2);
    CHECK(ones[0] == 0x80);
    CHECK(ones[1] == 0x00);
}

TEST_CASE("NRZMDiff keeps state across calls, for bits and bytes", "[common][codings][differential]")
{
    std::vector<uint8_t> data = makeBits(8 * 64, 3);
    std::vector<uint8_t> codedBits = nrzmEncodeBits(data);
    std::vector<uint8_t> codedBytes = packBits(codedBits);

    diff::NRZMDiff bitDec;
    size_t pos = 0;
    for (size_t chunk : {1u, 7u, 100u, 3u, 250u})
    {
        bitDec.decode_bits(&codedBits[pos], chunk);
        pos += chunk;
    }
    bitDec.decode_bits(&codedBits[pos], codedBits.size() - pos);
    CHECK(codedBits == data);

    diff::NRZMDiff byteDec;
    byteDec.decode(&codedBytes[0], 5);
    byteDec.decode(&codedBytes[5], 1);
    byteDec.decode(&codedBytes[6], codedBytes.size() - 6);
    CHECK(codedBytes == packBits(data));
}

TEST_CASE("nrzm_decode decodes a single block starting from level 0", "[common][codings][differential]")
{
    std::vector<uint8_t> data = makeBits(8 * 32, 4);
    std::vector<uint8_t> coded = packBits(nrzmEncodeBits(data));
    std::vector<uint8_t> copy = coded;

    diff::nrzm_decode(coded.data(), coded.size());
    CHECK(coded == packBits(data));

    // No continuity: decoding the second half on its own assumes a 0 level before it
    diff::nrzm_decode(&copy[16], 16);
    std::vector<uint8_t> expected = packBits(data);
    uint8_t previousLevelBit = packBits(nrzmEncodeBits(data))[15] & 1;
    expected[16] ^= previousLevelBit << 7;
    CHECK(std::vector<uint8_t>(copy.begin() + 16, copy.end()) == std::vector<uint8_t>(expected.begin() + 16, expected.end()));
}
