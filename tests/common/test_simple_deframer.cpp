#include "common/simple_deframer.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
    const uint64_t ASM = 0x1ACFFC1D;
    const std::vector<uint8_t> ASM_BYTES = {0x1A, 0xCF, 0xFC, 0x1D};

    std::vector<uint8_t> makePayload(int count, uint8_t seed)
    {
        std::vector<uint8_t> out(count);
        uint32_t state = seed * 2654435761u + 1;
        for (auto &b : out)
        {
            state = state * 1664525u + 1013904223u;
            b = state >> 24;
        }
        return out;
    }

    std::vector<uint8_t> concat(std::initializer_list<std::vector<uint8_t>> parts)
    {
        std::vector<uint8_t> out;
        for (auto &p : parts)
            out.insert(out.end(), p.begin(), p.end());
        return out;
    }

    // Shift a byte stream right by `bits` bits, prefixing with zero bits and padding the tail
    std::vector<uint8_t> shiftBits(const std::vector<uint8_t> &in, int bits)
    {
        std::vector<uint8_t> out(in.size() + 1, 0);
        for (size_t i = 0; i < in.size() * 8; i++)
        {
            int bit = (in[i / 8] >> (7 - i % 8)) & 1;
            size_t o = i + bits;
            out[o / 8] |= bit << (7 - o % 8);
        }
        return out;
    }

    std::vector<int8_t> toSoft(const std::vector<uint8_t> &in)
    {
        std::vector<int8_t> soft;
        for (uint8_t b : in)
            for (int i = 7; i >= 0; i--)
                soft.push_back(((b >> i) & 1) ? 100 : -100);
        return soft;
    }
} // namespace

TEST_CASE("SimpleDeframer extracts byte-aligned frames in bit mode", "[common][simple_deframer]")
{
    // 4 byte ASM + 8 byte payload = 96 bits
    def::SimpleDeframer deframer(ASM, 32, 96, 0);
    auto p1 = makePayload(8, 1);
    auto p2 = makePayload(8, 2);
    auto stream = concat({{0x00, 0x55, 0xAA}, ASM_BYTES, p1, ASM_BYTES, p2, {0x00}});

    auto frames = deframer.work(stream.data(), stream.size());

    REQUIRE(frames.size() == 2);
    CHECK(frames[0] == concat({ASM_BYTES, p1}));
    CHECK(frames[1] == concat({ASM_BYTES, p2}));
}

TEST_CASE("SimpleDeframer finds frames at any bit offset", "[common][simple_deframer]")
{
    auto p1 = makePayload(8, 3);
    auto p2 = makePayload(8, 4);
    auto aligned = concat({{0x00, 0x00}, ASM_BYTES, p1, ASM_BYTES, p2, {0x00}});

    for (int shift = 1; shift < 8; shift++)
    {
        def::SimpleDeframer deframer(ASM, 32, 96, 0);
        auto stream = shiftBits(aligned, shift);
        auto frames = deframer.work(stream.data(), stream.size());

        INFO("shift " << shift);
        REQUIRE(frames.size() == 2);
        CHECK(frames[0] == concat({ASM_BYTES, p1}));
        CHECK(frames[1] == concat({ASM_BYTES, p2}));
    }
}

TEST_CASE("SimpleDeframer state carries across work calls", "[common][simple_deframer]")
{
    auto p1 = makePayload(8, 5);
    auto p2 = makePayload(8, 6);
    auto stream = concat({{0x00}, ASM_BYTES, p1, ASM_BYTES, p2});

    for (size_t split = 0; split <= stream.size(); split++)
    {
        def::SimpleDeframer deframer(ASM, 32, 96, 0);
        auto frames = deframer.work(stream.data(), split);
        auto more = deframer.work(stream.data() + split, stream.size() - split);
        frames.insert(frames.end(), more.begin(), more.end());

        INFO("split " << split);
        REQUIRE(frames.size() == 2);
        CHECK(frames[0] == concat({ASM_BYTES, p1}));
        CHECK(frames[1] == concat({ASM_BYTES, p2}));
    }
}

TEST_CASE("SimpleDeframer threshold tolerates bit errors in the sync word", "[common][simple_deframer]")
{
    auto p1 = makePayload(8, 7);
    // Two bit errors in the ASM
    std::vector<uint8_t> corrupted = {0x1B, 0xCF, 0xFC, 0x1C};
    auto stream = concat({{0x00}, corrupted, p1, {0x00}});

    SECTION("threshold 3 accepts two errors and outputs the clean ASM")
    {
        def::SimpleDeframer deframer(ASM, 32, 96, 3);
        auto frames = deframer.work(stream.data(), stream.size());
        REQUIRE(frames.size() == 1);
        CHECK(frames[0] == concat({ASM_BYTES, p1}));
    }

    SECTION("threshold 2 rejects two errors")
    {
        def::SimpleDeframer deframer(ASM, 32, 96, 2);
        CHECK(deframer.work(stream.data(), stream.size()).empty());
    }

    SECTION("threshold 0 requires an exact match")
    {
        def::SimpleDeframer deframer(ASM, 32, 96, 0);
        CHECK(deframer.work(stream.data(), stream.size()).empty());
    }
}

TEST_CASE("SimpleDeframer pads a frame cut short by a new sync word", "[common][simple_deframer]")
{
    // 16 byte frames, but the second ASM arrives after only 4 payload bytes
    def::SimpleDeframer deframer(ASM, 32, 128, 0);
    auto shortp = makePayload(4, 8);
    auto full = makePayload(12, 9);
    auto stream = concat({{0x00}, ASM_BYTES, shortp, ASM_BYTES, full});

    auto frames = deframer.work(stream.data(), stream.size());

    REQUIRE(frames.size() == 2);
    // The interrupting ASM's own bits are part of the truncated frame before padding
    CHECK(frames[0] == concat({ASM_BYTES, shortp, ASM_BYTES, {0, 0, 0, 0}}));
    CHECK(frames[1] == concat({ASM_BYTES, full}));
}

TEST_CASE("SimpleDeframer byte-aligned mode", "[common][simple_deframer]")
{
    auto p1 = makePayload(8, 10);
    auto p2 = makePayload(8, 11);

    SECTION("finds aligned frames")
    {
        def::SimpleDeframer deframer(ASM, 32, 96, 0, true);
        auto stream = concat({{0x12, 0x34}, ASM_BYTES, p1, ASM_BYTES, p2});
        auto frames = deframer.work(stream.data(), stream.size());
        REQUIRE(frames.size() == 2);
        CHECK(frames[0] == concat({ASM_BYTES, p1}));
        CHECK(frames[1] == concat({ASM_BYTES, p2}));
    }

    SECTION("ignores sync words that are not byte aligned")
    {
        def::SimpleDeframer deframer(ASM, 32, 96, 0, true);
        auto stream = shiftBits(concat({{0x00}, ASM_BYTES, p1, {0x00}}), 4);
        CHECK(deframer.work(stream.data(), stream.size()).empty());
    }

    SECTION("pads a frame cut short by a new sync word")
    {
        def::SimpleDeframer deframer(ASM, 32, 128, 0, true);
        auto shortp = makePayload(2, 12);
        auto full = makePayload(12, 13);
        auto stream = concat({ASM_BYTES, shortp, ASM_BYTES, full});
        auto frames = deframer.work(stream.data(), stream.size());
        REQUIRE(frames.size() == 2);
        CHECK(frames[0] == concat({ASM_BYTES, shortp, ASM_BYTES, std::vector<uint8_t>(6, 0)}));
        CHECK(frames[1] == concat({ASM_BYTES, full}));
    }
}

TEST_CASE("SimpleDeframer soft bit input", "[common][simple_deframer]")
{
    def::SimpleDeframer deframer(ASM, 32, 96, 0, false, true);
    auto p1 = makePayload(8, 14);
    auto bytes = concat({{0xF0}, ASM_BYTES, p1, {0x00}});
    auto soft = toSoft(bytes);

    // Size is given in bytes (groups of 8 soft symbols)
    auto frames = deframer.work((uint8_t *)soft.data(), bytes.size());

    REQUIRE(frames.size() == 1);
    CHECK(frames[0] == concat({ASM_BYTES, p1}));
}

TEST_CASE("SimpleDeframer with a 16 bit sync word and no sync in data", "[common][simple_deframer]")
{
    def::SimpleDeframer deframer(0xEB90, 16, 48, 0);
    std::vector<uint8_t> nosync(64, 0x00);
    CHECK(deframer.work(nosync.data(), nosync.size()).empty());
    CHECK(deframer.work(nosync.data(), 0).empty());

    std::vector<uint8_t> stream = {0x00, 0xEB, 0x90, 0x01, 0x02, 0x03, 0x04, 0x00};
    auto frames = deframer.work(stream.data(), stream.size());
    REQUIRE(frames.size() == 1);
    CHECK(frames[0] == std::vector<uint8_t>{0xEB, 0x90, 0x01, 0x02, 0x03, 0x04});
}
