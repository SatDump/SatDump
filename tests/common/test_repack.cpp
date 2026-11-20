#include "common/repack.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
    // Deterministic pseudo-random bytes, so failures are reproducible
    std::vector<uint8_t> makeBytes(int count)
    {
        std::vector<uint8_t> bytes(count);
        uint32_t state = 0x12345678;
        for (auto &b : bytes)
        {
            state = state * 1664525u + 1013904223u;
            b = state >> 24;
        }
        return bytes;
    }

    // Straightforward MSB-first repacking to compare the optimized versions against
    std::vector<uint32_t> referenceRepack(const std::vector<uint8_t> &bytes, int bits)
    {
        std::vector<uint32_t> words;
        uint32_t shifter = 0;
        int inshifter = 0;
        for (uint8_t byte : bytes)
        {
            for (int b = 7; b >= 0; b--)
            {
                shifter = (shifter << 1) | ((byte >> b) & 1);
                if (++inshifter == bits)
                {
                    words.push_back(shifter);
                    shifter = 0;
                    inshifter = 0;
                }
            }
        }
        return words;
    }

    template <typename WordT>
    void checkAgainstReference(int (*repack)(uint8_t *, int, WordT *), int bits)
    {
        // Lengths covering both the block-aligned and the leftover code paths
        for (int length : {0, 1, 2, 3, 5, 7, 13, 16, 20, 64, 65, 100, 257})
        {
            INFO("bits = " << bits << ", byte length = " << length);

            std::vector<uint8_t> bytes = makeBytes(length);
            std::vector<uint32_t> expected = referenceRepack(bytes, bits);
            std::vector<WordT> words(length + 1, 0);

            int count = repack(bytes.data(), length, words.data());

            REQUIRE(count == (int)expected.size());
            for (int i = 0; i < count; i++)
                REQUIRE(words[i] == expected[i]);
        }
    }
} // namespace

TEST_CASE("repackBytesTo10bits packs known values", "[common][repack]")
{
    uint8_t bytes[7] = {0xFF, 0xC0, 0x00, 0x00, 0x01, 0xAB, 0xCD};
    uint16_t words[8] = {0};

    REQUIRE(repackBytesTo10bits(bytes, 5, words) == 4);
    CHECK(words[0] == 1023);
    CHECK(words[1] == 0);
    CHECK(words[2] == 0);
    CHECK(words[3] == 1);

    // Two leftover bytes only hold one more complete word
    REQUIRE(repackBytesTo10bits(bytes, 7, words) == 5);
    CHECK(words[4] == 0x2AF);
}

TEST_CASE("repackBytesTo12bits packs known values", "[common][repack]")
{
    uint8_t bytes[3] = {0xAB, 0xCD, 0xEF};
    uint16_t words[2] = {0};

    REQUIRE(repackBytesTo12bits(bytes, 3, words) == 2);
    CHECK(words[0] == 0xABC);
    CHECK(words[1] == 0xDEF);
}

TEST_CASE("repackBytesTo16bits packs big-endian words", "[common][repack]")
{
    uint8_t bytes[4] = {0x12, 0x34, 0x56, 0x78};
    uint16_t words[2] = {0};

    REQUIRE(repackBytesTo16bits(bytes, 4, words) == 2);
    CHECK(words[0] == 0x1234);
    CHECK(words[1] == 0x5678);
}

TEST_CASE("16-bit-word repackers match a reference implementation", "[common][repack]")
{
    checkAgainstReference<uint16_t>(repackBytesTo10bits, 10);
    checkAgainstReference<uint16_t>(repackBytesTo12bits, 12);
    checkAgainstReference<uint16_t>(repackBytesTo13bits, 13);
    checkAgainstReference<uint16_t>(repackBytesTo14bits, 14);
    checkAgainstReference<uint16_t>(repackBytesTo16bits, 16);
}

TEST_CASE("32-bit-word repackers match a reference implementation", "[common][repack]")
{
    checkAgainstReference<uint32_t>(repackBytesTo17bits, 17);
    checkAgainstReference<uint32_t>(repackBytesTo18bits, 18);
    checkAgainstReference<uint32_t>(repackBytesTo19bits, 19);
    checkAgainstReference<uint32_t>(repackBytesTo20bits, 20);
}

TEST_CASE("shift_array_left shifts bits across byte boundaries", "[common][repack]")
{
    // The function reads one byte past byte_length to fill the last output byte
    uint8_t in[3] = {0x0F, 0xF0, 0xA0};
    uint8_t out[2] = {0, 0};

    shift_array_left(in, 2, 4, out);
    CHECK(out[0] == 0xFF);
    CHECK(out[1] == 0x0A);

    shift_array_left(in, 2, 1, out);
    CHECK(out[0] == 0x1F);
    CHECK(out[1] == 0xE1);
}
