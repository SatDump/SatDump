#include "common/ccsds/ccsds_aos/demuxer.h"
#include "../ccsds_stream_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstring>
#include <vector>

namespace
{
    // ASM + AOS primary header (6) + optional insert zone + M_PDU header (2) + data zone
    std::vector<uint8_t> makeAosCadu(const uint8_t *zone, int zoneSize, uint16_t fhp, uint32_t counter, int insertZone = 0)
    {
        std::vector<uint8_t> cadu(12 + insertZone + zoneSize, 0);
        cadu[0] = 0x1A;
        cadu[1] = 0xCF;
        cadu[2] = 0xFC;
        cadu[3] = 0x1D;
        cadu[4] = 0x40 | (0x9D >> 2);
        cadu[5] = (0x9D & 3) << 6 | 5;
        cadu[6] = counter >> 16;
        cadu[7] = counter >> 8;
        cadu[8] = counter;
        cadu[9] = 0;
        for (int i = 0; i < insertZone; i++)
            cadu[10 + i] = 0xEE;
        cadu[10 + insertZone] = (fhp >> 8) & 0b111;
        cadu[11 + insertZone] = fhp & 0xFF;
        if (zone != nullptr)
            std::memcpy(&cadu[12 + insertZone], zone, zoneSize);
        return cadu;
    }

    std::vector<uint8_t> makeAosCaduForZone(const PacketStream &stream, int zone, int zoneSize, int insertZone = 0)
    {
        return makeAosCadu(zoneData(stream, zone, zoneSize), zoneSize, firstHeaderPointer(stream, zone, zoneSize), zone, insertZone);
    }

    std::vector<ccsds::CCSDSPacket> demuxZones(ccsds::ccsds_aos::Demuxer &demux, const PacketStream &stream, int zoneSize, int fromZone, int toZone)
    {
        std::vector<ccsds::CCSDSPacket> out;
        for (int z = fromZone; z < toZone; z++)
        {
            auto cadu = makeAosCaduForZone(stream, z, zoneSize);
            append(out, demux.work(cadu.data()));
        }
        return out;
    }
} // namespace

TEST_CASE("ccsds_aos Demuxer reassembles a random packet stream", "[common][ccsds][aos][demuxer]")
{
    int zoneSize = GENERATE(884, 64, 17);
    uint32_t seed = GENERATE(1u, 2u, 3u);
    INFO("zone size " << zoneSize << ", seed " << seed);

    PacketStream stream = makeRandomStream(seed, 300, zoneSize);
    int zones = zoneCount(stream, zoneSize);
    REQUIRE(zones > 10);

    ccsds::ccsds_aos::Demuxer demux(zoneSize);
    auto out = demuxZones(demux, stream, zoneSize, 0, zones);

    checkPacketsArePrefix(out, stream, 0, (size_t)(zones - 1) * zoneSize);
}

TEST_CASE("ccsds_aos Demuxer extracts several packets and idle fill from one frame", "[common][ccsds][aos][demuxer]")
{
    const int D = 64;
    PacketStream stream;
    uint32_t rng = 42;
    appendPacket(stream, 100, 0, 10, rng);
    appendPacket(stream, 101, 1, 20, rng);
    appendPacket(stream, APID_IDLE, 0, D - 16 - 26 - 6, rng); // fills the zone exactly
    REQUIRE(stream.bytes.size() == (size_t)D);

    ccsds::ccsds_aos::Demuxer demux(D);
    auto cadu = makeAosCaduForZone(stream, 0, D);
    REQUIRE(cadu[11] == 0); // FHP 0
    auto out = demux.work(cadu.data());

    // Idle packets are not filtered by the demuxer
    REQUIRE(out.size() == 3);
    CHECK(samePacket(out[0], stream.packets[0].pkt));
    CHECK(samePacket(out[1], stream.packets[1].pkt));
    CHECK(samePacket(out[2], stream.packets[2].pkt));
    CHECK(out[2].header.apid == APID_IDLE);
}

TEST_CASE("ccsds_aos Demuxer handles a primary header split at every position across frames", "[common][ccsds][aos][demuxer]")
{
    const int D = 64;
    int split = GENERATE(1, 2, 3, 4, 5, 6); // header bytes that land in the first frame
    INFO("header bytes in first frame: " << split);

    PacketStream stream;
    uint32_t rng = 7 + split;
    appendPacket(stream, 1, 0, D - split - 6, rng);            // ends where packet 2 starts
    appendPacket(stream, 2, 1, 10, rng);                       // header straddles the boundary
    appendPacket(stream, 3, 2, D - (16 - split) - 6, rng);     // fills the second zone exactly
    appendPacket(stream, 4, 3, 5, rng);
    appendPacket(stream, APID_IDLE, 4, D - 11 - 6, rng);       // fills the third zone exactly
    REQUIRE(stream.bytes.size() == (size_t)3 * D);
    REQUIRE(firstHeaderPointer(stream, 1, D) == 16 - split);

    ccsds::ccsds_aos::Demuxer demux(D);
    auto out0 = demux.work(makeAosCaduForZone(stream, 0, D).data());
    REQUIRE(out0.size() == 1);
    CHECK(samePacket(out0[0], stream.packets[0].pkt));

    auto out1 = demux.work(makeAosCaduForZone(stream, 1, D).data());
    REQUIRE(out1.size() >= 1);
    CHECK(samePacket(out1[0], stream.packets[1].pkt));

    std::vector<ccsds::CCSDSPacket> all = out0;
    append(all, out1);
    append(all, demux.work(makeAosCaduForZone(stream, 2, D).data()));
    checkPacketsArePrefix(all, stream, 0, 2 * D);
}

TEST_CASE("ccsds_aos Demuxer emits a packet ending exactly at the zone end one frame late", "[common][ccsds][aos][demuxer]")
{
    // When the only packet starting in a zone ends exactly at the end of that
    // zone, it is pushed on the next call to work(), not the current one.
    const int D = 64;
    PacketStream stream;
    uint32_t rng = 3;
    appendPacket(stream, 10, 0, D - 6, rng);
    appendPacket(stream, 11, 1, D - 6, rng);

    ccsds::ccsds_aos::Demuxer demux(D);
    auto out0 = demux.work(makeAosCaduForZone(stream, 0, D).data());
    CHECK(out0.empty());
    auto out1 = demux.work(makeAosCaduForZone(stream, 1, D).data());
    REQUIRE(out1.size() == 1);
    CHECK(samePacket(out1[0], stream.packets[0].pkt));
}

TEST_CASE("ccsds_aos Demuxer reassembles a packet spanning many frames", "[common][ccsds][aos][demuxer]")
{
    const int D = 100;
    PacketStream stream;
    uint32_t rng = 11;
    appendPacket(stream, 0x50, 0, 4 * D + 37, rng);
    appendPacket(stream, 0x51, 1, 20, rng);
    appendPacket(stream, 0x52, 2, 3 * D, rng);
    int zones = zoneCount(stream, D);
    REQUIRE(firstHeaderPointer(stream, 1, D) == FHP_NO_HEADER);
    REQUIRE(firstHeaderPointer(stream, 2, D) == FHP_NO_HEADER);

    ccsds::ccsds_aos::Demuxer demux(D);
    auto out = demuxZones(demux, stream, D, 0, zones);
    REQUIRE(out.size() >= 2);
    CHECK(out[0].payload.size() == (size_t)4 * D + 37);
    checkPacketsArePrefix(out, stream, 0, (size_t)(zones - 1) * D);
}

TEST_CASE("ccsds_aos Demuxer starting mid-stream skips the partial packet", "[common][ccsds][aos][demuxer]")
{
    const int D = 64;
    PacketStream stream = makeRandomStream(99, 200, D);
    int zones = zoneCount(stream, D);
    int startZone = zones / 3;

    ccsds::ccsds_aos::Demuxer demux(D);
    auto out = demuxZones(demux, stream, D, startZone, zones);

    size_t first = firstPacketAtOrAfter(stream, (size_t)startZone * D);
    checkPacketsArePrefix(out, stream, first, (size_t)(zones - 1) * D);
}

TEST_CASE("ccsds_aos Demuxer resynchronises after a missing frame", "[common][ccsds][aos][demuxer]")
{
    const int D = 64;
    uint32_t seed = GENERATE(5u, 6u, 7u, 8u);
    PacketStream stream = makeRandomStream(seed, 200, D);
    int zones = zoneCount(stream, D);
    int gap = GENERATE(10, 23, 41);
    INFO("seed " << seed << ", dropped zone " << gap);

    ccsds::ccsds_aos::Demuxer demux(D);
    auto before = demuxZones(demux, stream, D, 0, gap);
    checkPacketsArePrefix(before, stream, 0, (size_t)(gap - 1) * D);

    auto after = demuxZones(demux, stream, D, gap + 1, zones);
    size_t first = firstPacketAtOrAfter(stream, (size_t)(gap + 1) * D);

    // The packet in progress across the gap may come out once, corrupted, then everything is clean again
    if (!after.empty() && first < stream.packets.size() && !samePacket(after[0], stream.packets[first].pkt))
        after.erase(after.begin());
    checkPacketsArePrefix(after, stream, first, (size_t)(zones - 1) * D);
}

TEST_CASE("ccsds_aos Demuxer ignores frames with an out-of-range or idle first header pointer", "[common][ccsds][aos][demuxer]")
{
    const int D = 64;
    PacketStream stream = makeRandomStream(1234, 150, D);
    int zones = zoneCount(stream, D);

    ccsds::ccsds_aos::Demuxer reference(D);
    auto expected = demuxZones(reference, stream, D, 0, zones);

    ccsds::ccsds_aos::Demuxer demux(D);
    std::vector<ccsds::CCSDSPacket> out;
    std::vector<uint8_t> junk(D, 0x5A);
    for (int z = 0; z < zones; z++)
    {
        if (z % 5 == 2)
        {
            // FHP pointing past the data zone, and the idle-data marker: both produce nothing
            CHECK(demux.work(makeAosCadu(junk.data(), D, D, 0).data()).empty());
            CHECK(demux.work(makeAosCadu(junk.data(), D, 2000, 0).data()).empty());
            CHECK(demux.work(makeAosCadu(junk.data(), D, FHP_IDLE, 0).data()).empty());
        }
        append(out, demux.work(makeAosCaduForZone(stream, z, D).data()));
    }

    REQUIRE(out.size() == expected.size());
    for (size_t i = 0; i < out.size(); i++)
        REQUIRE(samePacket(out[i], expected[i]));
}

TEST_CASE("ccsds_aos Demuxer with secondaryHeaderExtendsPkt adds 8 bytes to flagged packets", "[common][ccsds][aos][demuxer]")
{
    const int D = 80;
    PacketStream stream;
    uint32_t rng = 77;
    for (int i = 0; i < 60; i++)
    {
        bool flagged = i % 2 == 0;
        size_t size = 9 + nextRand(rng) % 150;
        appendPacket(stream, i, i, size, rng, flagged, flagged ? 8 : 0);
    }
    int zones = zoneCount(stream, D);

    ccsds::ccsds_aos::Demuxer demux(D, false, 2, true);
    std::vector<ccsds::CCSDSPacket> out;
    for (int z = 0; z < zones; z++)
        append(out, demux.work(makeAosCaduForZone(stream, z, D).data()));

    checkPacketsArePrefix(out, stream, 0, (size_t)(zones - 1) * D);
    REQUIRE(out.size() > 2);
    CHECK(out[0].payload.size() == out[0].header.packet_length + 1u + 8u);
    CHECK(out[1].payload.size() == out[1].header.packet_length + 1u);
}

TEST_CASE("ccsds_aos Demuxer reads the M_PDU after a VCDU insert zone", "[common][ccsds][aos][demuxer]")
{
    const int D = 64;
    PacketStream stream = makeRandomStream(555, 100, D);
    int zones = zoneCount(stream, D);

    ccsds::ccsds_aos::Demuxer demux(D, true, 3);
    std::vector<ccsds::CCSDSPacket> out;
    for (int z = 0; z < zones; z++)
        append(out, demux.work(makeAosCaduForZone(stream, z, D, 3).data()));

    checkPacketsArePrefix(out, stream, 0, (size_t)(zones - 1) * D);
}
