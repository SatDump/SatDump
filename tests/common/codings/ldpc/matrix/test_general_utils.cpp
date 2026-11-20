// general_utils.h is not self-contained (uses std::setw/std::stringstream without including them)
#include <iomanip>
#include <sstream>
#include "common/codings/ldpc/matrix/general_utils.h"
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <stdexcept>

TEST_CASE("ldpc split by delimiter keeps empty fields", "[common][codings][ldpc]")
{
    using V = std::vector<std::string>;
    CHECK(codings::ldpc::split("a,b,,c", ',') == V({"a", "b", "", "c"}));
    CHECK(codings::ldpc::split("abc", ',') == V({"abc"}));
    CHECK(codings::ldpc::split("", ',').empty());
}

TEST_CASE("ldpc split on whitespace drops empty tokens", "[common][codings][ldpc]")
{
    using V = std::vector<std::string>;
    CHECK(codings::ldpc::split("  3  5\t7\n") == V({"3", "5", "7"}));
    CHECK(codings::ldpc::split("   ").empty());
}

TEST_CASE("ldpc getline skips comments and blank lines", "[common][codings][ldpc]")
{
    std::stringstream ss("# comment\n\n   \nfirst\n#x\nsecond\n");
    std::string line;
    codings::ldpc::getline(ss, line);
    CHECK(line == "first");
    codings::ldpc::getline(ss, line);
    CHECK(line == "second");

    std::stringstream done("");
    done.get(); // sets eof
    CHECK_THROWS_AS(codings::ldpc::getline(done, line), std::runtime_error);
}
