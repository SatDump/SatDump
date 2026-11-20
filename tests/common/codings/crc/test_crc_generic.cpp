#include "common/codings/crc/crc_generic.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace
{
    struct CrcModel
    {
        const char *name;
        unsigned bits;
        uint64_t poly;
        uint64_t init;
        uint64_t xorout;
        bool refin;
        bool refout;
        uint64_t check; // CRC of the ASCII string "123456789" (CRC catalogue check value)
    };

    // Parameters and check values from the Williams/Ewing CRC catalogue
    const CrcModel CRC_MODELS[] = {
        {"CRC-8/SMBUS", 8, 0x07, 0x00, 0x00, false, false, 0xF4},
        {"CRC-8/MAXIM-DOW", 8, 0x31, 0x00, 0x00, true, true, 0xA1},
        {"CRC-12/UMTS", 12, 0x80F, 0x000, 0x000, false, true, 0xDAF},
        {"CRC-16/IBM-3740 (CCITT-FALSE)", 16, 0x1021, 0xFFFF, 0x0000, false, false, 0x29B1},
        {"CRC-16/IBM-SDLC (X-25)", 16, 0x1021, 0xFFFF, 0xFFFF, true, true, 0x906E},
        {"CRC-16/KERMIT", 16, 0x1021, 0x0000, 0x0000, true, true, 0x2189},
        {"CRC-16/XMODEM", 16, 0x1021, 0x0000, 0x0000, false, false, 0x31C3},
        {"CRC-16/ARC", 16, 0x8005, 0x0000, 0x0000, true, true, 0xBB3D},
        {"CRC-16/MODBUS", 16, 0x8005, 0xFFFF, 0x0000, true, true, 0x4B37},
        {"CRC-16/UMTS", 16, 0x8005, 0x0000, 0x0000, false, false, 0xFEE8},
        {"CRC-24/OPENPGP", 24, 0x864CFB, 0xB704CE, 0x000000, false, false, 0x21CF02},
        {"CRC-32/ISO-HDLC", 32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true, 0xCBF43926},
        {"CRC-32/BZIP2", 32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, false, false, 0xFC891918},
        {"CRC-32/MPEG-2", 32, 0x04C11DB7, 0xFFFFFFFF, 0x00000000, false, false, 0x0376E6E7},
        {"CRC-32/ISCSI (32C)", 32, 0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF, true, true, 0xE3069283},
        {"CRC-64/ECMA-182", 64, 0x42F0E1EBA9EA3693, 0x0, 0x0, false, false, 0x6C40DF5F0B497347},
        {"CRC-64/XZ", 64, 0x42F0E1EBA9EA3693, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, true, true, 0x995DC9BBDF1939FA},
        {"CRC-5/USB", 5, 0x05, 0x1F, 0x1F, true, true, 0x19},
    };

    codings::crc::GenericCRC makeCrc(const CrcModel &m) { return codings::crc::GenericCRC(m.bits, m.poly, m.init, m.xorout, m.refin, m.refout); }

    // Plain bit-at-a-time reference implementation of the same parameter model
    uint64_t referenceCrc(const CrcModel &m, const std::vector<uint8_t> &data)
    {
        uint64_t mask = m.bits == 64 ? ~0ULL : (1ULL << m.bits) - 1;
        uint64_t topbit = 1ULL << (m.bits - 1);
        uint64_t crc = m.init & mask;
        for (uint8_t byte : data)
        {
            for (int b = 0; b < 8; b++)
            {
                int bit = m.refin ? (byte >> b) & 1 : (byte >> (7 - b)) & 1;
                bool fb = ((crc & topbit) != 0) ^ bit;
                crc = (crc << 1) & mask;
                if (fb)
                    crc ^= m.poly & mask;
            }
        }
        if (m.refout)
        {
            uint64_t r = 0;
            for (unsigned i = 0; i < m.bits; i++)
                if (crc & (1ULL << i))
                    r |= 1ULL << (m.bits - 1 - i);
            crc = r;
        }
        return (crc ^ m.xorout) & mask;
    }

    std::vector<uint8_t> checkString()
    {
        const char *s = "123456789";
        return std::vector<uint8_t>(s, s + 9);
    }
} // namespace

TEST_CASE("GenericCRC matches the catalogue check value for standard models", "[common][codings][crc]")
{
    for (const CrcModel &m : CRC_MODELS)
    {
        INFO(m.name);
        auto crc = makeCrc(m);
        std::vector<uint8_t> data = checkString();
        CHECK(crc.compute(data.data(), data.size()) == m.check);
        CHECK(referenceCrc(m, data) == m.check); // validates the reference itself
    }
}

TEST_CASE("GenericCRC matches a bitwise reference on random data of many lengths", "[common][codings][crc]")
{
    std::vector<uint8_t> data(300);
    uint32_t state = 0xC0FFEE;
    for (auto &b : data)
    {
        state = state * 1664525u + 1013904223u;
        b = state >> 24;
    }

    for (const CrcModel &m : CRC_MODELS)
    {
        auto crc = makeCrc(m);
        for (size_t len : {1u, 2u, 3u, 7u, 8u, 64u, 255u, 300u})
        {
            INFO(m.name << ", length " << len);
            std::vector<uint8_t> chunk(data.begin(), data.begin() + len);
            REQUIRE(crc.compute(chunk.data(), len) == referenceCrc(m, chunk));
        }
    }
}

TEST_CASE("GenericCRC of empty input is the initial value after reflection and final XOR", "[common][codings][crc]")
{
    uint8_t dummy = 0;
    CHECK(codings::crc::GenericCRC(16, 0x1021, 0xFFFF, 0x0000, false, false).compute(&dummy, 0) == 0xFFFF);
    CHECK(codings::crc::GenericCRC(32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true).compute(&dummy, 0) == 0);
    CHECK(codings::crc::GenericCRC(16, 0x1021, 0x1234, 0x0000, false, false).compute(&dummy, 0) == 0x1234);
    // Input not reflected but output reflected: the init value comes out reflected
    CHECK(codings::crc::GenericCRC(16, 0x1021, 0x0001, 0x0000, false, true).compute(&dummy, 0) == 0x8000);
}

TEST_CASE("GenericCRC masks initial value and final XOR to the CRC width", "[common][codings][crc]")
{
    std::vector<uint8_t> data = checkString();
    auto crc = codings::crc::GenericCRC(16, 0x1021, 0xABCDFFFF, 0x55550000, false, false);
    CHECK(crc.compute(data.data(), data.size()) == 0x29B1);
}

TEST_CASE("GenericCRC appending the CRC gives a zero remainder", "[common][codings][crc]")
{
    // For a model without reflection or final XOR, CRC(message || CRC) == 0
    auto crc = codings::crc::GenericCRC(16, 0x1021, 0x0000, 0x0000, false, false);
    std::vector<uint8_t> msg = {'S', 'a', 't', 'D', 'u', 'm', 'p'};
    uint64_t value = crc.compute(msg.data(), msg.size());
    msg.push_back(value >> 8);
    msg.push_back(value & 0xFF);
    CHECK(crc.compute(msg.data(), msg.size()) == 0);

    // Reflected equivalent: CRC sent LSB first, residue is 0 as well with zero init/xorout
    auto kermit = codings::crc::GenericCRC(16, 0x1021, 0x0000, 0x0000, true, true);
    std::vector<uint8_t> msg2 = {'S', 'a', 't', 'D', 'u', 'm', 'p'};
    uint64_t v2 = kermit.compute(msg2.data(), msg2.size());
    msg2.push_back(v2 & 0xFF);
    msg2.push_back(v2 >> 8);
    CHECK(kermit.compute(msg2.data(), msg2.size()) == 0);
}

TEST_CASE("GenericCRC detects single bit errors", "[common][codings][crc]")
{
    auto crc = codings::crc::GenericCRC(32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true);
    std::vector<uint8_t> data(64, 0x3C);
    uint64_t good = crc.compute(data.data(), data.size());
    for (int bit = 0; bit < 64 * 8; bit += 13)
    {
        data[bit / 8] ^= 1 << (bit % 8);
        REQUIRE(crc.compute(data.data(), data.size()) != good);
        data[bit / 8] ^= 1 << (bit % 8);
    }
    CHECK(crc.compute(data.data(), data.size()) == good);
}

TEST_CASE("GenericCRC reflect reverses the low num_bits bits", "[common][codings][crc]")
{
    auto crc16 = codings::crc::GenericCRC(16, 0x1021, 0, 0, false, false);
    CHECK(crc16.reflect(0x0001) == 0x8000);
    CHECK(crc16.reflect(0x8000) == 0x0001);
    CHECK(crc16.reflect(0x1021) == 0x8408);
    CHECK(crc16.reflect(0xF0F0) == 0x0F0F);
    CHECK(crc16.reflect(crc16.reflect(0x1234)) == 0x1234);

    auto crc32 = codings::crc::GenericCRC(32, 0x04C11DB7, 0, 0, false, false);
    CHECK(crc32.reflect(0x04C11DB7) == 0xEDB88320);

    auto crc64 = codings::crc::GenericCRC(64, 1, 0, 0, false, false);
    CHECK(crc64.reflect(1) == 0x8000000000000000ULL);
}
