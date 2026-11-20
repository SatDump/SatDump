#include "common/codings/dvb-s2/bbframe_ts_parser.h"
#include "helpers/random.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <vector>

namespace
{
    // Short FECFRAME, rate 1/2: Kbch = 7032 bits
    const int KBCH = 7032;
    const int FRAME_BYTES = KBCH / 8;

    // DVB-S2 CRC-8, g(x) = x^8 + x^7 + x^6 + x^4 + x^2 + 1, MSB first, zero init (EN 302 307 5.1.4)
    uint8_t crc8(const uint8_t *data, int len)
    {
        uint8_t crc = 0;
        for (int i = 0; i < len; i++)
            for (int b = 7; b >= 0; b--)
            {
                bool fb = ((crc >> 7) & 1) ^ ((data[i] >> b) & 1);
                crc <<= 1;
                if (fb)
                    crc ^= 0xD5;
            }
        return crc;
    }

    std::vector<std::vector<uint8_t>> makePackets(int count, uint32_t seed)
    {
        std::vector<std::vector<uint8_t>> pkts(count, std::vector<uint8_t>(TS_SIZE));
        test_helpers::Lcg rng(seed);
        for (int p = 0; p < count; p++)
        {
            pkts[p][0] = 0x47;
            for (int i = 1; i < TS_SIZE; i++)
                pkts[p][i] = rng.byte();
            pkts[p][1] &= 0x1F; // TEI and flags clear, PID high bits
        }
        return pkts;
    }

    // Mode adaptation: the sync byte of each user packet carries the CRC-8 of the previous packet
    std::vector<uint8_t> userPacketStream(const std::vector<std::vector<uint8_t>> &pkts)
    {
        std::vector<uint8_t> s;
        uint8_t prevCrc = 0;
        for (auto &p : pkts)
        {
            s.push_back(prevCrc);
            s.insert(s.end(), p.begin() + 1, p.end());
            prevCrc = crc8(&p[1], TS_SIZE - 1);
        }
        s.push_back(prevCrc); // CRC of the last packet
        return s;
    }

    void writeHeader(uint8_t *bbf, int dflBits, int syncdBits)
    {
        bbf[0] = 0xF0; // TS, single input stream, CCM, no ISSY, no NPD, rolloff 0.35
        bbf[1] = 0x00;
        bbf[2] = (188 * 8) >> 8; // UPL
        bbf[3] = (188 * 8) & 0xFF;
        bbf[4] = dflBits >> 8;
        bbf[5] = dflBits & 0xFF;
        bbf[6] = 0x47;
        bbf[7] = syncdBits >> 8;
        bbf[8] = syncdBits & 0xFF;
        bbf[9] = crc8(bbf, 9);
    }

    // Slice a user packet stream into BBFrames of `dfl` data bytes each
    std::vector<uint8_t> makeFrames(const std::vector<uint8_t> &stream, int dfl, int frames)
    {
        std::vector<uint8_t> out(FRAME_BYTES * frames, 0);
        for (int f = 0; f < frames; f++)
        {
            uint8_t *bbf = &out[f * FRAME_BYTES];
            int start = f * dfl;
            int syncd = (TS_SIZE - start % TS_SIZE) % TS_SIZE; // bytes to the first user packet
            writeHeader(bbf, dfl * 8, syncd * 8);
            for (int i = 0; i < dfl; i++)
                bbf[10 + i] = stream[start + i];
        }
        return out;
    }

    // Feed frames one at a time, as the only in-tree caller does
    std::vector<uint8_t> parseOneByOne(dvbs2::BBFrameTSParser &parser, std::vector<uint8_t> &frames, int count, int *packets = nullptr)
    {
        std::vector<uint8_t> ts;
        std::vector<uint8_t> out(TS_SIZE * 100);
        int total = 0;
        for (int f = 0; f < count; f++)
        {
            int n = parser.work(&frames[f * FRAME_BYTES], 1, out.data(), out.size());
            ts.insert(ts.end(), out.begin(), out.begin() + n * TS_SIZE);
            total += n;
        }
        if (packets)
            *packets = total;
        return ts;
    }

    std::vector<uint8_t> flatten(const std::vector<std::vector<uint8_t>> &pkts, size_t count)
    {
        std::vector<uint8_t> v;
        for (size_t i = 0; i < count && i < pkts.size(); i++)
            v.insert(v.end(), pkts[i].begin(), pkts[i].end());
        return v;
    }
} // namespace

TEST_CASE("BBHeader parses the DVB-S2 baseband header fields", "[common][codings][dvbs2]")
{
    uint8_t bbf[10];
    writeHeader(bbf, 1234 * 8, 100 * 8);
    bbf[0] = 0b11010101; // TS, MIS, ACM, no ISSY, NPD, ro = 01
    bbf[1] = 0x2A;
    dvbs2::BBHeader h(bbf);
    CHECK(h.ts_gs == 3);
    CHECK(h.sis_mis == false);
    CHECK(h.ccm_acm == true);
    CHECK(h.issyi == false);
    CHECK(h.npd == true);
    CHECK(h.ro == true);
    CHECK(h.isi == 0x2A); // only meaningful for multiple input streams
    CHECK(h.upl == 1504);
    CHECK(h.dfl == 1234 * 8);
    CHECK(h.sync == 0x47);
    CHECK(h.syncd == 800);

    bbf[0] |= 0x20; // single input stream: ISI not present
    CHECK(dvbs2::BBHeader(bbf).isi == 0);
}

// Bug: BBHeader::ro is a bool, but the roll-off field is 2 bits (0.35/0.25/0.20/reserved), so the
// values 1, 2 and 3 all collapse to true (bbframe_ts_parser.h:41 and :55).
TEST_CASE("BBHeader keeps the full 2-bit roll-off field", "[common][codings][dvbs2][!shouldfail]")
{
    uint8_t bbf[10];
    writeHeader(bbf, 800, 0);
    bbf[0] = 0xF2; // ro = 10 (0.20)
    dvbs2::BBHeader h(bbf);
    CHECK(int(h.ro) == 2);
}

TEST_CASE("BBFrameTSParser reassembles TS packets spanning BBFrames", "[common][codings][dvbs2]")
{
    // Data field sizes that make packets straddle frame boundaries at different offsets
    int dfl = GENERATE(869, 868, 600, 376, 200);
    INFO("DFL bytes " << dfl);
    const int frames = 12;
    auto pkts = makePackets(dfl * frames / TS_SIZE + 2, dfl);
    std::vector<uint8_t> stream = userPacketStream(pkts);
    std::vector<uint8_t> bb = makeFrames(stream, dfl, frames);

    dvbs2::BBFrameTSParser parser(KBCH);
    int packets = 0;
    std::vector<uint8_t> ts = parseOneByOne(parser, bb, frames, &packets);

    // Every packet whose last byte is inside the parsed frames comes out, without TEI (a packet
    // buffered across frames is only emitted once the next packet starts)
    int complete = dfl * frames / TS_SIZE;
    CHECK(packets >= complete - 1);
    CHECK(packets <= complete);
    CHECK(ts == flatten(pkts, packets));
}

TEST_CASE("BBFrameTSParser resynchronises using SYNCD when starting mid-stream", "[common][codings][dvbs2]")
{
    const int dfl = 700;
    const int frames = 8;
    auto pkts = makePackets(dfl * frames / TS_SIZE + 2, 3);
    std::vector<uint8_t> stream = userPacketStream(pkts);
    std::vector<uint8_t> bb = makeFrames(stream, dfl, frames);

    // Start at frame 1: its first user packet starts SYNCD bytes into the data field
    int firstPacket = (dfl + TS_SIZE - 1) / TS_SIZE;
    dvbs2::BBFrameTSParser parser(KBCH);
    std::vector<uint8_t> tail(bb.begin() + FRAME_BYTES, bb.end());
    int packets = 0;
    std::vector<uint8_t> ts = parseOneByOne(parser, tail, frames - 1, &packets);

    REQUIRE(packets > 10);
    std::vector<uint8_t> expected;
    for (int p = firstPacket; p < firstPacket + packets; p++)
        expected.insert(expected.end(), pkts[p].begin(), pkts[p].end());
    CHECK(ts == expected);
}

TEST_CASE("BBFrameTSParser flags packets with a bad CRC-8 using the TEI bit", "[common][codings][dvbs2]")
{
    const int dfl = 868;
    const int frames = 4;
    auto pkts = makePackets(dfl * frames / TS_SIZE + 2, 4);
    std::vector<uint8_t> stream = userPacketStream(pkts);
    // Corrupt one byte in packet 2 (entirely inside frame 0) and packet 4 (spanning frames 0 and 1)
    stream[2 * TS_SIZE + 50] ^= 0x01;
    stream[4 * TS_SIZE + 10] ^= 0x10;
    std::vector<uint8_t> bb = makeFrames(stream, dfl, frames);

    dvbs2::BBFrameTSParser parser(KBCH);
    int packets = 0;
    std::vector<uint8_t> ts = parseOneByOne(parser, bb, frames, &packets);
    REQUIRE(packets >= 8);

    for (int p = 0; p < packets; p++)
    {
        INFO("packet " << p);
        std::vector<uint8_t> expected = pkts[p];
        if (p == 2)
            expected[50] ^= 0x01;
        if (p == 4)
            expected[10] ^= 0x10;
        if (p == 2 || p == 4)
            expected[1] |= TS_ERROR_INDICATOR;
        CHECK(std::equal(expected.begin(), expected.end(), ts.begin() + p * TS_SIZE));
    }
}

TEST_CASE("BBFrameTSParser drops frames with an invalid header", "[common][codings][dvbs2]")
{
    const int dfl = 868;
    auto pkts = makePackets(10, 5);
    std::vector<uint8_t> stream = userPacketStream(pkts);
    std::vector<uint8_t> out(TS_SIZE * 100);

    SECTION("bad header CRC")
    {
        std::vector<uint8_t> bb = makeFrames(stream, dfl, 1);
        bb[9] ^= 0xFF;
        dvbs2::BBFrameTSParser parser(KBCH);
        CHECK(parser.work(bb.data(), 1, out.data(), out.size()) == 0);
    }

    SECTION("DFL larger than Kbch - 80")
    {
        std::vector<uint8_t> bb = makeFrames(stream, dfl, 1);
        writeHeader(bb.data(), KBCH - 80 + 8, 0);
        dvbs2::BBFrameTSParser parser(KBCH);
        CHECK(parser.work(bb.data(), 1, out.data(), out.size()) == 0);
    }

    SECTION("DFL not a whole number of bytes")
    {
        std::vector<uint8_t> bb = makeFrames(stream, dfl, 1);
        writeHeader(bb.data(), dfl * 8 - 3, 0);
        dvbs2::BBFrameTSParser parser(KBCH);
        CHECK(parser.work(bb.data(), 1, out.data(), out.size()) == 0);
    }

    SECTION("valid frame after a bad one is decoded")
    {
        std::vector<uint8_t> bb = makeFrames(stream, dfl, 2);
        bb[9] ^= 0xFF; // kill frame 0
        dvbs2::BBFrameTSParser parser(KBCH);
        CHECK(parser.work(bb.data(), 1, out.data(), out.size()) == 0);
        int n = parser.work(&bb[FRAME_BYTES], 1, out.data(), out.size());
        REQUIRE(n >= 3);
        // Frame 1 starts mid-packet; the first full packet is packet ceil(dfl / 188)
        int first = (dfl + TS_SIZE - 1) / TS_SIZE;
        CHECK(std::equal(pkts[first].begin(), pkts[first].end(), out.begin()));
    }
}

TEST_CASE("BBFrameTSParser stops writing at buffer_outsize", "[common][codings][dvbs2]")
{
    const int dfl = 868;
    auto pkts = makePackets(10, 6);
    std::vector<uint8_t> stream = userPacketStream(pkts);
    std::vector<uint8_t> bb = makeFrames(stream, dfl, 1);

    std::vector<uint8_t> out(TS_SIZE * 4, 0xEE);
    dvbs2::BBFrameTSParser parser(KBCH);
    CHECK(parser.work(bb.data(), 1, out.data(), 2 * TS_SIZE) == 2);
    CHECK(std::equal(out.begin(), out.begin() + 2 * TS_SIZE, flatten(pkts, 2).begin()));
    CHECK(out[2 * TS_SIZE] == 0xEE);
}

// Bug: work() never resets its read offset (in_p) between BBFrames, so with cnt > 1 every frame after
// the first is read from the wrong position (bbframe_ts_parser.cpp:100, in_p is declared outside the
// per-frame loop). The in-tree caller only ever passes cnt = 1.
TEST_CASE("BBFrameTSParser with several frames per call matches one frame per call", "[common][codings][dvbs2][!shouldfail]")
{
    const int dfl = 868;
    const int frames = 4;
    auto pkts = makePackets(dfl * frames / TS_SIZE + 2, 7);
    std::vector<uint8_t> stream = userPacketStream(pkts);
    std::vector<uint8_t> bb = makeFrames(stream, dfl, frames);

    dvbs2::BBFrameTSParser single(KBCH);
    int expectedPackets = 0;
    std::vector<uint8_t> expected = parseOneByOne(single, bb, frames, &expectedPackets);

    // Extra padding so the misplaced reads stay inside the buffer
    std::vector<uint8_t> padded = bb;
    padded.resize(bb.size() + FRAME_BYTES * frames, 0);
    std::vector<uint8_t> out(TS_SIZE * 100);
    dvbs2::BBFrameTSParser multi(KBCH);
    int n = multi.work(padded.data(), frames, out.data(), out.size());
    CHECK(n == expectedPackets);
    CHECK(std::equal(expected.begin(), expected.end(), out.begin()));
}

// Bug: a packet written directly to the output whose CRC-8 byte only arrives in the next BBFrame
// cannot be flagged any more: on the next call tei_p is 0, so the TEI bit is ORed into tsframes[0]
// (which is then overwritten by the next 0x47) and the corrupted packet was already returned with
// TEI clear (bbframe_ts_parser.cpp:189 and :213).
TEST_CASE("BBFrameTSParser flags a bad packet whose CRC is in the next frame", "[common][codings][dvbs2][!shouldfail]")
{
    // With DFL = 376 bytes, packet 1 ends exactly at the end of frame 0
    const int dfl = 376;
    const int frames = 3;
    auto pkts = makePackets(8, 8);
    std::vector<uint8_t> stream = userPacketStream(pkts);
    stream[1 * TS_SIZE + 100] ^= 0x04;
    std::vector<uint8_t> bb = makeFrames(stream, dfl, frames);

    dvbs2::BBFrameTSParser parser(KBCH);
    int packets = 0;
    std::vector<uint8_t> ts = parseOneByOne(parser, bb, frames, &packets);
    REQUIRE(packets >= 2);
    CHECK((ts[1 * TS_SIZE + 1] & TS_ERROR_INDICATOR) != 0);
    // The next packet is not damaged
    CHECK(std::equal(pkts[2].begin(), pkts[2].end(), ts.begin() + 2 * TS_SIZE));
}
