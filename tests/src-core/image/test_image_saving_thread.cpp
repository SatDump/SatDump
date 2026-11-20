#include "image/image_saving_thread.h"
#include "image/image_test_helpers.h"
#include <catch2/catch_test_macros.hpp>

using satdump::image::Image;
namespace img = satdump::image;

TEST_CASE("image::ImageSavingThread without a thread saves synchronously", "[image][image_saving_thread]")
{
    image_test::TempDir dir;
    Image in = image_test::makePattern(8, 4, 4, 1);
    {
        img::ImageSavingThread saver(false);
        CHECK_FALSE(saver.use_thread);
        saver.push(in, dir.file("sync.png"));
        // Already on disk before the saver goes away
        CHECK(std::filesystem::exists(dir.file("sync.png")));
    }
    Image out;
    img::load_png(out, dir.file("sync.png"));
    CHECK(image_test::samePixels(out, in));
}

TEST_CASE("image::ImageSavingThread flushes its queue on destruction", "[image][image_saving_thread]")
{
    image_test::TempDir dir;
    Image a = image_test::makePattern(8, 4, 4, 1, 1);
    Image b = image_test::makePattern(16, 3, 5, 3, 2);
    {
        img::ImageSavingThread saver(true);
        saver.push(a, dir.file("a.png"));
        saver.push(b, dir.file("b.ppm"));
        // The pushed images are copies, changing them now has no effect
        a.fill(0);
    }
    Image ra, rb;
    img::load_png(ra, dir.file("a.png"));
    img::load_pbm(rb, dir.file("b.ppm"));
    Image expected_a = image_test::makePattern(8, 4, 4, 1, 1);
    CHECK(image_test::samePixels(ra, expected_a));
    REQUIRE(rb.channels() == 3);
    CHECK(image_test::samePixels(rb, b));
}
