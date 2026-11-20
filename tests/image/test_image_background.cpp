#include "image/image_background.h"
#include "image/image_test_helpers.h"
#include "image/meta.h"
#include <catch2/catch_test_macros.hpp>

using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::remove_background without a projection leaves the image alone", "[image][image_background]")
{
    Image im = image_test::makePattern(8, 6, 6, 1);
    Image orig = im;
    float progress = -1;
    img::remove_background(im, &progress);
    CHECK(image_test::samePixels(im, orig));
    CHECK(progress == -1);
}

TEST_CASE("image::remove_background with an invalid projection leaves the image alone", "[image][image_background]")
{
    Image im = image_test::makePattern(16, 6, 6, 3);
    img::set_metadata_proj_cfg(im, {{"type", "not_a_projection"}});
    Image orig = im;
    img::remove_background(im, nullptr);
    CHECK(image_test::samePixels(im, orig));
}

TEST_CASE("image::remove_background clears off-disk pixels of a geostationary view", "[image][image_background]")
{
    // 40x40 pixels covering +-6000 km in the GEOS plane: the Earth disk
    // (about 5430 km radius) fills the centre, the corners are space
    const int N = 40;
    Image im(8, N, N, 1);
    im.fill(200);
    img::set_metadata_proj_cfg(
        im, {{"type", "geos"}, {"altitude", 35786023.0}, {"sweep_x", false}, {"offset_x", -6000000.0}, {"offset_y", 6000000.0}, {"scalar_x", 12000000.0 / N}, {"scalar_y", -12000000.0 / N}});
    float progress = -1;
    img::remove_background(im, &progress);

    REQUIRE(im.channels() == 4);
    // Corners are space: fully cleared, alpha included
    for (int c = 0; c < 4; c++)
    {
        CHECK(im.get(c, 0, 0) == 0);
        CHECK(im.get(c, N - 1, 0) == 0);
        CHECK(im.get(c, 0, N - 1) == 0);
        CHECK(im.get(c, N - 1, N - 1) == 0);
    }
    // Centre is on the disk: kept and made opaque
    for (int c = 0; c < 3; c++)
        CHECK(im.get(c, N / 2, N / 2) == 200);
    CHECK(im.get(3, N / 2, N / 2) == 255);
    CHECK(progress > 0.9f);
}
