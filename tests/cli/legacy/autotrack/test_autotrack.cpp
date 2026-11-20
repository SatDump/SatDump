#include "cli/cli_test_helpers.h"
#include "helpers/kepler_db.h"
#include "helpers/wait.h"
#include "legacy/autotrack/autotrack.h"
#include "pipeline/pipeline_test_helpers.h"
#include "satdump_vars.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

// "satdump legacy autotrack" is run against the built-in file source, with its rotator pointed
// at a closed 127.0.0.1 port, tracking a made-up satellite from a made-up location: no SDR, no
// rotator and no real TLE is involved. AutoTrackApp calls exit() when its parameters are
// incomplete or its source is not found, so only complete configurations are used.
//
// The tracked satellite is in a polar orbit and the station is next to the north pole, so that
// there is a pass on every orbit whenever the tests run. This matters: with no upcoming pass
// at all the scheduler reads out of bounds (see tests/UNTESTED_ISSUES.md). The satellite has no
// live or recorded downlink, so its passes start nothing.
//
// Everything in AutoTrackApp is private. What it does is observed through the log and through
// its status webserver, which is queried with the POSIX client of the helpers.

using namespace cli_test;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::StartsWith;

namespace
{
    const int SAMPLERATE = 48000;
    const int NORAD = 99001;

    // Circular polar orbit at about 830 km, with elements as of now
    satdump::KeplerData polarSatellite()
    {
        satdump::KeplerData k;
        k.satellite_number = NORAD;
        k.element_number = 1;
        k.name = "UNIT POLAR";
        k.designator = "2020-001A";
        k.epoch = (double)time(0);
        k.inclination = 90;
        k.right_ascension = 0;
        k.eccentricity = 0.001;
        k.argument_of_perigee = 0;
        k.mean_anomaly = 0;
        k.mean_motion = 14.2;
        k.derivative_mean_motion = 0;
        k.second_derivative_mean_motion = 0;
        k.bstar_drag_term = 0;
        k.revolutions_at_epoch = 1;
        return k;
    }

    struct AutotrackEnv : CommandEnv
    {
        // The tracker and scheduler read the TLE database, and register event handlers
        // capturing themselves (dropped with the event bus of CommandEnv)
        test_helpers::KeplerDbFixture keplers;
        // Every tracked object owns a pipeline selector, which needs a pipeline to select
        pipeline_test::PipelinesStateGuard pipelines;
        // Font of the schedule
        ValueGuard<std::string> respath{satdump::RESPATH, std::string(SATDUMP_TEST_SOURCE_DIR) + "/"};
        LogCapture log;
        std::string input, out;

        AutotrackEnv() : input(writeRampBaseband(dir, 1000)), out(dir.file("autotrack_out"))
        {
            keplers.add(polarSatellite());
            // The tracker lists satellites from this cache, which is normally filled by
            // KeplerDBHandler::init() and on TLE updates
            satdump::db_keplers->all_ = {keplers.stored(NORAD)};

            satdump::pipeline::Pipeline p;
            p.id = "unit_cli_autotrack";
            p.name = "Unit CLI Autotrack";
            satdump::pipeline::pipelines.push_back(p);
        }

        // A complete configuration
        nlohmann::json settings()
        {
            nlohmann::json s;
            s["output_folder"] = out;
            s["parameters"] = {{"source", "file"}, {"file_path", input}, {"samplerate", SAMPLERATE}, {"initial_frequency", 137100000}};
            s["qth"] = {{"lon", 15.5}, {"lat", 89.0}, {"alt", 35}};
            s["tracked_objects"] = {{{"norad", NORAD}}};
#ifndef _WIN32
            s["tracking"]["rotator_cfg"] = {{"address", "127.0.0.1"}, {"port", test_helpers::closedPort()}};
#endif
            return s;
        }
    };
} // namespace

TEST_CASE("main_autotrack prints its usage without a configuration file", "[cli][legacy][autotrack]")
{
    AutotrackEnv env;

    CHECK(call(main_autotrack, {"satdump legacy", "autotrack"}) == 1);

    INFO(env.log.dump());
    CHECK(env.log.contains("Usage : satdump legacy autotrack autotrack_config.json", slog::LOG_ERROR));
}

TEST_CASE("main_autotrack fails when the configuration file does not exist", "[cli][legacy][autotrack]")
{
    AutotrackEnv env;
    std::string missing = env.dir.file("missing.json");

    CHECK(call(main_autotrack, {"satdump legacy", "autotrack", missing}) == 1);

    INFO(env.log.dump());
    CHECK(env.log.contains(missing + " does not exist! Exiting", slog::LOG_CRIT));
    CHECK_FALSE(std::filesystem::exists(env.out));
}

#ifndef _WIN32
TEST_CASE("main_autotrack sets everything up and stops on its signal flag", "[cli][legacy][autotrack]")
{
    AutotrackEnv env;
    autotrack_should_exit = true;

    std::string config = env.dir.file("autotrack.json");
    test_helpers::writeFile(config, env.settings().dump(4));

    CHECK(call(main_autotrack, {"satdump legacy", "autotrack", config}) == 0);

    INFO(env.log.dump());
    CHECK(env.log.contains("Using QTH 15.5", slog::LOG_TRACE));
    CHECK(env.log.contains("Setup Done!", slog::LOG_INFO));
    CHECK(env.log.contains("Signal Received. Stopping.", slog::LOG_WARN));
    CHECK(std::filesystem::is_directory(env.out));

    // Nothing is recorded or processed for an object without downlinks
    CHECK(std::filesystem::is_empty(env.out));
    CHECK_FALSE(env.log.contains("Start webserver"));
}

TEST_CASE("AutoTrackApp serves its status over HTTP", "[cli][legacy][autotrack]")
{
    AutotrackEnv env;
    int port = test_helpers::closedPort();

    nlohmann::json settings = env.settings();
    settings["http_server"] = loopback(port);
    std::filesystem::create_directories(env.out);

    {
        AutoTrackApp app(settings, settings["parameters"], env.out);

        INFO(env.log.dump());
        CHECK(env.log.contains("Start webserver on " + loopback(port), slog::LOG_INFO));

        SECTION("/api is the status as JSON")
        {
            HttpResponse res = httpRequest(port, "/api");
            REQUIRE(res.ok);
            CHECK(res.hasHeader("Content-Type: application/json"));

            nlohmann::json status = nlohmann::json::parse(res.body);
            CHECK(status["frequency"] == 137100000);
            CHECK(status["object_tracker"]["object_name"] == "UNIT POLAR");
            CHECK(status["object_tracker"]["rotator_engaged"] == true);
            CHECK(status["object_tracker"]["rotator_tracking"] == true);

            // Only present while something is processed or recorded
            CHECK_FALSE(status.contains("live_pipeline"));
            CHECK_FALSE(status.contains("recording"));
            CHECK_FALSE(status.contains("vfos"));
        }

        SECTION("/ is a page that loads /status")
        {
            HttpResponse res = httpRequest(port, "/");
            REQUIRE(res.ok);
            CHECK(res.hasHeader("Content-Type: text/html"));
            CHECK_THAT(res.body, StartsWith("<!DOCTYPE html>"));
            CHECK_THAT(res.body, ContainsSubstring("<title>SatDump Status Page</title>"));
            CHECK_THAT(res.body, ContainsSubstring("var url = \"/status\";"));
        }

        SECTION("other pages do not exist")
        {
            HttpResponse res = httpRequest(port, "/unit_cli_no_such_page");
            REQUIRE(res.ok);
            CHECK(res.body == "Error 404");

            // /status is only matched exactly
            CHECK(httpRequest(port, "/status/").body == "Error 404");
        }

        SECTION("/polarplot.jpeg is a JPEG")
        {
            HttpResponse res = httpRequest(port, "/polarplot.jpeg");
            REQUIRE(res.ok);
            CHECK(res.hasHeader("Content-Type: image/jpeg"));
            REQUIRE(res.body.size() > 4);
            CHECK((uint8_t)res.body[0] == 0xFF);
            CHECK((uint8_t)res.body[1] == 0xD8);
            CHECK((uint8_t)res.body[res.body.size() - 2] == 0xFF);
            CHECK((uint8_t)res.body[res.body.size() - 1] == 0xD9);
        }

        SECTION("/status is the body of the page")
        {
            // The page needs the range of the tracked satellite, which the tracker only has
            // once its thread has loaded the elements. Asking earlier would take the test
            // executable down (see tests/UNTESTED_ISSUES.md).
            REQUIRE(test_helpers::waitUntil(
                [&]()
                {
                    nlohmann::json status = nlohmann::json::parse(httpRequest(port, "/api").body, nullptr, false);
                    return status.is_object() && status["object_tracker"].contains("sat_current_range");
                },
                test_helpers::kTimeout, std::chrono::milliseconds(10)));

            HttpResponse res = httpRequest(port, "/status");
            REQUIRE(res.ok);
            CHECK(res.hasHeader("Content-Type: text/html"));
            CHECK_THAT(res.body, ContainsSubstring("<p>Hardware: <span class=\"fakeinput\">File Source</span></p>"));
            CHECK_THAT(res.body, ContainsSubstring("<p>Started: <span class=\"fakeinput\">YES</span></p>"));
            CHECK_THAT(res.body, ContainsSubstring("<p>Sample rate: <span class=\"fakeinput\">0.048000</span> Msps</p>"));
            CHECK_THAT(res.body, ContainsSubstring("<p>Frequency: <span class=\"fakeinput\">137.100000</span> MHz</p>"));
            CHECK_THAT(res.body, ContainsSubstring("<p>Current object: <span class=\"fakeinput\">UNIT POLAR</span></p>"));
            CHECK_THAT(res.body, ContainsSubstring("<span class=\"fakeinput true\">engaged</span>"));
            CHECK_THAT(res.body, ContainsSubstring("<span class=\"fakeinput true\">tracking</span>"));
            CHECK_THAT(res.body, ContainsSubstring("<img src=\"polarplot.jpeg?r="));
            CHECK_THAT(res.body, ContainsSubstring("<h2>Schedule</h2><img src=\"schedule.jpeg?r="));
            CHECK_THAT(res.body, !ContainsSubstring("<h2>FFT</h2>"));
        }

        SECTION("/schedule.jpeg is a JPEG")
        {
            HttpResponse res = httpRequest(port, "/schedule.jpeg");
            REQUIRE(res.ok);
            CHECK(res.hasHeader("Content-Type: image/jpeg"));
            REQUIRE(res.body.size() > 4);
            CHECK((uint8_t)res.body[0] == 0xFF);
            CHECK((uint8_t)res.body[1] == 0xD8);
        }

        SECTION("the FFT is not served unless enabled")
        {
            // The handler exists, but its callback is the default empty one
            HttpResponse res = httpRequest(port, "/fft.jpeg");
            REQUIRE(res.ok);
            CHECK(res.body.empty());
            app.web_heartbeat();
        }
    }

    // Destroying the app stops the webserver
    CHECK_FALSE(portIsOpen(port));
}

TEST_CASE("AutoTrackApp sets its FFT up when enabled", "[cli][legacy][autotrack]")
{
    AutotrackEnv env;
    int port = test_helpers::closedPort();

    nlohmann::json settings = env.settings();
    settings["http_server"] = loopback(port);
    settings["parameters"]["fft_enable"] = true;
    settings["parameters"]["fft_size"] = 1024;
    settings["parameters"]["fft_rate"] = 20;
    settings["parameters"]["fft_min"] = -100;
    settings["parameters"]["fft_max"] = 0;
    settings["parameters"]["fft_avgn"] = 5.0;
    std::filesystem::create_directories(env.out);

    {
        AutoTrackApp app(settings, settings["parameters"], env.out);

        INFO(env.log.dump());
        CHECK(env.log.contains("FFT GOOD!", slog::LOG_CRIT));
        // Set once in the constructor, and again when the source starts: 48000 / 20 samples per FFT
        CHECK(env.log.contains("FFT Rate 2400, Samplerate 48000, Final Size 1024, Skip 1376", slog::LOG_TRACE));

        // /fft.jpeg is not requested: drawing the plot writes out of bounds (see
        // tests/UNTESTED_ISSUES.md). The FFT branch is only enabled by such a request, so the
        // heartbeat has nothing to shut down.
        app.web_heartbeat();
        CHECK_FALSE(env.log.contains("Enabling FFT"));
        CHECK_FALSE(env.log.contains("Shutting down FFT"));

        // The rest of the status is served as usual
        nlohmann::json status = nlohmann::json::parse(httpRequest(port, "/api").body);
        CHECK(status["frequency"] == 137100000);
    }

    CHECK_FALSE(portIsOpen(port));
}

TEST_CASE("AutoTrackApp leaves the source stopped when idle if asked to", "[cli][legacy][autotrack]")
{
    AutotrackEnv env;
    int port = test_helpers::closedPort();

    nlohmann::json settings = env.settings();
    settings["http_server"] = loopback(port);
    // With the source never started, its file is never opened: a missing one is not an error
    settings["parameters"]["file_path"] = env.dir.file("missing.cf32");
    std::filesystem::create_directories(env.out);

    SECTION("stop_sdr_when_idle")
    {
        settings["tracking"]["autotrack_cfg"] = {{"stop_sdr_when_idle", true}};
        AutoTrackApp app(settings, settings["parameters"], env.out);

        INFO(env.log.dump());
        CHECK_FALSE(env.log.contains("Invalid file path"));
        nlohmann::json status = nlohmann::json::parse(httpRequest(port, "/api").body);
        CHECK(status["frequency"] == 137100000);
    }

    SECTION("by default the source is started right away")
    {
        AutoTrackApp app(settings, settings["parameters"], env.out);

        // Starting fails on the missing file, which is logged and otherwise ignored
        INFO(env.log.dump());
        CHECK(env.log.contains("Invalid file path", slog::LOG_ERROR));
        CHECK(httpRequest(port, "/api").ok);
    }
}
#endif
