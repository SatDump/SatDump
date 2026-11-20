#include "satdump_vars.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("satdump_vars version strings are configured", "[satdump_vars]")
{
    CHECK_FALSE(satdump::SATDUMP_VERSION.empty());
    // A leftover @VAR@ placeholder would mean the file was not configured by CMake
    CHECK(satdump::SATDUMP_VERSION.find('@') == std::string::npos);
    CHECK(satdump::SATDUMP_VERSION_TAG.find('@') == std::string::npos);
    CHECK(satdump::RESOURCES_PATH.find('@') == std::string::npos);
    CHECK(satdump::LIBRARIES_PATH.find('@') == std::string::npos);
}

TEST_CASE("satdump_vars getSatDumpVersionName includes version and optional tag", "[satdump_vars]")
{
    std::string expected = "SatDump v" + satdump::SATDUMP_VERSION;
    if (!satdump::SATDUMP_VERSION_TAG.empty())
        expected += " (" + satdump::SATDUMP_VERSION_TAG + ")";
    CHECK(satdump::getSatDumpVersionName() == expected);
}

#if !defined(__APPLE__) && !defined(_WIN32) && !defined(__ANDROID__)
// Bug: satdump_vars.cpp:77-78 initialise the globals RESPATH/LIBPATH from RESOURCES_PATH and
// LIBRARIES_PATH, which are defined in another translation unit (the generated satdump_varst.cpp).
// C++ does not order dynamic initialisation across translation units, and in this build they are
// read before being constructed, so RESPATH and LIBPATH end up empty (static initialisation order
// fiasco). Resources and system plugins are then only found relative to the working directory.
TEST_CASE("satdump_vars RESPATH and LIBPATH default to the configured install paths", "[satdump_vars][!shouldfail]")
{
    // On Linux, nothing is searched relative to the executable
    CHECK(satdump::RESPATH == satdump::RESOURCES_PATH);
    CHECK(satdump::LIBPATH == satdump::LIBRARIES_PATH);
}
#endif
