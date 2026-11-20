#include "common/codings/differential/nrzs.h"
#include "helpers/random.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace
{
    using test_helpers::randomBits;

    // NRZ-S: a 0 is sent as a level change, a 1 as no change; starting level 0
    std::vector<uint8_t> nrzsEncodeBits(const std::vector<uint8_t> &data)
    {
        std::vector<uint8_t> out(data.size());
        uint8_t level = 0;
        for (size_t i = 0; i < data.size(); i++)
            out[i] = level ^= !data[i];
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

TEST_CASE("NRZSDiff decode_bits inverts NRZ-S encoding", "[common][codings][differential]")
{
    std::vector<uint8_t> data = randomBits(1000, 11);
    std::vector<uint8_t> coded = nrzsEncodeBits(data);

    diff::NRZSDiff dec;
    dec.decode_bits(coded.data(), coded.size());
    CHECK(coded == data);
}

TEST_CASE("NRZSDiff decode on packed bytes inverts NRZ-S encoding", "[common][codings][differential]")
{
    std::vector<uint8_t> data = randomBits(8 * 128, 12);
    std::vector<uint8_t> coded = packBits(nrzsEncodeBits(data));

    diff::NRZSDiff dec;
    dec.decode(coded.data(), coded.size());
    CHECK(coded == packBits(data));
}

TEST_CASE("NRZSDiff decode known byte values", "[common][codings][differential]")
{
    // Complement of the NRZ-M result for the same levels
    uint8_t b = 0b10110000;
    diff::NRZSDiff dec;
    dec.decode(&b, 1);
    CHECK(b == 0x17);

    // Alternating levels starting with a change from 0 are all zeros
    uint8_t alt = 0b10101010;
    diff::NRZSDiff dec2;
    dec2.decode(&alt, 1);
    CHECK(alt == 0x00);
}

TEST_CASE("NRZSDiff keeps state across calls, for bits and bytes", "[common][codings][differential]")
{
    std::vector<uint8_t> data = randomBits(8 * 64, 13);
    std::vector<uint8_t> codedBits = nrzsEncodeBits(data);
    std::vector<uint8_t> codedBytes = packBits(codedBits);

    diff::NRZSDiff bitDec;
    bitDec.decode_bits(&codedBits[0], 9);
    bitDec.decode_bits(&codedBits[9], 1);
    bitDec.decode_bits(&codedBits[10], codedBits.size() - 10);
    CHECK(codedBits == data);

    diff::NRZSDiff byteDec;
    byteDec.decode(&codedBytes[0], 3);
    byteDec.decode(&codedBytes[3], codedBytes.size() - 3);
    CHECK(codedBytes == packBits(data));
}

TEST_CASE("nrzs_decode decodes a single block starting from level 0", "[common][codings][differential]")
{
    std::vector<uint8_t> data = randomBits(8 * 32, 14);
    std::vector<uint8_t> coded = packBits(nrzsEncodeBits(data));
    diff::nrzs_decode(coded.data(), coded.size());
    CHECK(coded == packBits(data));
}
