#include "utils/color.h"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdlib>

namespace
{
    std::array<int, 3> hsv(float h, float s, float v)
    {
        uint8_t rgb[3] = {0, 0, 0};
        satdump::hsv_to_rgb(h, s, v, rgb);
        return {rgb[0], rgb[1], rgb[2]};
    }

    // Channels are truncated from float, so allow one step of rounding error
    bool near(std::array<int, 3> a, std::array<int, 3> b)
    {
        for (int i = 0; i < 3; i++)
            if (std::abs(a[i] - b[i]) > 1)
                return false;
        return true;
    }
} // namespace

TEST_CASE("hsv_to_rgb primary and secondary hues at full saturation", "[utils][color]")
{
    CHECK(near(hsv(0.0f, 1, 1), {255, 0, 0}));
    CHECK(near(hsv(1.0f / 6.0f, 1, 1), {255, 255, 0}));
    CHECK(near(hsv(2.0f / 6.0f, 1, 1), {0, 255, 0}));
    CHECK(near(hsv(3.0f / 6.0f, 1, 1), {0, 255, 255}));
    CHECK(near(hsv(4.0f / 6.0f, 1, 1), {0, 0, 255}));
    CHECK(near(hsv(5.0f / 6.0f, 1, 1), {255, 0, 255}));
}

TEST_CASE("hsv_to_rgb intermediate hues interpolate between sectors", "[utils][color]")
{
    // 30 degrees is orange (255, 127.5, 0), 210 degrees is azure (0, 127.5, 255)
    CHECK(near(hsv(30.0f / 360.0f, 1, 1), {255, 127, 0}));
    CHECK(near(hsv(210.0f / 360.0f, 1, 1), {0, 127, 255}));
    // 330 degrees is rose (255, 0, 127.5)
    CHECK(near(hsv(330.0f / 360.0f, 1, 1), {255, 0, 127}));
}

TEST_CASE("hsv_to_rgb saturation and value scale the output", "[utils][color]")
{
    // Half saturation red is (255, 127.5, 127.5)
    CHECK(near(hsv(0.0f, 0.5f, 1), {255, 127, 127}));
    // Half value green is (0, 127.5, 0)
    CHECK(near(hsv(2.0f / 6.0f, 1, 0.5f), {0, 127, 0}));
    // Zero value is always black
    CHECK(hsv(0.3f, 1, 0) == std::array<int, 3>{0, 0, 0});
}

TEST_CASE("hsv_to_rgb zero saturation is gray", "[utils][color]")
{
    CHECK(hsv(0.7f, 0, 1) == std::array<int, 3>{255, 255, 255});
    CHECK(hsv(0.7f, 0, 0) == std::array<int, 3>{0, 0, 0});
    // The float to uint8 conversion truncates rather than rounds: 0.5 * 255 = 127.5 -> 127
    CHECK(hsv(0.2f, 0, 0.5f) == std::array<int, 3>{127, 127, 127});
}

TEST_CASE("hsv_to_rgb hue wraps around at 1.0", "[utils][color]")
{
    CHECK(near(hsv(1.0f, 1, 1), hsv(0.0f, 1, 1)));
    CHECK(near(hsv(1.5f, 1, 1), hsv(0.5f, 1, 1)));
    CHECK(near(hsv(2.0f + 4.0f / 6.0f, 1, 1), {0, 0, 255}));
}
