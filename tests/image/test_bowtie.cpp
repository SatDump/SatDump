#include "image/bowtie.h"
#include "image_test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using satdump::image::Image;
namespace bowtie = satdump::image::bowtie;

namespace
{
    Image makeScanImage(int depth, int w, int h, int channels)
    {
        Image im(depth, w, h, channels);
        for (int c = 0; c < channels; c++)
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    im.set(c, x, y, 100 * c + 10 * y + x);
        return im;
    }
} // namespace

TEST_CASE("image::bowtie::correctGenericBowTie with alpha 0, beta 1 is the identity", "[image][bowtie]")
{
    auto depth = GENERATE(8, 16);
    Image in = makeScanImage(depth, 4, 8, 2);
    std::vector<std::vector<int>> lut;
    Image out = bowtie::correctGenericBowTie(in, 2, 4, 0.0f, 1.0f, &lut);
    CHECK(image_test::samePixels(out, in));

    REQUIRE(lut.size() == 4);
    for (int x = 0; x < 4; x++)
    {
        REQUIRE(lut[x].size() == 4);
        for (int i = 0; i < 4; i++)
            CHECK(lut[x][i] == i);
    }
}

TEST_CASE("image::bowtie::correctGenericBowTie with constant beta resamples the scan centre", "[image][bowtie]")
{
    // beta = 0.5 on a 4-line scan keeps the centre 2 lines (padding 1) and
    // stretches them: output lines {0, 1, 2, 3} come from input lines {1, 1, 2, 2}
    Image in = makeScanImage(8, 4, 8, 1);
    std::vector<std::vector<int>> lut;
    Image out = bowtie::correctGenericBowTie(in, 1, 4, 0.0f, 0.5f, &lut);
    const int src_line[4] = {1, 1, 2, 2};
    for (int scan = 0; scan < 2; scan++)
        for (int i = 0; i < 4; i++)
            for (int x = 0; x < 4; x++)
                REQUIRE(out.get(0, x, scan * 4 + i) == in.get(0, x, scan * 4 + src_line[i]));
    for (int x = 0; x < 4; x++)
        for (int i = 0; i < 4; i++)
            CHECK(lut[x][i] == src_line[i]);
}

TEST_CASE("image::bowtie::correctGenericBowTie correction varies across the swath", "[image][bowtie]")
{
    // alpha = beta = 0.5: edge columns keep 2 of 4 lines, the centre column all 4
    Image in = makeScanImage(16, 4, 4, 1);
    std::vector<std::vector<int>> lut;
    Image out = bowtie::correctGenericBowTie(in, 1, 4, 0.5f, 0.5f, &lut);
    CHECK(lut[0] == std::vector<int>{1, 1, 2, 2}); // 2 centre lines, padding 1
    CHECK(lut[1] == std::vector<int>{0, 0, 1, 2}); // 3 lines, padding 0
    CHECK(lut[2] == std::vector<int>{0, 1, 2, 3}); // nadir, unchanged
    CHECK(lut[3] == std::vector<int>{0, 0, 1, 2});
    for (int x = 0; x < 4; x++)
        for (int i = 0; i < 4; i++)
            REQUIRE(out.get(0, x, i) == in.get(0, x, lut[x][i]));
}

TEST_CASE("image::bowtie::correctGenericBowTie output layout", "[image][bowtie]")
{
    SECTION("fewer channels than the input")
    {
        Image in = makeScanImage(8, 4, 4, 3);
        Image out = bowtie::correctGenericBowTie(in, 1, 4, 0.0f, 1.0f);
        CHECK(out.channels() == 1);
        CHECK(out.width() == 4);
        CHECK(out.height() == 4);
        for (int i = 0; i < 16; i++)
            REQUIRE(out.get(0, i) == in.get(0, i));
    }

    SECTION("incomplete trailing scan is left empty")
    {
        Image in = makeScanImage(8, 4, 6, 1);
        Image out = bowtie::correctGenericBowTie(in, 1, 4, 0.0f, 1.0f);
        REQUIRE(out.height() == 6);
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++)
                REQUIRE(out.get(0, x, y) == in.get(0, x, y));
        for (int y = 4; y < 6; y++)
            for (int x = 0; x < 4; x++)
                REQUIRE(out.get(0, x, y) == 0);
    }
}
