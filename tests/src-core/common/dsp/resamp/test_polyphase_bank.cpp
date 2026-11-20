#include "common/dsp/resamp/polyphase_bank.h"
#include "common/dsp/window/window.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numeric>

using Catch::Approx;

TEST_CASE("PolyphaseBank splits taps across arms in reverse arm order", "[common][dsp][polyphase_bank]")
{
    std::vector<float> taps(12);
    std::iota(taps.begin(), taps.end(), 1.0f);

    dsp::PolyphaseBank pfb;
    pfb.init(taps, 4);
    REQUIRE(pfb.nfilt == 4);
    REQUIRE(pfb.ntaps == 3);

    // Tap i lands in arm (nfilt - 1 - i % nfilt), position i / nfilt
    for (int i = 0; i < 12; i++)
        CHECK(pfb.taps[3 - (i % 4)][i / 4] == taps[i]);
}

TEST_CASE("PolyphaseBank pads non-multiple tap counts with zeros", "[common][dsp][polyphase_bank]")
{
    std::vector<float> taps(10, 1.0f);
    dsp::PolyphaseBank pfb;
    pfb.init(taps, 4);

    // ceil(10 / 4) = 3, then the "slightly over" check adds one more: arms are zero padded to 4 taps
    REQUIRE(pfb.ntaps == 4);
    float total = 0;
    for (int a = 0; a < 4; a++)
        for (int t = 0; t < pfb.ntaps; t++)
            total += pfb.taps[a][t];
    CHECK(total == 10.0f);
    // Padding is zero
    CHECK(pfb.taps[3 - (10 % 4)][10 / 4] == 0.0f);
    for (int a = 0; a < 4; a++)
        CHECK(pfb.taps[a][3] == 0.0f);
}

TEST_CASE("PolyphaseBank can be re-initialized", "[common][dsp][polyphase_bank]")
{
    dsp::PolyphaseBank pfb;
    pfb.init(std::vector<float>(64, 0.5f), 8);
    CHECK(pfb.nfilt == 8);
    CHECK(pfb.ntaps == 8);

    pfb.init(std::vector<float>(30, 2.0f), 3);
    CHECK(pfb.nfilt == 3);
    CHECK(pfb.ntaps == 10);
    for (int a = 0; a < 3; a++)
        for (int t = 0; t < 10; t++)
            CHECK(pfb.taps[a][t] == 2.0f);
}

TEST_CASE("PolyphaseBank of a windowed sinc has unity-gain arms", "[common][dsp][polyphase_bank]")
{
    // Interpolator setup used by the clock recovery blocks
    const int nfilt = 128, ntaps = 8;
    dsp::PolyphaseBank pfb;
    pfb.init(dsp::windowed_sinc(nfilt * ntaps, M_PI / nfilt, dsp::window::nuttall, nfilt), nfilt);
    REQUIRE(pfb.ntaps == ntaps);

    for (int a = 0; a < nfilt; a++)
    {
        float s = 0;
        for (int t = 0; t < ntaps; t++)
            s += pfb.taps[a][t];
        INFO("arm " << a);
        REQUIRE(s == Approx(1.0f).margin(0.01));
    }
}
