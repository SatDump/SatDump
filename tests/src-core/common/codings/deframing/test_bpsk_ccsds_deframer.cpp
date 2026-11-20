#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include "helpers/random.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace
{
    const uint32_t CCSDS_ASM = 0x1ACFFC1D;

    uint32_t nextRand(uint32_t &state) { return test_helpers::lcgNext(state) >> 8; }

    // Frames are ASM + random payload, `frameBits` long in total
    std::vector<std::vector<uint8_t>> makeFrames(int count, int frameBits, uint32_t seed, uint32_t asmWord = CCSDS_ASM)
    {
        std::vector<std::vector<uint8_t>> frames(count, std::vector<uint8_t>(frameBits / 8));
        for (auto &f : frames)
        {
            f[0] = asmWord >> 24;
            f[1] = asmWord >> 16;
            f[2] = asmWord >> 8;
            f[3] = asmWord;
            for (size_t i = 4; i < f.size(); i++)
                f[i] = nextRand(seed);
        }
        return frames;
    }

    void appendBytesAsBits(std::vector<uint8_t> &bits, const std::vector<uint8_t> &bytes, bool invert = false)
    {
        for (uint8_t b : bytes)
            for (int i = 7; i >= 0; i--)
                bits.push_back(((b >> i) & 1) ^ invert);
    }

    // Random bits that never contain the ASM or its complement in any 32-bit window
    std::vector<uint8_t> makeJunkBits(int count, uint32_t seed, uint32_t asmWord = CCSDS_ASM)
    {
        std::vector<uint8_t> bits;
        uint32_t shifter = 0;
        int n = 0;
        while ((int)bits.size() < count)
        {
            uint8_t bit = nextRand(seed) & 1;
            uint32_t next = shifter << 1 | bit;
            if (n >= 31 && (next == asmWord || next == ~asmWord))
                bit ^= 1, next ^= 1;
            shifter = next;
            bits.push_back(bit);
            n++;
        }
        return bits;
    }

    std::vector<uint8_t> streamBits(const std::vector<std::vector<uint8_t>> &frames, int junkBits = 0, bool invert = false)
    {
        std::vector<uint8_t> bits = makeJunkBits(junkBits, 999);
        for (const auto &f : frames)
            appendBytesAsBits(bits, f, invert);
        return bits;
    }

    // Flips `errors` distinct bits of the ASM part of a frame
    void corruptAsm(std::vector<uint8_t> &frame, int errors)
    {
        for (int i = 0; i < errors; i++)
        {
            int bit = (i * 7) % 32; // 7 is coprime with 32, so all positions are distinct
            frame[bit / 8] ^= 0x80 >> (bit % 8);
        }
    }

    std::vector<std::vector<uint8_t>> runDeframer(deframing::BPSK_CCSDS_Deframer &def, std::vector<uint8_t> &bits, int frameBytes)
    {
        std::vector<uint8_t> out(bits.size() / 4 + 4 * frameBytes);
        int n = def.work(bits.data(), bits.size(), out.data());
        std::vector<std::vector<uint8_t>> frames;
        for (int i = 0; i < n; i++)
            frames.emplace_back(out.begin() + i * frameBytes, out.begin() + (i + 1) * frameBytes);
        return frames;
    }
} // namespace

TEST_CASE("BPSK_CCSDS_Deframer default configuration", "[common][codings][deframing]")
{
    deframing::BPSK_CCSDS_Deframer def;
    CHECK(def.CADU_ASM == CCSDS_ASM);
    CHECK(def.CADU_ASM_INV == ~CCSDS_ASM);
    CHECK(def.CADU_SIZE == 8192);
    CHECK(def.CADU_ASM_SIZE == 32);
    CHECK(def.getState() == def.STATE_NOSYNC);
}

TEST_CASE("BPSK_CCSDS_Deframer locks onto a clean stream and outputs every frame", "[common][codings][deframing]")
{
    const int frameBits = 1024;
    auto frames = makeFrames(20, frameBits, 1);
    auto bits = streamBits(frames, 333);

    deframing::BPSK_CCSDS_Deframer def(frameBits);
    auto out = runDeframer(def, bits, frameBits / 8);
    CHECK(out == frames);
    CHECK(def.getState() == def.STATE_SYNCED);
}

TEST_CASE("BPSK_CCSDS_Deframer works with the default 8192-bit CADU", "[common][codings][deframing]")
{
    auto frames = makeFrames(14, 8192, 2);
    auto bits = streamBits(frames, 77);

    deframing::BPSK_CCSDS_Deframer def;
    auto out = runDeframer(def, bits, 1024);
    CHECK(out == frames);
}

TEST_CASE("BPSK_CCSDS_Deframer state goes NOSYNC -> SYNCING -> SYNCED", "[common][codings][deframing]")
{
    const int frameBits = 256;
    auto frames = makeFrames(13, frameBits, 3);
    std::vector<uint8_t> out(64 * 32);
    deframing::BPSK_CCSDS_Deframer def(frameBits);

    std::vector<uint8_t> junk = makeJunkBits(100, 4);
    CHECK(def.work(junk.data(), junk.size(), out.data()) == 0);
    CHECK(def.getState() == def.STATE_NOSYNC);

    // One complete frame plus the next ASM gives the first lock
    for (int i = 0; i < 13; i++)
    {
        std::vector<uint8_t> bits;
        appendBytesAsBits(bits, frames[i]);
        int n = def.work(bits.data(), bits.size(), out.data());
        INFO("frame " << i);
        CHECK(n == 1);
        // 11 consecutive good ASMs after the first lock are needed to be SYNCED
        CHECK(def.getState() == (i < 11 ? def.STATE_SYNCING : def.STATE_SYNCED));
    }
}

TEST_CASE("BPSK_CCSDS_Deframer corrects a bit-inverted stream", "[common][codings][deframing]")
{
    const int frameBits = 512;
    auto frames = makeFrames(15, frameBits, 5);
    auto bits = streamBits(frames, 50, true);

    deframing::BPSK_CCSDS_Deframer def(frameBits);
    auto out = runDeframer(def, bits, frameBits / 8);

    // Output frames are re-inverted, including the (non-inverted) ASM
    CHECK(out == frames);
}

TEST_CASE("BPSK_CCSDS_Deframer chunked input gives the same frames", "[common][codings][deframing]")
{
    const int frameBits = 512;
    auto frames = makeFrames(25, frameBits, 6);
    auto bits = streamBits(frames, 123);

    deframing::BPSK_CCSDS_Deframer def(frameBits);
    std::vector<std::vector<uint8_t>> got;
    std::vector<uint8_t> out(8 * 64);
    size_t pos = 0;
    uint32_t rng = 7;
    while (pos < bits.size())
    {
        size_t chunk = std::min<size_t>(1 + nextRand(rng) % 700, bits.size() - pos);
        int n = def.work(&bits[pos], chunk, out.data());
        for (int i = 0; i < n; i++)
            got.emplace_back(out.begin() + i * 64, out.begin() + (i + 1) * 64);
        pos += chunk;
    }
    CHECK(got == frames);
}

TEST_CASE("BPSK_CCSDS_Deframer tolerates ASM bit errors depending on state", "[common][codings][deframing]")
{
    const int frameBits = 256;

    SECTION("SYNCED accepts up to 11 errors and restores the clean ASM")
    {
        auto frames = makeFrames(20, frameBits, 8);
        auto sent = frames;
        for (int i = 13; i < 20; i++)
            corruptAsm(sent[i], 11);
        auto bits = streamBits(sent, 64);

        deframing::BPSK_CCSDS_Deframer def(frameBits);
        auto out = runDeframer(def, bits, frameBits / 8);
        CHECK(out == frames);
        CHECK(def.getState() == def.STATE_SYNCED);
    }

    SECTION("SYNCED drops lock on 12 errors, then relocks on the next clean ASM")
    {
        auto frames = makeFrames(20, frameBits, 9);
        auto sent = frames;
        corruptAsm(sent[15], 12);
        auto bits = streamBits(sent, 64);

        deframing::BPSK_CCSDS_Deframer def(frameBits);
        auto out = runDeframer(def, bits, frameBits / 8);

        // Frames 0-14 come out, frame 15 (bad ASM) is lost, and the exact search relocks on 16
        auto expected = std::vector<std::vector<uint8_t>>(frames.begin(), frames.begin() + 15);
        expected.insert(expected.end(), frames.begin() + 16, frames.end());
        CHECK(out == expected);
        CHECK(def.getState() == def.STATE_SYNCING);
    }

    SECTION("SYNCING accepts 5 errors")
    {
        auto frames = makeFrames(6, frameBits, 10);
        auto sent = frames;
        corruptAsm(sent[2], 5);
        auto bits = streamBits(sent, 64);

        deframing::BPSK_CCSDS_Deframer def(frameBits);
        auto out = runDeframer(def, bits, frameBits / 8);
        CHECK(out == frames);
    }

    SECTION("SYNCING rejects 6 errors and falls back to an exact search")
    {
        auto frames = makeFrames(6, frameBits, 11);
        auto sent = frames;
        corruptAsm(sent[2], 6);
        auto bits = streamBits(sent, 64);

        deframing::BPSK_CCSDS_Deframer def(frameBits);
        auto out = runDeframer(def, bits, frameBits / 8);
        std::vector<std::vector<uint8_t>> expected = {frames[0], frames[1], frames[3], frames[4], frames[5]};
        CHECK(out == expected);
    }

    SECTION("NOSYNC requires an exact ASM")
    {
        auto frames = makeFrames(3, frameBits, 12);
        auto sent = frames;
        corruptAsm(sent[0], 1);
        auto bits = streamBits(sent, 64);

        deframing::BPSK_CCSDS_Deframer def(frameBits);
        auto out = runDeframer(def, bits, frameBits / 8);
        std::vector<std::vector<uint8_t>> expected = {frames[1], frames[2]};
        CHECK(out == expected);
    }
}

TEST_CASE("BPSK_CCSDS_Deframer outputs nothing for random data", "[common][codings][deframing]")
{
    std::vector<uint8_t> bits = makeJunkBits(20000, 13);
    std::vector<uint8_t> out(4096);
    deframing::BPSK_CCSDS_Deframer def(1024);
    CHECK(def.work(bits.data(), bits.size(), out.data()) == 0);
    CHECK(def.getState() == def.STATE_NOSYNC);
}

TEST_CASE("BPSK_CCSDS_Deframer pads output frames with zero bits", "[common][codings][deframing]")
{
    const int frameBits = 256;
    auto frames = makeFrames(5, frameBits, 14);
    auto bits = streamBits(frames, 20);

    deframing::BPSK_CCSDS_Deframer def(frameBits);
    def.CADU_PADDING = 16;
    auto out = runDeframer(def, bits, frameBits / 8 + 2);

    REQUIRE(out.size() == frames.size());
    for (size_t i = 0; i < out.size(); i++)
    {
        CHECK(std::vector<uint8_t>(out[i].begin(), out[i].begin() + 32) == frames[i]);
        CHECK(out[i][32] == 0);
        CHECK(out[i][33] == 0);
    }
}

TEST_CASE("BPSK_CCSDS_Deframer works with a custom sync word", "[common][codings][deframing]")
{
    const uint32_t sync = 0x352EF853;
    const int frameBits = 320;
    auto frames = makeFrames(8, frameBits, 15, sync);
    std::vector<uint8_t> bits = makeJunkBits(40, 16, sync);
    for (const auto &f : frames)
        appendBytesAsBits(bits, f);

    deframing::BPSK_CCSDS_Deframer def(frameBits, sync);
    auto out = runDeframer(def, bits, frameBits / 8);
    CHECK(out == frames);
}
