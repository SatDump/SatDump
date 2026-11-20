#include "cli_test_helpers.h"
#include "init.h"
#include "legacy/offline.h"
#include "pipeline/pipeline_test_helpers.h"
#include <catch2/catch_test_macros.hpp>

using namespace cli_test;
using namespace pipeline_test;
namespace pl = satdump::pipeline;

namespace
{
    // A registered pipeline made of recording modules: baseband -> unit_cli_a -> unit_cli_b
    struct OfflineEnv
    {
        ModuleRegistryGuard reg;
        PipelinesStateGuard pipelines;
        EventBusGuard bus;
        CallsGuard calls;
        ConfigGuard cfg;
        ValueGuard<std::string> tle_override{satdump::tle_file_override};
        TempDir dir{"cli_offline"};
        LogCapture log;
        std::string input, out;

        OfflineEnv()
        {
            input = dir.file("input.raw");
            out = dir.file("out");
            writeFile(input, "DATA");

            pl::modules_registry.push_back(recordingEntry("unit_cli_a"));
            pl::modules_registry.push_back(recordingEntry("unit_cli_b"));

            pl::Pipeline p;
            p.id = "unit_cli_offline";
            p.name = "Unit CLI Offline";
            p.live = false;
            p.steps = {{"baseband", "", {}, ""}, {"soft", "unit_cli_a", {}, ""}, {"frames", "unit_cli_b", {{"from_pipeline", 1}}, ""}};
            pl::pipelines.push_back(p);
        }

        // "satdump legacy <args>" as main_old() hands it over
        int run(std::vector<std::string> args)
        {
            args.insert(args.begin(), "satdump legacy");
            return call(main_offline, args);
        }
    };
} // namespace

TEST_CASE("main_offline prints its usage when arguments are missing", "[cli][legacy][offline]")
{
    OfflineEnv env;

    CHECK(env.run({"unit_cli_offline", "baseband", env.input}) == 1);

    INFO(env.log.dump());
    CHECK(env.log.contains("Usage : satdump legacy [pipeline_id] [input_level] [input_file] [output_file_or_directory]", slog::LOG_ERROR));
    CHECK(callsOf("construct").empty());
    CHECK_FALSE(std::filesystem::exists(env.out));
}

TEST_CASE("main_offline runs the pipeline from the given input level", "[cli][legacy][offline]")
{
    OfflineEnv env;

    SECTION("from the first level")
    {
        CHECK(env.run({"unit_cli_offline", "baseband", env.input, env.out}) == 0);

        auto proc = callsOf("process");
        REQUIRE(proc.size() == 2);
        CHECK(proc[0].id == "unit_cli_a");
        CHECK(proc[0].input == env.input);
        CHECK(proc[0].hint == env.out + "/unit_cli_offline");
        CHECK(proc[1].id == "unit_cli_b");
        CHECK(readFile(env.out + "/unit_cli_offline_unit_cli_b.bin") == "DATA[unit_cli_a][unit_cli_b]");
    }

    SECTION("from an intermediate level")
    {
        CHECK(env.run({"unit_cli_offline", "soft", env.input, env.out}) == 0);

        auto proc = callsOf("process");
        REQUIRE(proc.size() == 1);
        CHECK(proc[0].id == "unit_cli_b");
        CHECK(readFile(env.out + "/unit_cli_offline_unit_cli_b.bin") == "DATA[unit_cli_b]");
    }

    INFO(env.log.dump());
    CHECK(env.log.contains("Starting processing pipeline unit_cli_offline...", slog::LOG_INFO));
    CHECK(env.log.contains("Output file : " + env.out, slog::LOG_DEBUG));
}

TEST_CASE("main_offline creates the output directory", "[cli][legacy][offline]")
{
    OfflineEnv env;
    std::string nested = env.dir.file("a/b/out");

    CHECK(env.run({"unit_cli_offline", "baseband", env.input, nested}) == 0);
    CHECK(std::filesystem::is_directory(nested));
    CHECK(std::filesystem::exists(nested + "/unit_cli_offline_unit_cli_b.bin"));
}

TEST_CASE("main_offline passes the extra flags to the modules", "[cli][legacy][offline]")
{
    OfflineEnv env;

    CHECK(env.run({"unit_cli_offline", "baseband", env.input, env.out, "--samplerate", "6e6", "--baseband_format", "cs16", "--dc_block", "--from_pipeline", "2"}) == 0);

    auto proc = callsOf("process");
    REQUIRE(proc.size() == 2);
    for (auto &c : proc)
    {
        INFO("module " << c.id);
        CHECK(c.params["samplerate"] == 6e6);
        CHECK(c.params["baseband_format"] == "cs16");
        CHECK(c.params["dc_block"] == true);
    }

    // Flags override the parameters set by the pipeline
    CHECK(proc[0].params["from_pipeline"] == 2);
    CHECK(proc[1].params["from_pipeline"] == 2);
}

TEST_CASE("main_offline takes parameters from the input file name", "[cli][legacy][offline]")
{
    OfflineEnv env;

    // A SatDump-style recording name: timestamp, samplerate and frequency
    std::string recording = env.dir.file("2023-06-01_12-00-00_2000000SPS_137100000Hz.cs16");
    writeFile(recording, "DATA");

    SECTION("when no flag gives them")
    {
        CHECK(env.run({"unit_cli_offline", "baseband", recording, env.out}) == 0);
        auto proc = callsOf("process");
        REQUIRE(proc.size() == 2);
        CHECK(proc[0].params["samplerate"] == 2000000);
        CHECK(proc[0].params["frequency"] == 137100000);
        CHECK(proc[0].params["baseband_format"] == "cs16");
        CHECK(proc[0].params.contains("start_timestamp"));
    }

    SECTION("flags win over the file name")
    {
        CHECK(env.run({"unit_cli_offline", "baseband", recording, env.out, "--samplerate", "1000000"}) == 0);
        auto proc = callsOf("process");
        REQUIRE(proc.size() == 2);
        CHECK(proc[0].params["samplerate"] == 1000000);
        CHECK(proc[0].params["frequency"] == 137100000);
    }
}

TEST_CASE("main_offline sets the TLE override", "[cli][legacy][offline]")
{
    OfflineEnv env;

    SECTION("from --tle_override")
    {
        satdump::tle_file_override = "";
        CHECK(env.run({"unit_cli_offline", "baseband", env.input, env.out, "--tle_override", env.dir.file("my.tle")}) == 0);
        CHECK(satdump::tle_file_override == env.dir.file("my.tle"));
    }

    SECTION("and clears it without the flag")
    {
        satdump::tle_file_override = "left_over.tle";
        CHECK(env.run({"unit_cli_offline", "baseband", env.input, env.out}) == 0);
        CHECK(satdump::tle_file_override == "");
    }
}

TEST_CASE("main_offline fails cleanly when the pipeline cannot run", "[cli][legacy][offline]")
{
    OfflineEnv env;

    SECTION("unknown pipeline")
    {
        CHECK(env.run({"unit_cli_no_such_pipeline", "baseband", env.input, env.out}) == 1);
        INFO(env.log.dump());
        CHECK(env.log.contains("Fatal error running pipeline : Pipeline unit_cli_no_such_pipeline not found!", slog::LOG_ERROR));
        CHECK(callsOf("construct").empty());
    }

    SECTION("pipeline using a module that is not registered")
    {
        pl::Pipeline p;
        p.id = "unit_cli_broken";
        p.name = "Unit CLI Broken";
        p.steps = {{"baseband", "", {}, ""}, {"soft", "unit_cli_missing_module", {}, ""}};
        pl::pipelines.push_back(p);

        CHECK(env.run({"unit_cli_broken", "baseband", env.input, env.out}) == 1);
        INFO(env.log.dump());
        CHECK(env.log.contains("Fatal error running pipeline : ", slog::LOG_ERROR));
        CHECK(env.log.contains("unit_cli_missing_module"));
    }
}

TEST_CASE("main_offline does nothing for an input level the pipeline does not have", "[cli][legacy][offline]")
{
    OfflineEnv env;

    // Pipeline::run() treats every step as already done, so a mistyped level is not an error
    CHECK(env.run({"unit_cli_offline", "unit_cli_no_such_level", env.input, env.out}) == 0);
    CHECK(callsOf("construct").empty());
    CHECK(std::filesystem::is_directory(env.out));
    CHECK(std::filesystem::is_empty(env.out));
}
