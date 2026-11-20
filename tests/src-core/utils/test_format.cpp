#include "utils/format.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace satdump;

TEST_CASE("to_string_with_precision uses fixed notation", "[utils][format]")
{
    CHECK(to_string_with_precision(3.14159, 2) == "3.14");
    CHECK(to_string_with_precision(2.5, 0) == "2");
    CHECK(to_string_with_precision(1.0, 3) == "1.000");
    CHECK(to_string_with_precision(1.5f) == "1.500000");
    CHECK(to_string_with_precision(-0.126, 2) == "-0.13");
}

TEST_CASE("svformat behaves like printf", "[utils][format]")
{
    CHECK(svformat("plain") == "plain");
    CHECK(svformat("%d-%s", 5, "a") == "5-a");
    CHECK(svformat("%05.1f MHz", 137.1) == "137.1 MHz");
    CHECK(svformat("%02X%02X", 0xAB, 0x01) == "AB01");
}

// Bug: utils/format.h:63-65 when the output exceeds the initial 1024-byte buffer, svformat resizes
// to the length snprintf reported, which leaves no room for the terminating NUL, so
// the last character is dropped. [!shouldfail] keeps the suite green until that is
// fixed, at which point this test starts failing and the tag should be removed.
TEST_CASE("svformat handles output longer than its initial buffer", "[utils][format][!shouldfail]")
{
    std::string big(2000, 'x');
    std::string formatted = svformat("%s", big.c_str());

    CHECK(formatted.size() == big.size());
    CHECK((formatted == big));
}
