#include "image/bayer/bayer.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdlib>
#include <vector>

namespace
{
    // Colour of the Bayer cell at (x, y) for each pattern: 0 = R, 1 = G, 2 = B
    int cellColour(dc1394color_filter_t tile, int x, int y)
    {
        // Top-left 2x2 block, row major
        static const int RGGB[4] = {0, 1, 1, 2};
        static const int GBRG[4] = {1, 2, 0, 1};
        static const int GRBG[4] = {1, 0, 2, 1};
        static const int BGGR[4] = {2, 1, 1, 0};
        const int *p = tile == DC1394_COLOR_FILTER_RGGB ? RGGB : tile == DC1394_COLOR_FILTER_GBRG ? GBRG
                                                             : tile == DC1394_COLOR_FILTER_GRBG   ? GRBG
                                                                                                  : BGGR;
        return p[(y % 2) * 2 + (x % 2)];
    }

    // Mosaic of a uniform colour scene
    template <typename T>
    std::vector<T> makeMosaic(dc1394color_filter_t tile, int sx, int sy, const int rgb[3])
    {
        std::vector<T> m(sx * sy);
        for (int y = 0; y < sy; y++)
            for (int x = 0; x < sx; x++)
                m[y * sx + x] = rgb[cellColour(tile, x, y)];
        return m;
    }

    const char *tileName(dc1394color_filter_t t)
    {
        switch (t)
        {
        case DC1394_COLOR_FILTER_RGGB:
            return "RGGB";
        case DC1394_COLOR_FILTER_GBRG:
            return "GBRG";
        case DC1394_COLOR_FILTER_GRBG:
            return "GRBG";
        default:
            return "BGGR";
        }
    }
} // namespace

TEST_CASE("bayer dc1394_bayer_decoding_8bit reconstructs a uniform colour", "[image][bayer]")
{
    auto tile = GENERATE(DC1394_COLOR_FILTER_RGGB, DC1394_COLOR_FILTER_GBRG, DC1394_COLOR_FILTER_GRBG, DC1394_COLOR_FILTER_BGGR);
    auto method = GENERATE(DC1394_BAYER_METHOD_NEAREST, DC1394_BAYER_METHOD_SIMPLE, DC1394_BAYER_METHOD_BILINEAR, DC1394_BAYER_METHOD_HQLINEAR, DC1394_BAYER_METHOD_EDGESENSE,
                           DC1394_BAYER_METHOD_VNG, DC1394_BAYER_METHOD_AHD);
    INFO("tile " << tileName(tile) << " method " << (int)method);

    const int sx = 24, sy = 24;
    const int colour[3] = {200, 100, 50};
    auto mosaic = makeMosaic<uint8_t>(tile, sx, sy, colour);
    std::vector<uint8_t> rgb(sx * sy * 3, 0);
    REQUIRE(dc1394_bayer_decoding_8bit(mosaic.data(), rgb.data(), sx, sy, tile, method) == DC1394_SUCCESS);

    // Borders may be cleared or approximated, check the interior
    // (AHD works in CIELab, allow a little rounding)
    int tol = method == DC1394_BAYER_METHOD_AHD ? 3 : 1;
    for (int y = 4; y < sy - 4; y++)
        for (int x = 4; x < sx - 4; x++)
            for (int c = 0; c < 3; c++)
                REQUIRE(std::abs(rgb[(y * sx + x) * 3 + c] - colour[c]) <= tol);
}

// DC1394_BAYER_METHOD_AHD is not exercised at 16 bits: bayer.cpp:2610 checks
// `(int)y < height` on an unsigned y that wraps to UINT_MAX at the top/left
// border, so dc1394_bayer_AHD_uint16 reads far out of bounds and crashes.
TEST_CASE("bayer dc1394_bayer_decoding_16bit reconstructs a uniform colour", "[image][bayer]")
{
    auto tile = GENERATE(DC1394_COLOR_FILTER_RGGB, DC1394_COLOR_FILTER_GBRG, DC1394_COLOR_FILTER_GRBG, DC1394_COLOR_FILTER_BGGR);
    auto method = GENERATE(DC1394_BAYER_METHOD_NEAREST, DC1394_BAYER_METHOD_SIMPLE, DC1394_BAYER_METHOD_BILINEAR, DC1394_BAYER_METHOD_HQLINEAR, DC1394_BAYER_METHOD_EDGESENSE,
                           DC1394_BAYER_METHOD_VNG);
    INFO("tile " << tileName(tile) << " method " << (int)method);

    const int sx = 24, sy = 24;
    const int colour[3] = {3000, 1500, 700}; // 12-bit values
    auto mosaic = makeMosaic<uint16_t>(tile, sx, sy, colour);
    std::vector<uint16_t> rgb(sx * sy * 3, 0);
    REQUIRE(dc1394_bayer_decoding_16bit(mosaic.data(), rgb.data(), sx, sy, tile, method, 12) == DC1394_SUCCESS);

    int tol = 1;
    for (int y = 4; y < sy - 4; y++)
        for (int x = 4; x < sx - 4; x++)
            for (int c = 0; c < 3; c++)
                REQUIRE(std::abs(rgb[(y * sx + x) * 3 + c] - colour[c]) <= tol);
}

TEST_CASE("bayer downsample produces a half resolution image", "[image][bayer]")
{
    auto tile = GENERATE(DC1394_COLOR_FILTER_RGGB, DC1394_COLOR_FILTER_GBRG, DC1394_COLOR_FILTER_GRBG, DC1394_COLOR_FILTER_BGGR);
    INFO("tile " << tileName(tile));
    const int sx = 8, sy = 6;

    // Each 2x2 cell has its own colour so the mapping can be checked exactly
    std::vector<uint8_t> mosaic(sx * sy);
    for (int y = 0; y < sy; y++)
        for (int x = 0; x < sx; x++)
        {
            int cell = (y / 2) * (sx / 2) + (x / 2);
            int col = cellColour(tile, x, y);
            mosaic[y * sx + x] = col == 0 ? 10 + cell : col == 1 ? 100 + 2 * cell
                                                                 : 200 + cell;
        }
    std::vector<uint8_t> rgb((sx / 2) * (sy / 2) * 3, 0);
    REQUIRE(dc1394_bayer_decoding_8bit(mosaic.data(), rgb.data(), sx, sy, tile, DC1394_BAYER_METHOD_DOWNSAMPLE) == DC1394_SUCCESS);
    for (int cell = 0; cell < (sx / 2) * (sy / 2); cell++)
    {
        CHECK(rgb[cell * 3 + 0] == 10 + cell);
        CHECK(rgb[cell * 3 + 1] == 100 + 2 * cell); // average of two identical greens
        CHECK(rgb[cell * 3 + 2] == 200 + cell);
    }

    std::vector<uint16_t> mosaic16(mosaic.begin(), mosaic.end());
    for (auto &v : mosaic16)
        v *= 16;
    std::vector<uint16_t> rgb16((sx / 2) * (sy / 2) * 3, 0);
    REQUIRE(dc1394_bayer_decoding_16bit(mosaic16.data(), rgb16.data(), sx, sy, tile, DC1394_BAYER_METHOD_DOWNSAMPLE, 12) == DC1394_SUCCESS);
    for (size_t i = 0; i < rgb.size(); i++)
        REQUIRE(rgb16[i] == rgb[i] * 16);
}

TEST_CASE("bayer bilinear interpolates missing samples exactly on linear ramps", "[image][bayer]")
{
    // Every colour plane is the same horizontal ramp, bilinear is exact on it
    const int sx = 16, sy = 12;
    std::vector<uint8_t> mosaic(sx * sy);
    for (int y = 0; y < sy; y++)
        for (int x = 0; x < sx; x++)
            mosaic[y * sx + x] = 10 * x;
    std::vector<uint8_t> rgb(sx * sy * 3, 0);
    REQUIRE(dc1394_bayer_decoding_8bit(mosaic.data(), rgb.data(), sx, sy, DC1394_COLOR_FILTER_RGGB, DC1394_BAYER_METHOD_BILINEAR) == DC1394_SUCCESS);
    for (int y = 1; y < sy - 1; y++)
        for (int x = 1; x < sx - 1; x++)
            for (int c = 0; c < 3; c++)
                REQUIRE(std::abs(rgb[(y * sx + x) * 3 + c] - 10 * x) <= 1);

    // The 1-pixel border is cleared to black
    for (int c = 0; c < 3; c++)
    {
        CHECK(rgb[(0 * sx + 5) * 3 + c] == 0);
        CHECK(rgb[(5 * sx + 0) * 3 + c] == 0);
        CHECK(rgb[((sy - 1) * sx + 5) * 3 + c] == 0);
        CHECK(rgb[(5 * sx + sx - 1) * 3 + c] == 0);
    }
}

TEST_CASE("bayer decoding rejects invalid methods and filters", "[image][bayer]")
{
    const int sx = 8, sy = 8;
    std::vector<uint8_t> mosaic(sx * sy, 0), rgb(sx * sy * 3, 0);
    std::vector<uint16_t> mosaic16(sx * sy, 0), rgb16(sx * sy * 3, 0);

    CHECK(dc1394_bayer_decoding_8bit(mosaic.data(), rgb.data(), sx, sy, DC1394_COLOR_FILTER_RGGB, (dc1394bayer_method_t)42) == DC1394_INVALID_BAYER_METHOD);
    CHECK(dc1394_bayer_decoding_16bit(mosaic16.data(), rgb16.data(), sx, sy, DC1394_COLOR_FILTER_RGGB, (dc1394bayer_method_t)42, 16) == DC1394_INVALID_BAYER_METHOD);

    auto bad_tile = (dc1394color_filter_t)1000;
    CHECK(dc1394_bayer_decoding_8bit(mosaic.data(), rgb.data(), sx, sy, bad_tile, DC1394_BAYER_METHOD_NEAREST) == DC1394_INVALID_COLOR_FILTER);
    CHECK(dc1394_bayer_decoding_8bit(mosaic.data(), rgb.data(), sx, sy, bad_tile, DC1394_BAYER_METHOD_DOWNSAMPLE) == DC1394_INVALID_COLOR_FILTER);
    CHECK(dc1394_bayer_decoding_16bit(mosaic16.data(), rgb16.data(), sx, sy, bad_tile, DC1394_BAYER_METHOD_NEAREST, 16) == DC1394_INVALID_COLOR_FILTER);
}
