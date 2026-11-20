#include "common/widgets/double_list.h"
#include <catch2/catch_test_macros.hpp>

using satdump::widgets::DoubleList;

// Only the non-rendering selection logic is tested here

TEST_CASE("DoubleList selects from a fixed list", "[common][widgets][double_list]")
{
    DoubleList list("Samplerate");
    list.set_list({250e3, 1e6, 2.4e6}, false);

    CHECK(list.get_list().size() == 3);
    CHECK(list.get_value() == 250e3); // First entry by default

    CHECK(list.set_value(2.4e6));
    CHECK(list.get_value() == 2.4e6);

    // Values outside the list are rejected without manual entry
    CHECK_FALSE(list.set_value(3e6));
    CHECK(list.get_value() == 2.4e6);
}

TEST_CASE("DoubleList with manual entry accepts any value", "[common][widgets][double_list]")
{
    DoubleList list("Samplerate");
    list.set_list({1e6, 2e6}, true);

    // A "Manual" placeholder entry is appended
    REQUIRE(list.get_list().size() == 3);
    CHECK(list.get_list().back() == -1);
    CHECK(list.get_value() == 1e6);

    CHECK(list.set_value(3.2e6));
    CHECK(list.get_value() == 3.2e6);

    // While in manual mode a listed value keeps manual mode but updates the value
    CHECK(list.set_value(2e6));
    CHECK(list.get_value() == 2e6);
}

TEST_CASE("DoubleList set_list resets the selection", "[common][widgets][double_list]")
{
    DoubleList list("Rate");
    list.set_list({1, 2, 3}, false);
    list.set_value(3);
    CHECK(list.get_value() == 3);

    list.set_list({10, 20}, false);
    CHECK(list.get_value() == 10);
}
