#include "common/widgets/notated_num.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

using satdump::widgets::NotatedNum;

// Only the headless parts of the widget (construction, get/set and input parsing) are tested here

TEST_CASE("NotatedNum formats its initial value", "[common][widgets][notated_num]")
{
    NotatedNum<double> num("##id", 2400000, "sps");
    CHECK(num.get() == 2400000);
    CHECK(num.display_val == "2.4 Msps");

    num.set(137.1e6);
    CHECK(num.get() == 137.1e6);
    CHECK(num.display_val == "137.1 Msps");
}

TEST_CASE("NotatedNum::parse_input understands SI suffixes and units", "[common][widgets][notated_num]")
{
    NotatedNum<uint64_t> num("##freq", 0, "Hz");

    struct Case
    {
        std::string input;
        uint64_t value;
        std::string display;
    };
    for (Case c : {Case{"137.1M", 137100000, "137.1 MHz"}, Case{"137.1 MHz", 137100000, "137.1 MHz"}, Case{"1.7 ghz", 1700000000, "1.7 GHz"}, Case{"250k", 250000, "250 kHz"},
                   Case{"  42  ", 42, "42 Hz"}, Case{"5.5T", 5500000000000ULL, "5.5 THz"}, Case{"2P", 2000000000000000ULL, "2 PHz"}})
    {
        num.display_val = c.input;
        num.parse_input();
        INFO(c.input);
        CHECK(num.get() == c.value);
        CHECK(num.display_val == c.display);
    }
}

TEST_CASE("NotatedNum::parse_input reverts invalid input", "[common][widgets][notated_num]")
{
    NotatedNum<double> num("##id", 1000, "Hz");
    REQUIRE(num.display_val == "1 kHz");

    num.display_val = "garbage";
    num.parse_input();
    CHECK(num.get() == 1000);
    CHECK(num.display_val == "1 kHz");

    // Only units and whitespace: nothing left to parse
    num.display_val = " Hz ";
    num.parse_input();
    CHECK(num.get() == 1000);
    CHECK(num.display_val == "1 kHz");
}

TEST_CASE("NotatedNum<int> does not accept T and P suffixes", "[common][widgets][notated_num]")
{
    NotatedNum<int> num("##int", 0, "");

    num.display_val = "1G";
    num.parse_input();
    CHECK(num.get() == 1000000000);

    num.display_val = "-3k";
    num.parse_input();
    CHECK(num.get() == -3000);

    // 'T' is not a suffix for int, so stod parses the leading "3" and ignores the rest
    num.display_val = "3T";
    num.parse_input();
    CHECK(num.get() == 3);
}

TEST_CASE("NotatedNum<int64_t> handles negative values", "[common][widgets][notated_num]")
{
    NotatedNum<int64_t> num("##off", -5000, "Hz");
    CHECK(num.display_val == "-5 kHz");

    num.display_val = "-1.25M";
    num.parse_input();
    CHECK(num.get() == -1250000);
    CHECK(num.display_val == "-1.25 MHz");
}
