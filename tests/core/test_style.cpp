#include "core/style.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <string>

// style::setStyle() and setFonts() need a live ImGui context, so only the theme colour parser
// is tested. It is not declared in style.h but is exported by satdump_core.
namespace style
{
    void hexToImVec4(std::string color_hex, ImVec4 *this_color);
}

using Catch::Approx;

TEST_CASE("hexToImVec4 parses RRGGBBAA colours", "[core][style]")
{
    ImVec4 c(0, 0, 0, 0);
    style::hexToImVec4("FF8000C0", &c);
    CHECK(c.x == Approx(1.0f));
    CHECK(c.y == Approx(128.0f / 255.0f));
    CHECK(c.z == Approx(0.0f));
    CHECK(c.w == Approx(192.0f / 255.0f));

    style::hexToImVec4("00000000", &c);
    CHECK(c.x == 0.0f);
    CHECK(c.w == 0.0f);
}

TEST_CASE("hexToImVec4 ignores non hex characters and accepts lower case", "[core][style]")
{
    ImVec4 c(0, 0, 0, 0);
    style::hexToImVec4("#ff00ff80", &c);
    CHECK(c.x == Approx(1.0f));
    CHECK(c.y == Approx(0.0f));
    CHECK(c.z == Approx(1.0f));
    CHECK(c.w == Approx(128.0f / 255.0f));

    style::hexToImVec4(" 0x: 10-20-30-40 ", &c);
    // "0x" contributes a leading 0 digit, leaving 9 digits, so this one is rejected
    CHECK(c.x == Approx(1.0f));

    style::hexToImVec4(" #10-20-30-40 ", &c);
    CHECK(c.x == Approx(16.0f / 255.0f));
    CHECK(c.y == Approx(32.0f / 255.0f));
    CHECK(c.z == Approx(48.0f / 255.0f));
    CHECK(c.w == Approx(64.0f / 255.0f));
}

TEST_CASE("hexToImVec4 leaves the colour untouched on invalid input", "[core][style]")
{
    ImVec4 c(0.1f, 0.2f, 0.3f, 0.4f);
    style::hexToImVec4("FFFFFF", &c); // RGB without alpha
    style::hexToImVec4("", &c);
    style::hexToImVec4("FFFFFFFFFF", &c);
    style::hexToImVec4("not a colour", &c);
    CHECK(c.x == 0.1f);
    CHECK(c.y == 0.2f);
    CHECK(c.z == 0.3f);
    CHECK(c.w == 0.4f);
}

TEST_CASE("style::Theme default font settings", "[core][style]")
{
    style::Theme t;
    CHECK(t.font == "Roboto-Medium");
    CHECK(t.font_size == 16.0f);
}
