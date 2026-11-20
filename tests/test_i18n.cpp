#include "i18n.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("i18n _() returns the message itself without a translation", "[i18n]")
{
    // No text domain or locale is set up in the tests, so gettext falls back to the msgid
    CHECK(std::string(_("SatDump unit test string with no translation")) == "SatDump unit test string with no translation");
}
