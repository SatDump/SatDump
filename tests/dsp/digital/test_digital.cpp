#include "dsp/digital/binary_slicer.h"
#include "dsp/digital/bit_to_float.h"
#include "dsp/digital/bits_repack.h"
#include "dsp/digital/cadu_deframer.h"
#include "dsp/digital/cadu_derand.h"
#include "dsp/digital/differential_decoder.h"
#include "dsp/digital/unpack_bits.h"
#include "../filter/ndsp_sig_test_helpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump::ndsp;

namespace
{
    std::vector<int8_t> randomBits(size_t n, uint32_t seed)
    {
        ndsp_sig_test::Lcg rng(seed);
        std::vector<int8_t> v(n);
        for (auto &b : v)
            b = rng.next() >> 31;
        return v;
    }

    std::vector<int8_t> randomBytes(size_t n, uint32_t seed)
    {
        ndsp_sig_test::Lcg rng(seed);
        std::vector<int8_t> v(n);
        for (auto &b : v)
            b = (int8_t)(rng.next() >> 24);
        return v;
    }

    // Blocks without any parameter
    void checkNoParams(Block &b)
    {
        CHECK(ndsp_sig_test::getAll(b).empty());
        CHECK_THROWS(b.get_cfg("anything"));
        CHECK_THROWS(b.set_cfg("anything", 1));
        CHECK(ndsp_sig_test::setAll(b, {{"anything", 1}}) == Block::RES_ERR);
    }

    const uint32_t kAsm = 0x1ACFFC1D;
} // namespace

TEST_CASE("ndsp BinarySlicerBlock slices on the sign", "[dsp][digital]")
{
    std::vector<float> in = {1.0f, -1.0f, 0.5f, -0.001f, 3e-6f, -1e9f, 1e9f, 0.0f};
    auto blk = std::make_shared<BinarySlicerBlock>();
    auto r = ndsp_sig_test::runBlock<float, int8_t>(blk, in);
    REQUIRE(r.completed);
    // 0.0 counts as positive
    std::vector<int8_t> expected = {1, 0, 1, 0, 1, 0, 1, 1};
    CHECK(r.output() == expected);

    CHECK(blk->d_id == "binary_slicer_fh");
    CHECK(blk->get_inputs()[0].type == DSP_SAMPLE_TYPE_F32);
    CHECK(blk->get_outputs()[0].type == DSP_SAMPLE_TYPE_S8);
    checkNoParams(*blk);
}

TEST_CASE("ndsp BitToFloatBlock maps bits to +-1", "[dsp][digital]")
{
    std::vector<int8_t> in = {0, 1, 1, 0, 5, -3};
    auto blk = std::make_shared<BitToFloatBlock>();
    auto r = ndsp_sig_test::runBlock<int8_t, float>(blk, in);
    REQUIRE(r.completed);
    std::vector<float> expected = {-1, 1, 1, -1, 1, 1};
    CHECK(r.output() == expected);
    CHECK(blk->d_id == "bit_to_float_hf");
    checkNoParams(*blk);
}

TEST_CASE("ndsp BitToFloatBlock then BinarySlicerBlock round-trips bits", "[dsp][digital]")
{
    auto bits = randomBits(50000, 1);
    auto r = ndsp_sig_test::runChain<int8_t, int8_t>({std::make_shared<BitToFloatBlock>(), std::make_shared<BinarySlicerBlock>()}, bits,
                                                     ndsp_sig_test::chunked(3001));
    REQUIRE(r.completed);
    CHECK(r.output() == bits);
}

TEST_CASE("ndsp UnpackBitsBlock outputs bits MSB first", "[dsp][digital]")
{
    std::vector<int8_t> in = {(int8_t)0xA5, 0x00, (int8_t)0xFF, 0x01, (int8_t)0x80};
    auto blk = std::make_shared<UnpackBitsBlock>();
    auto r = ndsp_sig_test::runBlock<int8_t, int8_t>(blk, in);
    REQUIRE(r.completed);
    std::vector<int8_t> expected = {1, 0, 1, 0, 0, 1, 0, 1, //
                                    0, 0, 0, 0, 0, 0, 0, 0, //
                                    1, 1, 1, 1, 1, 1, 1, 1, //
                                    0, 0, 0, 0, 0, 0, 0, 1, //
                                    1, 0, 0, 0, 0, 0, 0, 0};
    CHECK(r.output() == expected);
    CHECK(blk->d_id == "unpack_bits_hh");
    checkNoParams(*blk);
}

TEST_CASE("ndsp UnpackBitsBlock produces 8 bits per byte on large buffers", "[dsp][digital]")
{
    auto bytes = randomBytes(40000, 2);
    auto r = ndsp_sig_test::runBlock<int8_t, int8_t>(std::make_shared<UnpackBitsBlock>(), bytes, ndsp_sig_test::chunked(8192));
    REQUIRE(r.completed);
    REQUIRE(r.output().size() == bytes.size() * 8);
    for (size_t i = 0; i < bytes.size(); i++)
        for (int b = 0; b < 8; b++)
            REQUIRE(r.output()[i * 8 + b] == (((uint8_t)bytes[i] >> (7 - b)) & 1));
}

TEST_CASE("ndsp BitsRepackBlock packs bits MSB first and carries partial bytes across buffers", "[dsp][digital]")
{
    std::vector<int8_t> bits = {1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1};
    auto blk = std::make_shared<BitsRepackBlock>();
    // 3 bits per buffer, so every byte straddles buffers
    auto r = ndsp_sig_test::runBlock<int8_t, int8_t>(blk, bits, ndsp_sig_test::chunked(3));
    REQUIRE(r.completed);
    // Incomplete trailing byte is held back
    std::vector<int8_t> expected = {(int8_t)0xA5, 0x01};
    CHECK(r.output() == expected);
    CHECK(blk->d_id == "bits_repack_hh");
    checkNoParams(*blk);
}

TEST_CASE("ndsp UnpackBitsBlock then BitsRepackBlock round-trips bytes", "[dsp][digital]")
{
    auto bytes = randomBytes(20000, 3);
    auto r = ndsp_sig_test::runChain<int8_t, int8_t>({std::make_shared<UnpackBitsBlock>(), std::make_shared<BitsRepackBlock>()}, bytes, ndsp_sig_test::chunked(777));
    REQUIRE(r.completed);
    CHECK(r.output() == bytes);
}

TEST_CASE("ndsp DifferentialDecoderBlock decodes NRZ-M", "[dsp][digital]")
{
    // NRZ-M : a 1 is a level change, a 0 no change. Initial level is 0
    auto data = randomBits(30000, 4);
    std::vector<int8_t> encoded(data.size());
    int8_t level = 0;
    for (size_t i = 0; i < data.size(); i++)
        encoded[i] = level = level ^ data[i];

    auto blk = std::make_shared<DifferentialDecoderBlock>();
    auto r = ndsp_sig_test::runBlock<int8_t, int8_t>(blk, encoded, ndsp_sig_test::chunked(1001));
    REQUIRE(r.completed);
    // The state is kept between buffers
    CHECK(r.output() == data);
    CHECK(blk->d_id == "differential_decoder_hh");
    checkNoParams(*blk);
}

TEST_CASE("ndsp DifferentialDecoderBlock hand-built sequence", "[dsp][digital]")
{
    std::vector<int8_t> in = {0, 1, 1, 0, 0, 0, 1, 0};
    std::vector<int8_t> expected = {0, 1, 0, 1, 0, 0, 1, 1};
    auto r = ndsp_sig_test::runBlock<int8_t, int8_t>(std::make_shared<DifferentialDecoderBlock>(), in);
    REQUIRE(r.completed);
    CHECK(r.output() == expected);
}

TEST_CASE("ndsp CADUDerandBlock derandomizes CADUs and leaves the ASM untouched", "[dsp][digital]")
{
    const int cadu_size = 1024;
    const int nframes = 6;
    auto payload = randomBytes(cadu_size * nframes, 5);
    std::vector<int8_t> in(payload);
    for (int f = 0; f < nframes; f++)
        for (int i = 0; i < 4; i++)
            in[f * cadu_size + i] = payload[f * cadu_size + i] = (int8_t)(kAsm >> (24 - 8 * i));

    // Randomize, with the CCSDS PN sequence (h(x) = x^8 + x^7 + x^5 + x^3 + 1, all ones
    // initial state, period 255) applied from the first byte after the ASM
    std::vector<uint8_t> pn(255);
    uint8_t sr = 0xFF;
    for (int i = 0; i < 255; i++)
    {
        uint8_t byte = 0;
        for (int b = 0; b < 8; b++)
        {
            uint8_t out = sr & 1;
            byte = byte << 1 | out;
            uint8_t fb = ((sr >> 0) ^ (sr >> 3) ^ (sr >> 5) ^ (sr >> 7)) & 1;
            sr = (sr >> 1) | (fb << 7);
        }
        pn[i] = byte;
    }
    // Known start of the CCSDS sequence
    REQUIRE(pn[0] == 0xFF);
    REQUIRE(pn[1] == 0x48);
    REQUIRE(pn[2] == 0x0E);
    REQUIRE(pn[3] == 0xC0);

    for (int f = 0; f < nframes; f++)
        for (int i = 4; i < cadu_size; i++)
            in[f * cadu_size + i] ^= pn[(i - 4) % 255];

    auto blk = std::make_shared<CADUDerandBlock>();
    // Buffers must be whole CADUs
    auto r = ndsp_sig_test::runBlock<int8_t, int8_t>(blk, in, ndsp_sig_test::chunked(2 * cadu_size));
    REQUIRE(r.completed);
    CHECK(r.output() == payload);
}

TEST_CASE("ndsp CADUDerandBlock drops buffers that are not whole CADUs", "[dsp][digital]")
{
    CADUDerandBlock blk;
    REQUIRE(blk.set_cfg("cadu_size", 16) == Block::RES_OK);
    std::vector<int8_t> in(24, 0);
    CHECK(ndsp_sig_test::processDirect<int8_t>(blk, in, 24).empty());
    CHECK(ndsp_sig_test::processDirect<int8_t>(blk, in, 16).size() == 16); // 16 then 8 (dropped)

    // offset >= cadu_size : nothing to derandomize, dropped too
    REQUIRE(blk.set_cfg("offset", 16) == Block::RES_OK);
    CHECK(ndsp_sig_test::processDirect<int8_t>(blk, std::vector<int8_t>(16, 0), 16).empty());
}

TEST_CASE("ndsp CADUDerandBlock configuration round-trip", "[dsp][digital]")
{
    CADUDerandBlock blk;
    CHECK(blk.d_id == "cadu_derand_hh");
    auto def = ndsp_sig_test::getAll(blk);
    CHECK(def["cadu_size"] == 1024);
    CHECK(def["offset"] == 4);
    CHECK(ndsp_sig_test::setAll(blk, {{"cadu_size", 892}, {"offset", 0}}) == Block::RES_OK);
    CHECK(ndsp_sig_test::getAll(blk) == nlohmann::json({{"cadu_size", 892}, {"offset", 0}}));
    CHECK_THROWS(blk.set_cfg("x", 1));
    CHECK_THROWS(blk.get_cfg("x"));
}

namespace
{
    std::vector<int8_t> framesAsBits(int nframes, int frame_bytes, uint32_t seed, std::vector<int8_t> &frames_bytes, bool invert = false)
    {
        frames_bytes = randomBytes(nframes * frame_bytes, seed);
        for (int f = 0; f < nframes; f++)
            for (int i = 0; i < 4; i++)
                frames_bytes[f * frame_bytes + i] = (int8_t)(kAsm >> (24 - 8 * i));
        std::vector<int8_t> bits;
        for (int8_t b : frames_bytes)
            for (int i = 7; i >= 0; i--)
                bits.push_back((((uint8_t)b >> i) & 1) ^ invert);
        return bits;
    }
} // namespace

TEST_CASE("ndsp CADUDeframerBlock extracts CADUs from a bit stream", "[dsp][digital]")
{
    std::vector<int8_t> frames;
    auto bits = framesAsBits(16, 1024, 6, frames);
    // Some junk in front, not containing the ASM (all zeros)
    bits.insert(bits.begin(), 333, 0);

    auto blk = std::make_shared<CADUDeframerBlock>();
    auto r = ndsp_sig_test::runBlock<int8_t, int8_t>(blk, bits, ndsp_sig_test::chunked(5000));
    REQUIRE(r.completed);
    CHECK(r.output() == frames);
    // Outputs whole frames only
    for (auto s : r.buffer_sizes[0])
        CHECK(s % 1024 == 0);
    // Now synced
    CHECK(blk->get_cfg("state") == 12);
}

TEST_CASE("ndsp CADUDeframerBlock corrects bit inversion and supports other CADU sizes", "[dsp][digital]")
{
    std::vector<int8_t> frames;
    auto bits = framesAsBits(20, 256, 7, frames, true);

    auto blk = std::make_shared<CADUDeframerBlock>();
    REQUIRE(blk->set_cfg("cadu_size", 256 * 8) == Block::RES_OK);
    auto r = ndsp_sig_test::runBlock<int8_t, int8_t>(blk, bits, ndsp_sig_test::chunked(4096));
    REQUIRE(r.completed);
    CHECK(r.output() == frames);
}

TEST_CASE("ndsp CADUDeframerBlock configuration round-trip", "[dsp][digital]")
{
    CADUDeframerBlock blk;
    CHECK(blk.d_id == "cadu_deframer_hh");
    auto def = ndsp_sig_test::getAll(blk);
    CHECK(def["cadu_size"] == 8192);
    CHECK(def["state"] == 0);
    CHECK(blk.set_cfg("cadu_size", 2048) == Block::RES_OK);
    // The state is read-only, writing it is ignored
    CHECK(blk.set_cfg("state", 6) == Block::RES_OK);
    CHECK(ndsp_sig_test::getAll(blk) == nlohmann::json({{"cadu_size", 2048}, {"state", 0}}));
    CHECK_THROWS(blk.set_cfg("x", 1));
}
