#include "common/cli_utils.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <string>
#include <vector>

namespace
{
    // Holds mutable copies of the arguments, since the parser takes char *argv[]
    struct Args
    {
        std::vector<std::string> storage;
        std::vector<char *> ptrs;

        Args(std::vector<std::string> a) : storage(std::move(a))
        {
            for (auto &s : storage)
                ptrs.push_back(s.data());
        }

        nlohmann::json parse(std::map<std::string, std::optional<std::type_index>> strong = {})
        {
            return parse_common_flags(ptrs.size(), ptrs.data(), strong);
        }
    };
} // namespace

TEST_CASE("parse_common_flags with no arguments returns empty json", "[common][cli_utils]")
{
    CHECK(parse_common_flags(0, nullptr).empty());
}

TEST_CASE("parse_common_flags parses integers, doubles, bools and strings", "[common][cli_utils]")
{
    Args args({"--int", "42", "--neg", "-7", "--dbl", "2.5", "--exp", "1e3",
               "--yes", "true", "--no", "false", "--name", "hello"});
    nlohmann::json p = args.parse();

    REQUIRE(p["int"].is_number_integer());
    CHECK(p["int"].get<long long>() == 42);
    REQUIRE(p["neg"].is_number_integer());
    CHECK(p["neg"].get<long long>() == -7);
    REQUIRE(p["dbl"].is_number_float());
    CHECK(p["dbl"].get<double>() == Catch::Approx(2.5));
    // Whole-valued numbers become integers, even in exponent notation
    REQUIRE(p["exp"].is_number_integer());
    CHECK(p["exp"].get<long long>() == 1000);
    REQUIRE(p["yes"].is_boolean());
    CHECK(p["yes"].get<bool>() == true);
    REQUIRE(p["no"].is_boolean());
    CHECK(p["no"].get<bool>() == false);
    REQUIRE(p["name"].is_string());
    CHECK(p["name"].get<std::string>() == "hello");
    CHECK(p.size() == 7);
}

TEST_CASE("parse_common_flags treats flags without values as switches", "[common][cli_utils]")
{
    Args args({"--first", "--second", "value", "--last"});
    nlohmann::json p = args.parse();

    CHECK(p["first"] == true);
    CHECK(p["second"] == "value");
    CHECK(p["last"] == true);
    CHECK(p.size() == 3);
}

TEST_CASE("parse_common_flags ignores positional arguments", "[common][cli_utils]")
{
    Args args({"satdump", "pipeline", "input.cf32", "--samplerate", "6e6", "out_dir"});
    nlohmann::json p = args.parse();

    CHECK(p.size() == 1);
    CHECK(p["samplerate"].get<long long>() == 6000000);
}

TEST_CASE("parse_common_flags keeps IP addresses and words as strings", "[common][cli_utils]")
{
    Args args({"--ip", "192.168.1.10", "--e", "e", "--mixed", "12abc"});
    nlohmann::json p = args.parse();

    CHECK(p["ip"] == "192.168.1.10");
    CHECK(p["e"] == "e");
    CHECK(p["mixed"] == "12abc");
}

TEST_CASE("parse_common_flags honours strong string types", "[common][cli_utils]")
{
    Args args({"--id", "123", "--other", "123"});
    nlohmann::json p = args.parse({{"id", std::type_index(typeid(std::string))}});

    REQUIRE(p["id"].is_string());
    CHECK(p["id"] == "123");
    REQUIRE(p["other"].is_number_integer());
    CHECK(p["other"] == 123);
}

// BUG: values containing only digits, '.', 'e' and '-' are passed to std::stod,
// which silently parses a prefix, so a date like "2024-01-02" becomes 2024.
TEST_CASE("parse_common_flags keeps dash separated values as strings", "[common][cli_utils][!shouldfail]")
{
    Args args({"--date", "2024-01-02"});
    nlohmann::json p = args.parse();

    CHECK(p["date"].is_string());
    CHECK(p["date"] == "2024-01-02");
}
