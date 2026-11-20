#include "common/codings/deframing/hdlc_def.h"
#include "helpers/random.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace
{
    using test_helpers::randomBytes;

    // CRC-16/X-25 (the HDLC FCS), bit by bit
    uint16_t hdlcFcs(const std::vector<uint8_t> &data)
    {
        uint16_t crc = 0xFFFF;
        for (uint8_t byte : data)
        {
            crc ^= byte;
            for (int i = 0; i < 8; i++)
                crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : crc >> 1;
        }
        return crc ^ 0xFFFF;
    }

    void appendFlag(std::vector<uint8_t> &bits)
    {
        for (int i = 0; i < 8; i++)
            bits.push_back((0x7E >> i) & 1);
    }

    // Appends data + FCS, LSB first, with a 0 stuffed after every five consecutive 1s
    void appendFrame(std::vector<uint8_t> &bits, const std::vector<uint8_t> &data, bool corruptFcs = false)
    {
        std::vector<uint8_t> bytes = data;
        uint16_t fcs = hdlcFcs(data);
        if (corruptFcs)
            fcs ^= 0x0100;
        bytes.push_back(fcs & 0xFF);
        bytes.push_back(fcs >> 8);

        int ones = 0;
        for (uint8_t byte : bytes)
        {
            for (int i = 0; i < 8; i++)
            {
                uint8_t bit = (byte >> i) & 1;
                bits.push_back(bit);
                ones = bit ? ones + 1 : 0;
                if (ones == 5)
                {
                    bits.push_back(0);
                    ones = 0;
                }
            }
        }
    }
} // namespace

TEST_CASE("HDLCDeframer extracts a single frame between flags", "[common][codings][deframing][hdlc]")
{
    std::vector<uint8_t> data = randomBytes(20, 1);
    std::vector<uint8_t> bits;
    appendFlag(bits);
    appendFlag(bits);
    appendFrame(bits, data);
    appendFlag(bits);

    ax25::HDLCDeframer def(4, 400);
    auto frames = def.work(bits.data(), bits.size());
    REQUIRE(frames.size() == 1);
    CHECK(frames[0] == data);
}

TEST_CASE("HDLCDeframer handles shared and repeated flags between frames", "[common][codings][deframing][hdlc]")
{
    std::vector<std::vector<uint8_t>> sent;
    std::vector<uint8_t> bits;
    appendFlag(bits);
    for (int i = 0; i < 10; i++)
    {
        sent.push_back(randomBytes(10 + i * 7, 100 + i));
        appendFrame(bits, sent.back());
        for (int f = 0; f < 1 + i % 3; f++) // one shared flag, or several
            appendFlag(bits);
    }

    ax25::HDLCDeframer def(4, 400);
    auto frames = def.work(bits.data(), bits.size());
    CHECK(frames == sent);
}

TEST_CASE("HDLCDeframer removes stuffed bits from data full of ones and flag bytes", "[common][codings][deframing][hdlc]")
{
    std::vector<uint8_t> data = {0xFF, 0xFF, 0x7E, 0x7E, 0xFE, 0x7F, 0x3F, 0x1F, 0xF8, 0x00, 0xFF};
    std::vector<uint8_t> bits;
    appendFlag(bits);
    appendFrame(bits, data);
    appendFlag(bits);

    ax25::HDLCDeframer def(4, 100);
    auto frames = def.work(bits.data(), bits.size());
    REQUIRE(frames.size() == 1);
    CHECK(frames[0] == data);
}

TEST_CASE("HDLCDeframer drops frames with a bad FCS and keeps going", "[common][codings][deframing][hdlc]")
{
    std::vector<uint8_t> good1 = randomBytes(30, 5), bad = randomBytes(30, 6), good2 = randomBytes(30, 7);
    std::vector<uint8_t> bits;
    appendFlag(bits);
    appendFrame(bits, good1);
    appendFlag(bits);
    appendFrame(bits, bad, true);
    appendFlag(bits);
    appendFrame(bits, good2);
    appendFlag(bits);

    ax25::HDLCDeframer def(4, 400);
    auto frames = def.work(bits.data(), bits.size());
    REQUIRE(frames.size() == 2);
    CHECK(frames[0] == good1);
    CHECK(frames[1] == good2);
}

TEST_CASE("HDLCDeframer drops frames shorter than the minimum length", "[common][codings][deframing][hdlc]")
{
    // The minimum length counts the 2 FCS bytes
    std::vector<uint8_t> shortFrame = randomBytes(3, 8), okFrame = randomBytes(4, 9);
    std::vector<uint8_t> bits;
    appendFlag(bits);
    appendFrame(bits, shortFrame);
    appendFlag(bits);
    appendFrame(bits, okFrame);
    appendFlag(bits);

    ax25::HDLCDeframer def(6, 100);
    auto frames = def.work(bits.data(), bits.size());
    REQUIRE(frames.size() == 1);
    CHECK(frames[0] == okFrame);
}

TEST_CASE("HDLCDeframer keeps state across calls, even one bit at a time", "[common][codings][deframing][hdlc]")
{
    std::vector<std::vector<uint8_t>> sent;
    std::vector<uint8_t> bits;
    appendFlag(bits);
    for (int i = 0; i < 5; i++)
    {
        sent.push_back(randomBytes(40, 200 + i));
        appendFrame(bits, sent.back());
        appendFlag(bits);
    }

    ax25::HDLCDeframer def(4, 400);
    std::vector<std::vector<uint8_t>> got;
    for (size_t i = 0; i < bits.size(); i++)
        for (auto &f : def.work(&bits[i], 1))
            got.push_back(f);
    CHECK(got == sent);
}

TEST_CASE("HDLCDeframer outputs nothing for flags only or an unterminated frame", "[common][codings][deframing][hdlc]")
{
    std::vector<uint8_t> bits;
    for (int i = 0; i < 20; i++)
        appendFlag(bits);
    appendFrame(bits, randomBytes(25, 10)); // no closing flag

    ax25::HDLCDeframer def(4, 400);
    CHECK(def.work(bits.data(), bits.size()).empty());
    CHECK(def.work(bits.data(), 0).empty());
}
