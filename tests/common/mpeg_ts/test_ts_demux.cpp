#include "common/mpeg_ts/ts_demux.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
    // 188 byte TS packet; payload bytes are filled with an incrementing counter from `fill`
    std::vector<uint8_t> tsPacket(int pid, bool pusi, int afc, uint8_t fill)
    {
        std::vector<uint8_t> pkt(188);
        pkt[0] = 0x47;
        pkt[1] = (pusi ? 0x40 : 0x00) | ((pid >> 8) & 0x1F);
        pkt[2] = pid & 0xFF;
        pkt[3] = (afc << 4);
        for (int i = 4; i < 188; i++)
            pkt[i] = fill + i;
        return pkt;
    }

    std::vector<uint8_t> slice(const std::vector<uint8_t> &v, int start, int end) { return std::vector<uint8_t>(v.begin() + start, v.begin() + end); }
} // namespace

TEST_CASE("mpeg_ts::TSDemux reassembles a section spanning packets", "[common][mpeg_ts][ts_demux]")
{
    mpeg_ts::TSDemux demux;

    auto p1 = tsPacket(0x100, true, 0b01, 0);
    p1[4] = 0; // Pointer field: section starts right away
    auto p2 = tsPacket(0x100, false, 0b01, 50);
    auto p3 = tsPacket(0x100, true, 0b01, 100);
    p3[4] = 10; // 10 bytes finish the previous section

    CHECK(demux.demux(p1.data()).empty());
    CHECK(demux.demux(p2.data()).empty());
    auto out = demux.demux(p3.data());

    REQUIRE(out.size() == 1);
    std::vector<uint8_t> expected = slice(p1, 5, 188);
    auto p2pay = slice(p2, 4, 188);
    auto p3pay = slice(p3, 5, 15);
    expected.insert(expected.end(), p2pay.begin(), p2pay.end());
    expected.insert(expected.end(), p3pay.begin(), p3pay.end());
    CHECK(out[0].size() == 183 + 184 + 10);
    CHECK(out[0] == expected);

    // The next section started after the pointer and is emitted on the next PUSI
    auto p4 = tsPacket(0x100, true, 0b01, 200);
    p4[4] = 0;
    out = demux.demux(p4.data());
    REQUIRE(out.size() == 1);
    CHECK(out[0] == slice(p3, 15, 188));
}

TEST_CASE("mpeg_ts::TSDemux ignores continuation packets before a section start", "[common][mpeg_ts][ts_demux]")
{
    mpeg_ts::TSDemux demux;
    auto cont = tsPacket(0x100, false, 0b01, 0);
    CHECK(demux.demux(cont.data()).empty());

    auto start = tsPacket(0x100, true, 0b01, 0);
    start[4] = 0;
    CHECK(demux.demux(start.data()).empty());
    auto next = tsPacket(0x100, true, 0b01, 0);
    next[4] = 0;
    auto out = demux.demux(next.data());
    REQUIRE(out.size() == 1);
    CHECK(out[0] == slice(start, 5, 188));
}

TEST_CASE("mpeg_ts::TSDemux filters by PID", "[common][mpeg_ts][ts_demux]")
{
    mpeg_ts::TSDemux demux;
    auto a = tsPacket(0x100, true, 0b01, 0);
    a[4] = 0;
    auto other = tsPacket(0x200, false, 0b01, 77);
    auto b = tsPacket(0x100, true, 0b01, 0);
    b[4] = 0;

    CHECK(demux.demux(a.data(), 0x100).empty());
    CHECK(demux.demux(other.data(), 0x100).empty());
    auto out = demux.demux(b.data(), 0x100);
    REQUIRE(out.size() == 1);
    CHECK(out[0] == slice(a, 5, 188)); // Nothing from PID 0x200 mixed in
}

TEST_CASE("mpeg_ts::TSDemux handles adaptation fields", "[common][mpeg_ts][ts_demux]")
{
    mpeg_ts::TSDemux demux;

    auto start = tsPacket(0x100, true, 0b01, 0);
    start[4] = 0;
    demux.demux(start.data());

    // Adaptation only: no payload added
    auto af_only = tsPacket(0x100, false, 0b10, 30);
    CHECK(demux.demux(af_only.data()).empty());

    // Adaptation field of 7 bytes followed by payload
    auto af_payload = tsPacket(0x100, false, 0b11, 60);
    af_payload[4] = 7;
    CHECK(demux.demux(af_payload.data()).empty());

    auto end = tsPacket(0x100, true, 0b01, 0);
    end[4] = 0;
    auto out = demux.demux(end.data());

    REQUIRE(out.size() == 1);
    std::vector<uint8_t> expected = slice(start, 5, 188);
    auto pay = slice(af_payload, 4 + 8, 188);
    expected.insert(expected.end(), pay.begin(), pay.end());
    CHECK(out[0] == expected);
}

TEST_CASE("mpeg_ts::TSDemux rejects out of range pointer fields", "[common][mpeg_ts][ts_demux]")
{
    mpeg_ts::TSDemux demux;
    auto start = tsPacket(0x100, true, 0b01, 0);
    start[4] = 0;
    demux.demux(start.data());

    auto bad = tsPacket(0x100, true, 0b01, 0);
    bad[4] = 184; // Points past the end of the packet
    CHECK(demux.demux(bad.data()).empty());

    // The maximum valid pointer (183) consumes the whole payload
    auto edge = tsPacket(0x100, true, 0b01, 0);
    edge[4] = 183;
    auto out = demux.demux(edge.data());
    REQUIRE(out.size() == 1);
    CHECK(out[0].size() == 183 + 183);
}
