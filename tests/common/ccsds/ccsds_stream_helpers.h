#pragma once

// Shared helpers for the AOS and TM demuxer tests: build a continuous stream of
// CCSDS space packets, then cut it into fixed-size M_PDU data zones with a
// First Header Pointer computed the way the CCSDS standards define it.

#include "common/ccsds/ccsds.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <vector>

namespace
{
    constexpr uint16_t FHP_NO_HEADER = 2047; // No packet header starts in this data zone
    constexpr uint16_t FHP_IDLE = 2046;      // Data zone contains only idle data (TM)
    constexpr uint16_t APID_IDLE = 0x7FF;

    struct StreamPacket
    {
        ccsds::CCSDSPacket pkt;
        size_t start; // Offset of the primary header in the stream
        size_t end;   // One past the last byte
    };

    struct PacketStream
    {
        std::vector<uint8_t> bytes;
        std::vector<StreamPacket> packets;
    };

    uint32_t nextRand(uint32_t &state)
    {
        state = state * 1664525u + 1013904223u;
        return state >> 8;
    }

    // Appends one packet. payloadSize is the number of bytes that follow the
    // 6-byte primary header. extraLength is subtracted from the length field
    // (used for the "secondary header extends packet" mode).
    void appendPacket(PacketStream &stream, uint16_t apid, uint16_t count, size_t payloadSize, uint32_t &rng,
                      bool secondaryHeader = false, int extraLength = 0)
    {
        StreamPacket sp;
        sp.pkt.header.version = 0;
        sp.pkt.header.type = 0;
        sp.pkt.header.secondary_header_flag = secondaryHeader;
        sp.pkt.header.apid = apid;
        sp.pkt.header.sequence_flag = 3;
        sp.pkt.header.packet_sequence_count = count & 0x3FFF;
        sp.pkt.payload.resize(payloadSize);
        for (auto &b : sp.pkt.payload)
            b = nextRand(rng);
        sp.pkt.header.packet_length = payloadSize - 1 - extraLength;
        sp.pkt.header.encodeHDR();

        sp.start = stream.bytes.size();
        stream.bytes.insert(stream.bytes.end(), sp.pkt.header.raw, sp.pkt.header.raw + 6);
        stream.bytes.insert(stream.bytes.end(), sp.pkt.payload.begin(), sp.pkt.payload.end());
        sp.end = stream.bytes.size();
        stream.packets.push_back(sp);
    }

    // Random mixture of tiny, medium and multi-zone packets
    PacketStream makeRandomStream(uint32_t seed, int count, int zoneSize)
    {
        PacketStream stream;
        uint32_t rng = seed;
        for (int i = 0; i < count; i++)
        {
            int kind = nextRand(rng) % 10;
            size_t size;
            if (kind < 3)
                size = 1 + nextRand(rng) % 10;
            else if (kind < 7)
                size = 1 + nextRand(rng) % zoneSize;
            else
                size = 1 + nextRand(rng) % (3 * zoneSize);
            appendPacket(stream, nextRand(rng) % 2047, i, size, rng);
        }
        return stream;
    }

    int zoneCount(const PacketStream &stream, int zoneSize) { return stream.bytes.size() / zoneSize; }

    // First Header Pointer of data zone `zone`: offset of the first header starting in it
    uint16_t firstHeaderPointer(const PacketStream &stream, int zone, int zoneSize)
    {
        size_t begin = (size_t)zone * zoneSize;
        for (const auto &p : stream.packets)
            if (p.start >= begin && p.start < begin + zoneSize)
                return p.start - begin;
        return FHP_NO_HEADER;
    }

    const uint8_t *zoneData(const PacketStream &stream, int zone, int zoneSize) { return &stream.bytes[(size_t)zone * zoneSize]; }

    bool samePacket(const ccsds::CCSDSPacket &a, const ccsds::CCSDSPacket &b)
    {
        return std::memcmp(a.header.raw, b.header.raw, 6) == 0 && a.header.apid == b.header.apid &&
               a.header.packet_sequence_count == b.header.packet_sequence_count &&
               a.header.packet_length == b.header.packet_length && a.payload == b.payload;
    }

    // Checks that `out` is exactly stream.packets[first ...] (in order), and that
    // at least every packet ending before `guaranteedEnd` was produced.
    void checkPacketsArePrefix(const std::vector<ccsds::CCSDSPacket> &out, const PacketStream &stream, size_t first, size_t guaranteedEnd)
    {
        size_t mustHave = 0;
        for (size_t i = first; i < stream.packets.size(); i++)
            if (stream.packets[i].end <= guaranteedEnd)
                mustHave = i - first + 1;

        REQUIRE(out.size() >= mustHave);
        REQUIRE(first + out.size() <= stream.packets.size());
        for (size_t i = 0; i < out.size(); i++)
        {
            INFO("output packet " << i << " (stream packet " << first + i << ")");
            REQUIRE(samePacket(out[i], stream.packets[first + i].pkt));
        }
    }

    // Index of the first stream packet whose header starts at or after `offset`
    size_t firstPacketAtOrAfter(const PacketStream &stream, size_t offset)
    {
        for (size_t i = 0; i < stream.packets.size(); i++)
            if (stream.packets[i].start >= offset)
                return i;
        return stream.packets.size();
    }

    void append(std::vector<ccsds::CCSDSPacket> &dst, const std::vector<ccsds::CCSDSPacket> &src) { dst.insert(dst.end(), src.begin(), src.end()); }
} // namespace
