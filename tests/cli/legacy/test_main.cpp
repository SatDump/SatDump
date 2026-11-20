#include "cli/cli_test_helpers.h"
#include "satdump_vars.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

// main_old() is the dispatcher behind "satdump legacy". Only the paths that return a value are
// called: the ones reaching the end of the function ("sdr_probe", or any command that
// succeeds) have no return statement, and "help" / "-h" call exit() (see
// tests/UNTESTED_ISSUES.md).
//
// main_old() replaces the global logger with one writing to stderr, so the output is checked
// by redirecting stderr, which is done with POSIX calls.

#ifndef _WIN32
using namespace cli_test;
using Catch::Matchers::ContainsSubstring;

namespace
{
    struct Result
    {
        int ret;
        std::string output;
    };

    // Runs "satdump legacy <args>", like LegacyCmdHandler::run() does
    Result runLegacy(std::vector<std::string> args)
    {
        ValueGuard<std::shared_ptr<slog::Logger>> logger_guard(logger);
        StderrCapture err;
        args.insert(args.begin(), "satdump legacy");
        int ret = call(main_old, args);
        return {ret, err.stop()};
    }
} // namespace

TEST_CASE("main_old without a command asks for one", "[cli][legacy][main]")
{
    Result r = runLegacy({});
    CHECK(r.ret == 1);
    CHECK_THAT(r.output, ContainsSubstring("Please specify either live/record or pipeline name!"));
    CHECK_THAT(r.output, ContainsSubstring("Use -h or help for information"));
    CHECK_THAT(r.output, !ContainsSubstring("Done! Goodbye"));
}

TEST_CASE("main_old prints the version", "[cli][legacy][main]")
{
    for (std::string cmd : {"version", "--v"})
    {
        INFO("command " << cmd);
        Result r = runLegacy({cmd});
        CHECK(r.ret == 0);
        CHECK_THAT(r.output, ContainsSubstring("This is SatDump v" + (std::string)satdump::SATDUMP_VERSION));
    }
}

TEST_CASE("main_old dispatches to the command named by its first argument", "[cli][legacy][main]")
{
    // Each command is given too few arguments, so that it only prints its own usage
    SECTION("live")
    {
        Result r = runLegacy({"live"});
        CHECK(r.ret == 1);
        CHECK_THAT(r.output, ContainsSubstring("Usage : satdump legacy live [pipeline_id]"));
    }

    SECTION("record")
    {
        Result r = runLegacy({"record"});
        CHECK(r.ret == 1);
        CHECK_THAT(r.output, ContainsSubstring("Usage : satdump legacy record [output_baseband"));
    }

    SECTION("autotrack")
    {
        Result r = runLegacy({"autotrack"});
        CHECK(r.ret == 1);
        CHECK_THAT(r.output, ContainsSubstring("Usage : satdump legacy autotrack autotrack_config.json"));
    }

    SECTION("anything else is an offline pipeline")
    {
        Result r = runLegacy({"metop_ahrpt", "baseband"});
        CHECK(r.ret == 1);
        CHECK_THAT(r.output, ContainsSubstring("Usage : satdump legacy [pipeline_id] [input_level] [input_file]"));
    }

    SECTION("command names are case sensitive")
    {
        Result r = runLegacy({"LIVE"});
        CHECK(r.ret == 1);
        CHECK_THAT(r.output, ContainsSubstring("[pipeline_id] [input_level] [input_file]"));
    }
}

TEST_CASE("main_old logs at trace level whatever the level was before", "[cli][legacy][main]")
{
    // The usage of a command is logged as errors, the version as info: both show up although
    // the test main turned the global logger off
    REQUIRE(logger->get_level() == slog::LOG_OFF);
    Result r = runLegacy({"version"});
    CHECK_THAT(r.output, ContainsSubstring("(I) This is SatDump v"));
    CHECK(logger->get_level() == slog::LOG_OFF);
}
#endif
