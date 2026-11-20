#include "common/ccsds/ccsds.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace
{
    // Bit-by-bit CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection, no final XOR)
    uint16_t referenceCrc16CcittFalse(const std::vector<uint8_t> &data)
    {
        uint16_t crc = 0xFFFF;
        for (uint8_t byte : data)
        {
            crc ^= byte << 8;
            for (int i = 0; i < 8; i++)
                crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
        }
        return crc;
    }

    // Builds a packet from explicit header fields and payload, header encoded with encodeHDR()
    ccsds::CCSDSPacket makePacket(uint16_t apid, uint16_t count, const std::vector<uint8_t> &payload)
    {
        ccsds::CCSDSPacket pkt;
        pkt.header.version = 0;
        pkt.header.type = 0;
        pkt.header.secondary_header_flag = 1;
        pkt.header.apid = apid;
        pkt.header.sequence_flag = 3;
        pkt.header.packet_sequence_count = count;
        pkt.payload = payload;
        pkt.encodeHDR();
        return pkt;
    }

    std::vector<uint8_t> headerAndPayloadWithoutTrailer(const ccsds::CCSDSPacket &pkt, int trailer)
    {
        std::vector<uint8_t> all(pkt.header.raw, pkt.header.raw + 6);
        all.insert(all.end(), pkt.payload.begin(), pkt.payload.end() - trailer);
        return all;
    }
} // namespace

TEST_CASE("CCSDSHeader parses every field of the primary header", "[common][ccsds]")
{
    // version 0, type 0, sec hdr 1, APID 0x123, seq flags 0b11, count 0x2ABC, length 0x0102
    uint8_t raw[6] = {0x09, 0x23, 0xEA, 0xBC, 0x01, 0x02};
    ccsds::CCSDSHeader hdr(raw);

    CHECK(hdr.version == 0);
    CHECK(hdr.type == false);
    CHECK(hdr.secondary_header_flag == true);
    CHECK(hdr.apid == 0x123);
    CHECK(hdr.sequence_flag == 3);
    CHECK(hdr.packet_sequence_count == 0x2ABC);
    CHECK(hdr.packet_length == 0x0102);
    for (int i = 0; i < 6; i++)
        CHECK(hdr.raw[i] == raw[i]);

    ccsds::CCSDSHeader viaFunction = ccsds::parseCCSDSHeader(raw);
    CHECK(viaFunction.apid == 0x123);
    CHECK(viaFunction.packet_sequence_count == 0x2ABC);
}

TEST_CASE("CCSDSHeader parses all-ones and all-zeros headers", "[common][ccsds]")
{
    SECTION("all ones gives the maximum of every field")
    {
        uint8_t raw[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        ccsds::CCSDSHeader hdr(raw);
        CHECK(hdr.version == 7);
        CHECK(hdr.type == true);
        CHECK(hdr.secondary_header_flag == true);
        CHECK(hdr.apid == 0x7FF);
        CHECK(hdr.sequence_flag == 3);
        CHECK(hdr.packet_sequence_count == 0x3FFF);
        CHECK(hdr.packet_length == 0xFFFF);
    }

    SECTION("all zeros")
    {
        uint8_t raw[6] = {0, 0, 0, 0, 0, 0};
        ccsds::CCSDSHeader hdr(raw);
        CHECK(hdr.version == 0);
        CHECK(hdr.type == false);
        CHECK(hdr.secondary_header_flag == false);
        CHECK(hdr.apid == 0);
        CHECK(hdr.sequence_flag == 0);
        CHECK(hdr.packet_sequence_count == 0);
        CHECK(hdr.packet_length == 0);
    }

    SECTION("single bits land in the right fields")
    {
        uint8_t typeBit[6] = {0x10, 0, 0, 0, 0, 0};
        CHECK(ccsds::CCSDSHeader(typeBit).type == true);
        CHECK(ccsds::CCSDSHeader(typeBit).apid == 0);

        uint8_t apidMsb[6] = {0x04, 0, 0, 0, 0, 0};
        CHECK(ccsds::CCSDSHeader(apidMsb).apid == 0x400);
        CHECK(ccsds::CCSDSHeader(apidMsb).secondary_header_flag == false);

        uint8_t countMsb[6] = {0, 0, 0x20, 0, 0, 0};
        CHECK(ccsds::CCSDSHeader(countMsb).packet_sequence_count == 0x2000);
        CHECK(ccsds::CCSDSHeader(countMsb).sequence_flag == 0);

        uint8_t seqFlag[6] = {0, 0, 0x40, 0, 0, 0};
        CHECK(ccsds::CCSDSHeader(seqFlag).sequence_flag == 1);
        CHECK(ccsds::CCSDSHeader(seqFlag).packet_sequence_count == 0);

        uint8_t version[6] = {0x20, 0, 0, 0, 0, 0};
        CHECK(ccsds::CCSDSHeader(version).version == 1);
    }
}

TEST_CASE("CCSDSHeader encodeHDR round-trips through the parser", "[common][ccsds]")
{
    ccsds::CCSDSHeader hdr;
    hdr.version = 0;
    hdr.type = 1;
    hdr.secondary_header_flag = 0;
    hdr.apid = 0x7FE;
    hdr.sequence_flag = 2;
    hdr.packet_sequence_count = 0x1234;
    hdr.packet_length = 0xBEEF;
    hdr.encodeHDR();

    CHECK(hdr.raw[0] == 0x17);
    CHECK(hdr.raw[1] == 0xFE);
    CHECK(hdr.raw[2] == 0x92);
    CHECK(hdr.raw[3] == 0x34);
    CHECK(hdr.raw[4] == 0xBE);
    CHECK(hdr.raw[5] == 0xEF);

    ccsds::CCSDSHeader parsed(hdr.raw);
    CHECK(parsed.version == hdr.version);
    CHECK(parsed.type == hdr.type);
    CHECK(parsed.secondary_header_flag == hdr.secondary_header_flag);
    CHECK(parsed.apid == hdr.apid);
    CHECK(parsed.sequence_flag == hdr.sequence_flag);
    CHECK(parsed.packet_sequence_count == hdr.packet_sequence_count);
    CHECK(parsed.packet_length == hdr.packet_length);
}

TEST_CASE("CCSDSHeader encodeHDR masks out-of-range fields to their bit width", "[common][ccsds]")
{
    ccsds::CCSDSHeader hdr;
    hdr.version = 0xFF;
    hdr.type = 0;
    hdr.secondary_header_flag = 0;
    hdr.apid = 0xFFFF;
    hdr.sequence_flag = 0xFF;
    hdr.packet_sequence_count = 0xFFFF;
    hdr.packet_length = 0;
    hdr.encodeHDR();

    ccsds::CCSDSHeader parsed(hdr.raw);
    CHECK(parsed.version == 7);
    CHECK(parsed.type == false);
    CHECK(parsed.secondary_header_flag == false);
    CHECK(parsed.apid == 0x7FF);
    CHECK(parsed.sequence_flag == 3);
    CHECK(parsed.packet_sequence_count == 0x3FFF);
}

TEST_CASE("CCSDSPacket encodeHDR sets the length field to payload size minus one", "[common][ccsds]")
{
    ccsds::CCSDSPacket pkt = makePacket(42, 7, std::vector<uint8_t>(100, 0xAA));
    CHECK(pkt.header.packet_length == 99);
    CHECK(pkt.header.raw[4] == 0x00);
    CHECK(pkt.header.raw[5] == 99);

    pkt.payload.resize(1);
    pkt.encodeHDR();
    CHECK(pkt.header.packet_length == 0);

    pkt.payload.resize(65536);
    pkt.encodeHDR();
    CHECK(pkt.header.packet_length == 0xFFFF);
    CHECK(pkt.header.raw[4] == 0xFF);
    CHECK(pkt.header.raw[5] == 0xFF);
}

TEST_CASE("CCSDSHeader and CCSDSPacket copy and assignment copy everything", "[common][ccsds]")
{
    ccsds::CCSDSPacket original = makePacket(0x55, 0x1FFF, {1, 2, 3, 4});

    ccsds::CCSDSPacket copy(original);
    CHECK(copy.header.apid == 0x55);
    CHECK(copy.header.packet_sequence_count == 0x1FFF);
    CHECK(copy.header.packet_length == 3);
    CHECK(copy.payload == original.payload);
    for (int i = 0; i < 6; i++)
        CHECK(copy.header.raw[i] == original.header.raw[i]);

    ccsds::CCSDSPacket assigned;
    assigned = original;
    CHECK(assigned.header.apid == 0x55);
    CHECK(assigned.payload == original.payload);

    // Copies are independent
    copy.payload[0] = 99;
    CHECK(original.payload[0] == 1);

    // Self-assignment is a no-op
    ccsds::CCSDSPacket &self = assigned;
    assigned = self;
    CHECK(assigned.payload == original.payload);

    ccsds::CCSDSHeader hdrCopy;
    hdrCopy = original.header;
    CHECK(hdrCopy.apid == 0x55);
    CHECK(hdrCopy.sequence_flag == 3);
    CHECK(hdrCopy.secondary_header_flag == true);
}

TEST_CASE("crcCheckCCITT accepts a correct CRC-16/CCITT-FALSE trailer and rejects corruption", "[common][ccsds][crc]")
{
    std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x00, 0x00};
    ccsds::CCSDSPacket pkt = makePacket(0x100, 1, payload);

    uint16_t crc = referenceCrc16CcittFalse(headerAndPayloadWithoutTrailer(pkt, 2));
    pkt.payload[pkt.payload.size() - 2] = crc >> 8;
    pkt.payload[pkt.payload.size() - 1] = crc & 0xFF;
    CHECK(ccsds::crcCheckCCITT(pkt));

    SECTION("flipping a payload bit is detected")
    {
        pkt.payload[0] ^= 0x01;
        CHECK_FALSE(ccsds::crcCheckCCITT(pkt));
    }

    SECTION("flipping a header bit is detected, the header is covered")
    {
        pkt.header.raw[1] ^= 0x80;
        CHECK_FALSE(ccsds::crcCheckCCITT(pkt));
    }

    SECTION("byte-swapped CRC is rejected")
    {
        std::swap(pkt.payload[pkt.payload.size() - 2], pkt.payload[pkt.payload.size() - 1]);
        if (pkt.payload[pkt.payload.size() - 2] != pkt.payload[pkt.payload.size() - 1])
            CHECK_FALSE(ccsds::crcCheckCCITT(pkt));
    }
}

TEST_CASE("crcCheckCCITT with only the CRC in the payload covers just the header", "[common][ccsds][crc]")
{
    ccsds::CCSDSPacket pkt = makePacket(0x7FF, 0, {0, 0});
    uint16_t crc = referenceCrc16CcittFalse(std::vector<uint8_t>(pkt.header.raw, pkt.header.raw + 6));
    pkt.payload[0] = crc >> 8;
    pkt.payload[1] = crc & 0xFF;
    CHECK(ccsds::crcCheckCCITT(pkt));
}

// Bug: ccsds.cpp:132 `return crc ^ 0xFFFFFFFF == crc2;` parses as `crc ^ (0xFFFFFFFF == crc2)`,
// so the function returns true for essentially any CRC field. (The loop also mixes the
// reflected polynomial 0xEDB88320 with MSB-first shifting of a 16-bit data word.)
TEST_CASE("crcCheckHLDC32 cannot accept two different CRC values for the same data", "[common][ccsds][crc][!shouldfail]")
{
    std::vector<uint8_t> payload = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0, 0, 0, 0};

    ccsds::CCSDSPacket a = makePacket(0x200, 5, payload);
    a.payload[6] = 0x00;
    a.payload[7] = 0x00;
    a.payload[8] = 0x00;
    a.payload[9] = 0x00;

    ccsds::CCSDSPacket b = makePacket(0x200, 5, payload);
    b.payload[6] = 0x12;
    b.payload[7] = 0x34;
    b.payload[8] = 0x56;
    b.payload[9] = 0x78;

    // Whatever the CRC convention, at most one of these trailers can be right
    CHECK_FALSE((ccsds::crcCheckHLDC32(a) && ccsds::crcCheckHLDC32(b)));
}

TEST_CASE("crcCheckVerticalParity checks the XOR of all 16-bit words", "[common][ccsds][crc]")
{
    // Header (6) + payload without trailer (6) = 12 bytes = 6 words
    std::vector<uint8_t> payload = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0x00, 0x00};
    ccsds::CCSDSPacket pkt = makePacket(0x0AB, 0x0CD, payload);

    uint16_t parity = 0;
    std::vector<uint8_t> covered = headerAndPayloadWithoutTrailer(pkt, 2);
    for (size_t i = 0; i < covered.size(); i += 2)
        parity ^= covered[i] << 8 | covered[i + 1];

    pkt.payload[6] = parity >> 8;
    pkt.payload[7] = parity & 0xFF;
    CHECK(ccsds::crcCheckVerticalParity(pkt));

    SECTION("a single flipped bit is detected")
    {
        pkt.payload[3] ^= 0x10;
        CHECK_FALSE(ccsds::crcCheckVerticalParity(pkt));
    }

    SECTION("two flips in the same bit column cancel out, as expected from a parity check")
    {
        pkt.payload[0] ^= 0x04;
        pkt.payload[2] ^= 0x04;
        CHECK(ccsds::crcCheckVerticalParity(pkt));
    }

    SECTION("wrong checksum is rejected")
    {
        pkt.payload[7] ^= 0xFF;
        CHECK_FALSE(ccsds::crcCheckVerticalParity(pkt));
    }
}

TEST_CASE("crcCheckVerticalParity ignores a trailing odd byte before the checksum", "[common][ccsds][crc]")
{
    // Header (6) + 3 data bytes = 9 bytes; only the first 4 whole words are summed
    ccsds::CCSDSPacket pkt = makePacket(0x001, 0, {0x11, 0x22, 0x33, 0x00, 0x00});
    std::vector<uint8_t> covered = headerAndPayloadWithoutTrailer(pkt, 2);
    uint16_t parity = 0;
    for (size_t i = 0; i + 1 < covered.size(); i += 2)
        parity ^= covered[i] << 8 | covered[i + 1];
    pkt.payload[3] = parity >> 8;
    pkt.payload[4] = parity & 0xFF;
    CHECK(ccsds::crcCheckVerticalParity(pkt));

    pkt.payload[2] ^= 0xFF; // the odd byte is not covered
    CHECK(ccsds::crcCheckVerticalParity(pkt));
}
