#include "common/codings/randomization.h"
#include "helpers/random.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace
{
    using test_helpers::randomBytes;

    // CCSDS 131.0-B pseudo-randomizer: h(x) = x^8 + x^7 + x^5 + x^3 + 1, all-ones seed,
    // i.e. a[n+8] = a[n+7] ^ a[n+5] ^ a[n+3] ^ a[n]
    std::vector<uint8_t> ccsdsPnBits(int count)
    {
        std::vector<uint8_t> bits(count);
        for (int n = 0; n < count; n++)
        {
            if (n < 8)
                bits[n] = 1;
            else
                bits[n] = bits[n - 1] ^ bits[n - 3] ^ bits[n - 5] ^ bits[n - 8];
        }
        return bits;
    }

    std::vector<uint8_t> ccsdsPnBytes(int count)
    {
        std::vector<uint8_t> bits = ccsdsPnBits(count * 8);
        std::vector<uint8_t> bytes(count, 0);
        for (int i = 0; i < count * 8; i++)
            bytes[i / 8] = bytes[i / 8] << 1 | bits[i];
        return bytes;
    }
} // namespace

TEST_CASE("derand_ccsds applied to zeros yields the CCSDS pseudo-random sequence", "[common][codings][randomization]")
{
    // Two full periods plus a bit, to cover the wrap at 255 bytes
    const int length = 255 * 2 + 17;
    std::vector<uint8_t> data(length, 0);
    derand_ccsds(data.data(), length);

    std::vector<uint8_t> expected = ccsdsPnBytes(length);
    REQUIRE(expected[0] == 0xFF);
    REQUIRE(expected[1] == 0x48);
    REQUIRE(expected[2] == 0x0E);
    REQUIRE(expected[3] == 0xC0);
    REQUIRE(expected[4] == 0x9A);

    for (int i = 0; i < length; i++)
    {
        INFO("byte " << i);
        REQUIRE(data[i] == expected[i]);
    }
}

TEST_CASE("derand_ccsds is its own inverse", "[common][codings][randomization]")
{
    for (int length : {0, 1, 254, 255, 256, 1020, 1279})
    {
        INFO("length " << length);
        std::vector<uint8_t> original = randomBytes(length, length + 1);
        std::vector<uint8_t> data = original;
        derand_ccsds(data.data(), length);
        if (length > 0)
            REQUIRE(data != original);
        derand_ccsds(data.data(), length);
        REQUIRE(data == original);
    }
}

TEST_CASE("derand_ccsds_bits flips unpacked bits following the PN sequence", "[common][codings][randomization]")
{
    const int length = 255 * 3 + 5;
    std::vector<uint8_t> pn = ccsdsPnBits(length);

    std::vector<uint8_t> zeros(length, 0);
    derand_ccsds_bits(zeros.data(), length);
    for (int i = 0; i < length; i++)
        REQUIRE(zeros[i] == pn[i]);

    std::vector<uint8_t> ones(length, 1);
    derand_ccsds_bits(ones.data(), length);
    for (int i = 0; i < length; i++)
        REQUIRE(ones[i] == (pn[i] ^ 1));

    // Same PN as the byte version, bit-for-bit
    std::vector<uint8_t> bytes(32, 0);
    derand_ccsds(bytes.data(), 32);
    for (int i = 0; i < 256; i++)
        REQUIRE(((bytes[i / 8] >> (7 - i % 8)) & 1) == pn[i]);
}

TEST_CASE("derand_ccsds_soft inverts soft symbols where the PN bit is set", "[common][codings][randomization]")
{
    const int length = 600;
    std::vector<uint8_t> pn = ccsdsPnBits(length);

    std::vector<int8_t> soft(length);
    for (int i = 0; i < length; i++)
        soft[i] = (int8_t)((i * 37) % 255 - 127);
    std::vector<int8_t> original = soft;

    derand_ccsds_soft(soft.data(), length);
    for (int i = 0; i < length; i++)
    {
        INFO("symbol " << i);
        // Inversion is a bitwise NOT, so the sign flips and the value becomes -x - 1
        int8_t expected = pn[i] ? (int8_t)~original[i] : original[i];
        REQUIRE(soft[i] == expected);
        if (pn[i])
            REQUIRE((soft[i] < 0) != (original[i] < 0));
    }

    derand_ccsds_soft(soft.data(), length);
    REQUIRE(soft == original);
}

TEST_CASE("derand_ccsds soft and hard paths agree on hard decisions", "[common][codings][randomization]")
{
    const int bytes = 40;
    std::vector<uint8_t> data = randomBytes(bytes, 99);

    std::vector<int8_t> soft(bytes * 8);
    for (int i = 0; i < bytes * 8; i++)
        soft[i] = ((data[i / 8] >> (7 - i % 8)) & 1) ? 90 : -90;

    derand_ccsds(data.data(), bytes);
    derand_ccsds_soft(soft.data(), bytes * 8);

    for (int i = 0; i < bytes * 8; i++)
        REQUIRE(((data[i / 8] >> (7 - i % 8)) & 1) == (soft[i] > 0));
}
