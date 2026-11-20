#include "common/ccsds/ccsds_tm/demuxer.h"
#include "../ccsds_stream_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstring>
#include <vector>

namespace
{
    // ASM + TM primary header (6, FHP in the last 11 bits) + optional frame
    // secondary header + optional M_PDU insert zone + data zone
    std::vector<uint8_t> makeTmCadu(const uint8_t *zone, int zoneSize, uint16_t fhp, uint8_t counter, int secondaryHeader = 0, int mpduInsertZone = 0)
    {
        std::vector<uint8_t> cadu(10 + secondaryHeader + mpduInsertZone + zoneSize, 0);
        cadu[0] = 0x1A;
        cadu[1] = 0xCF;
        cadu[2] = 0xFC;
        cadu[3] = 0x1D;
        cadu[4] = 0x2A5 >> 4;
        cadu[5] = (0x2A5 & 0xF) << 4 | 3 << 1;
        cadu[6] = counter;
        cadu[7] = counter;
        for (int i = 0; i < secondaryHeader; i++)
            cadu[8 + i] = 0xEE;
        cadu[8 + secondaryHeader] = 0x18 | ((fhp >> 8) & 0b111);
        cadu[9 + secondaryHeader] = fhp & 0xFF;
        for (int i = 0; i < mpduInsertZone; i++)
            cadu[10 + secondaryHeader + i] = 0xDD;
        if (zone != nullptr)
            std::memcpy(&cadu[10 + secondaryHeader + mpduInsertZone], zone, zoneSize);
        return cadu;
    }

    std::vector<uint8_t> makeTmCaduForZone(const PacketStream &stream, int zone, int zoneSize, int secondaryHeader = 0, int mpduInsertZone = 0)
    {
        return makeTmCadu(zoneData(stream, zone, zoneSize), zoneSize, firstHeaderPointer(stream, zone, zoneSize), zone, secondaryHeader, mpduInsertZone);
    }

    std::vector<ccsds::CCSDSPacket> demuxTmZones(ccsds::ccsds_tm::Demuxer &demux, const PacketStream &stream, int zoneSize, int fromZone, int toZone)
    {
        std::vector<ccsds::CCSDSPacket> out;
        for (int z = fromZone; z < toZone; z++)
            append(out, demux.work(makeTmCaduForZone(stream, z, zoneSize).data()));
        return out;
    }
} // namespace

TEST_CASE("ccsds_tm Demuxer reassembles a random packet stream", "[common][ccsds][tm][demuxer]")
{
    int zoneSize = GENERATE(1105, 884, 64, 13);
    uint32_t seed = GENERATE(10u, 20u, 30u);
    INFO("zone size " << zoneSize << ", seed " << seed);

    PacketStream stream = makeRandomStream(seed, 300, zoneSize);
    int zones = zoneCount(stream, zoneSize);
    REQUIRE(zones > 10);

    ccsds::ccsds_tm::Demuxer demux(zoneSize);
    auto out = demuxTmZones(demux, stream, zoneSize, 0, zones);
    checkPacketsArePrefix(out, stream, 0, (size_t)(zones - 1) * zoneSize);
}

TEST_CASE("ccsds_tm Demuxer extracts back-to-back packets from one frame", "[common][ccsds][tm][demuxer]")
{
    const int D = 100;
    PacketStream stream;
    uint32_t rng = 1;
    for (int i = 0; i < 10; i++)
        appendPacket(stream, 0x20 + i, i, 4, rng); // 10 x 10 bytes = exactly one zone
    REQUIRE(stream.bytes.size() == (size_t)D);

    ccsds::ccsds_tm::Demuxer demux(D);
    auto out = demux.work(makeTmCaduForZone(stream, 0, D).data());
    REQUIRE(out.size() == 10);
    for (int i = 0; i < 10; i++)
        CHECK(samePacket(out[i], stream.packets[i].pkt));
}

TEST_CASE("ccsds_tm Demuxer handles a primary header split at every position across frames", "[common][ccsds][tm][demuxer]")
{
    const int D = 50;
    int split = GENERATE(1, 2, 3, 4, 5, 6);
    INFO("header bytes in first frame: " << split);

    PacketStream stream;
    uint32_t rng = 100 + split;
    appendPacket(stream, 1, 0, D - split - 6, rng);
    appendPacket(stream, 2, 1, 3 * D, rng); // header split, then spans several zones
    appendPacket(stream, 3, 2, 7, rng);
    appendPacket(stream, 4, 3, 2 * D, rng);
    int zones = zoneCount(stream, D);

    ccsds::ccsds_tm::Demuxer demux(D);
    auto out = demuxTmZones(demux, stream, D, 0, zones);
    REQUIRE(out.size() >= 3);
    checkPacketsArePrefix(out, stream, 0, (size_t)(zones - 1) * D);
}

TEST_CASE("ccsds_tm Demuxer skips idle frames without losing state", "[common][ccsds][tm][demuxer]")
{
    const int D = 1105;
    PacketStream stream = makeRandomStream(4242, 200, D);
    int zones = zoneCount(stream, D);

    ccsds::ccsds_tm::Demuxer demux(D);
    std::vector<ccsds::CCSDSPacket> out;
    std::vector<uint8_t> idle(D, 0x55);
    for (int z = 0; z < zones; z++)
    {
        // Idle frame (FHP = 0b11111111110) between every data frame, mid-packet included
        CHECK(demux.work(makeTmCadu(idle.data(), D, FHP_IDLE, 0).data()).empty());
        append(out, demux.work(makeTmCaduForZone(stream, z, D).data()));
    }
    checkPacketsArePrefix(out, stream, 0, (size_t)(zones - 1) * D);
}

TEST_CASE("ccsds_tm Demuxer ignores frames whose FHP points outside the data zone", "[common][ccsds][tm][demuxer]")
{
    const int D = 64;
    PacketStream stream = makeRandomStream(31337, 150, D);
    int zones = zoneCount(stream, D);

    ccsds::ccsds_tm::Demuxer demux(D);
    std::vector<ccsds::CCSDSPacket> out;
    std::vector<uint8_t> junk(D, 0xA5);
    for (int z = 0; z < zones; z++)
    {
        if (z % 4 == 1)
        {
            CHECK(demux.work(makeTmCadu(junk.data(), D, D, 0).data()).empty());
            CHECK(demux.work(makeTmCadu(junk.data(), D, 1000, 0).data()).empty());
        }
        append(out, demux.work(makeTmCaduForZone(stream, z, D).data()));
    }
    checkPacketsArePrefix(out, stream, 0, (size_t)(zones - 1) * D);
}

TEST_CASE("ccsds_tm Demuxer resynchronises after a missing frame", "[common][ccsds][tm][demuxer]")
{
    const int D = 64;
    uint32_t seed = GENERATE(50u, 51u, 52u, 53u);
    int gap = GENERATE(12, 30);
    INFO("seed " << seed << ", dropped zone " << gap);

    PacketStream stream = makeRandomStream(seed, 200, D);
    int zones = zoneCount(stream, D);

    ccsds::ccsds_tm::Demuxer demux(D);
    auto before = demuxTmZones(demux, stream, D, 0, gap);
    checkPacketsArePrefix(before, stream, 0, (size_t)(gap - 1) * D);

    auto after = demuxTmZones(demux, stream, D, gap + 1, zones);
    size_t first = firstPacketAtOrAfter(stream, (size_t)(gap + 1) * D);
    if (!after.empty() && first < stream.packets.size() && !samePacket(after[0], stream.packets[first].pkt))
        after.erase(after.begin());
    checkPacketsArePrefix(after, stream, first, (size_t)(zones - 1) * D);
}

TEST_CASE("ccsds_tm Demuxer honours the frame secondary header and M_PDU insert zone", "[common][ccsds][tm][demuxer]")
{
    const int D = 64;
    PacketStream stream = makeRandomStream(8080, 120, D);
    int zones = zoneCount(stream, D);

    ccsds::ccsds_tm::Demuxer demux(D, true, 4, 2);
    std::vector<ccsds::CCSDSPacket> out;
    for (int z = 0; z < zones; z++)
        append(out, demux.work(makeTmCaduForZone(stream, z, D, 4, 2).data()));
    checkPacketsArePrefix(out, stream, 0, (size_t)(zones - 1) * D);
}

TEST_CASE("ccsds_tm Demuxer with secondaryHeaderExtendsPkt adds 8 bytes to flagged packets", "[common][ccsds][tm][demuxer]")
{
    const int D = 90;
    PacketStream stream;
    uint32_t rng = 9;
    for (int i = 0; i < 50; i++)
    {
        bool flagged = i % 3 != 0;
        appendPacket(stream, i, i, 9 + nextRand(rng) % 200, rng, flagged, flagged ? 8 : 0);
    }
    int zones = zoneCount(stream, D);

    ccsds::ccsds_tm::Demuxer demux(D, false, 2, 0, true);
    auto out = demuxTmZones(demux, stream, D, 0, zones);
    checkPacketsArePrefix(out, stream, 0, (size_t)(zones - 1) * D);
    REQUIRE(out.size() > 2);
}

TEST_CASE("ccsds_tm Demuxer without secondaryHeaderExtendsPkt follows the length field only", "[common][ccsds][tm][demuxer]")
{
    const int D = 100;
    PacketStream stream;
    uint32_t rng = 19;
    for (int i = 0; i < 30; i++)
        appendPacket(stream, i, i, 20, rng, true); // secondary header flag set, normal length
    int zones = zoneCount(stream, D);

    ccsds::ccsds_tm::Demuxer demux(D);
    auto out = demuxTmZones(demux, stream, D, 0, zones);
    checkPacketsArePrefix(out, stream, 0, (size_t)(zones - 1) * D);
    for (auto &p : out)
        CHECK(p.payload.size() == 20);
}
