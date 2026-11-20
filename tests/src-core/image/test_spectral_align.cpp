#include "image/spectral_align.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cmath>

using Catch::Approx;
using satdump::image::Image;

namespace
{
    // A few Gaussian blobs near the centre, offset by (dx, dy)
    Image makeBlobs(int depth, int w, int h, double dx, double dy)
    {
        const double blobs[][3] = {{0.40, 0.45, 3.0}, {0.58, 0.40, 2.0}, {0.50, 0.62, 4.0}, {0.62, 0.58, 1.5}};
        Image im(depth, w, h, 1);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                double v = 0;
                for (auto &b : blobs)
                {
                    double ddx = x - (b[0] * w + dx);
                    double ddy = y - (b[1] * h + dy);
                    v += exp(-(ddx * ddx + ddy * ddy) / (2 * b[2] * b[2]));
                }
                im.setf(0, x, y, std::min(1.0, v * 0.8));
            }
        return im;
    }
} // namespace

TEST_CASE("image::correlate_fft2d finds zero offset for identical images", "[image][spectral_align]")
{
    auto depth = GENERATE(8, 16);
    Image a = makeBlobs(depth, 64, 48, 0, 0);
    Image b = a;
    double x = 99, y = 99;
    REQUIRE_FALSE(satdump::image::correlate_fft2d(a, b, x, y));
    CHECK(x == Approx(0).margin(0.25));
    CHECK(y == Approx(0).margin(0.25));
}

TEST_CASE("image::correlate_fft2d recovers integer translations", "[image][spectral_align]")
{
    // The result is the offset that moves the target back onto the reference,
    // i.e. the negated shift applied to the target
    auto shift = GENERATE(std::make_pair(3, -2), std::make_pair(-5, 4), std::make_pair(0, 6), std::make_pair(7, 0));
    INFO("shift " << shift.first << ", " << shift.second);
    Image ref = makeBlobs(16, 64, 64, 0, 0);
    Image tgt = makeBlobs(16, 64, 64, shift.first, shift.second);
    double x, y;
    REQUIRE_FALSE(satdump::image::correlate_fft2d(ref, tgt, x, y));
    CHECK(x == Approx(-shift.first).margin(0.5));
    CHECK(y == Approx(-shift.second).margin(0.5));
}

TEST_CASE("image::correlate_fft2d rejects mismatched images", "[image][spectral_align]")
{
    Image a(16, 32, 32, 1);
    double x = 0, y = 0;

    Image other_depth(8, 32, 32, 1);
    CHECK(satdump::image::correlate_fft2d(a, other_depth, x, y));
    Image other_width(16, 31, 32, 1);
    CHECK(satdump::image::correlate_fft2d(a, other_width, x, y));
    Image other_height(16, 32, 30, 1);
    CHECK(satdump::image::correlate_fft2d(a, other_height, x, y));
    Image rgb(16, 32, 32, 3);
    CHECK(satdump::image::correlate_fft2d(rgb, rgb, x, y));
    CHECK(satdump::image::correlate_fft2d(a, rgb, x, y));
}
