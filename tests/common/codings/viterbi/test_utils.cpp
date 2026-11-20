#include "common/codings/viterbi/utils.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("signed_soft_to_unsigned offsets by 127 and never produces the erasure value", "[common][codings][viterbi]")
{
    std::vector<int8_t> in;
    for (int v = -128; v <= 127; v++)
        in.push_back(v);
    std::vector<uint8_t> out(in.size());
    signed_soft_to_unsigned(in.data(), out.data(), in.size());

    for (size_t i = 0; i < in.size(); i++)
    {
        INFO("input " << int(in[i]));
        if (in[i] == 1)
            CHECK(out[i] == 127); // would be 128, the erasure marker
        else
            CHECK(out[i] == uint8_t(in[i] + 127));
        CHECK(out[i] != 128);
    }
    // Sign is preserved around the decoder's decision threshold (> 127 means 1)
    CHECK(out[255] > 127); // +127
    CHECK(out[129] < 128); // +1 maps to the weakest 0 side value
    CHECK(out[127] < 128); // -1
}

TEST_CASE("signed_soft_to_unsigned with zero length writes nothing", "[common][codings][viterbi]")
{
    int8_t in[1] = {5};
    uint8_t out[1] = {0xAA};
    signed_soft_to_unsigned(in, out, 0);
    CHECK(out[0] == 0xAA);
}

TEST_CASE("ensureIs2Multiple rounds odd values up", "[common][codings][viterbi]")
{
    CHECK(ensureIs2Multiple(0) == 0);
    CHECK(ensureIs2Multiple(1) == 2);
    CHECK(ensureIs2Multiple(2) == 2);
    CHECK(ensureIs2Multiple(1023) == 1024);
    CHECK(ensureIs2Multiple(8192) == 8192);
}
