#include "common/utils.h"
#include "common/wav.h"
#include "core/config.h"
#include "helpers/temp_files.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

namespace
{
    using test_helpers::uniqueTempPath;

    // Temporarily sets the millisecond filename option, restoring the config afterwards
    struct MillisConfig
    {
        nlohmann::ordered_json saved;
        MillisConfig(bool millis) : saved(satdump::satdump_cfg.main_cfg) { satdump::satdump_cfg.main_cfg["user_interface"]["recorder_baseband_filename_millis_precision"]["value"] = millis; }
        ~MillisConfig() { satdump::satdump_cfg.main_cfg = saved; }
    };

    const time_t T_2024_01_02_030405 = 1704164645; // 2024-01-02 03:04:05 UTC
} // namespace

TEST_CASE("getFilesize returns the size of a file in bytes", "[common][utils]")
{
    test_helpers::TempFile path_file("size.bin");
    auto path = path_file.path;
    {
        std::ofstream f(path, std::ios::binary);
        std::string data(1234, 'x');
        f.write(data.data(), data.size());
    }
    CHECK(getFilesize(path.string()) == 1234);
    std::filesystem::remove(path);

    {
        std::ofstream f(path, std::ios::binary);
    }
    CHECK(getFilesize(path.string()) == 0);
}

TEST_CASE("getFilesize on a missing file returns all ones", "[common][utils]")
{
    // tellg() fails with -1, which is converted to the maximum uint64_t
    CHECK(getFilesize(uniqueTempPath("missing.bin").string()) == UINT64_MAX);
}

TEST_CASE("prepareAutomatedPipelineFolder creates a timestamped folder", "[common][utils]")
{
    test_helpers::TempDir base_dir("live");
    auto base = base_dir.path;

    std::string dir = prepareAutomatedPipelineFolder(T_2024_01_02_030405, 137.1e6, "noaa_apt", base.string());

    // Minutes resolution; frequency uses format_notated
    CHECK(dir == base.string() + "/2024-01-02_03-04_noaa_apt_137.1 MHz");
    CHECK(std::filesystem::is_directory(dir));
}

TEST_CASE("prepareBasebandFileName formats time, samplerate and frequency", "[common][utils]")
{
    SECTION("without milliseconds")
    {
        MillisConfig cfg(false);
        CHECK(prepareBasebandFileName(T_2024_01_02_030405 + 0.678, 2400000, 137100000) == "2024-01-02_03-04-05_2400000SPS_137100000Hz");
        CHECK(prepareBasebandFileName(0, 1, 2) == "1970-01-01_00-00-00_1SPS_2Hz");
    }

    SECTION("with milliseconds, zero padded")
    {
        MillisConfig cfg(true);
        CHECK(prepareBasebandFileName(T_2024_01_02_030405 + 0.678, 2400000, 137100000) == "2024-01-02_03-04-05-678_2400000SPS_137100000Hz");
        CHECK(prepareBasebandFileName(T_2024_01_02_030405 + 0.005, 2400000, 137100000) == "2024-01-02_03-04-05-005_2400000SPS_137100000Hz");
    }
}

TEST_CASE("prepareBasebandFileName round-trips through tryParseFilenameMetadata", "[common][utils]")
{
    for (bool millis : {false, true})
    {
        MillisConfig cfg(millis);
        std::string name = prepareBasebandFileName(T_2024_01_02_030405 + 0.25, 6000000, 1700000000) + ".cs16";
        wav::FileMetadata md = wav::tryParseFilenameMetadata(name);

        INFO(name);
        CHECK(md.timestamp == T_2024_01_02_030405);
        CHECK(md.samplerate == 6000000);
        CHECK(md.frequency == 1700000000);
        CHECK(md.baseband_format == "cs16");
    }
}

TEST_CASE("prepareBasebandFileName throws when the config key is missing", "[common][utils]")
{
    nlohmann::ordered_json saved = satdump::satdump_cfg.main_cfg;
    satdump::satdump_cfg.main_cfg = nlohmann::ordered_json::object();
    CHECK_THROWS(prepareBasebandFileName(0, 1, 2));
    satdump::satdump_cfg.main_cfg = saved;
}

// Bug: utils.cpp:55 rounds the millisecond field with setprecision(0), so a
// fraction >= .9995 renders as "1000" instead of a 3 digit field.
TEST_CASE("prepareBasebandFileName keeps the millisecond field at three digits", "[common][utils][!shouldfail]")
{
    MillisConfig cfg(true);
    std::string name = prepareBasebandFileName(T_2024_01_02_030405 + 0.9999, 2400000, 137100000);
    CHECK(name.find("-1000_") == std::string::npos);
}
