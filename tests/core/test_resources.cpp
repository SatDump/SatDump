#include "core/resources.h"
#include "satdump_vars.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <random>

// Resource lookup depends on the current directory and satdump::RESPATH. Both are changed only
// through RAII guards that restore them.

namespace
{
    std::filesystem::path uniqueTempPath(const std::string &name)
    {
        std::mt19937 rng(std::random_device{}());
        return std::filesystem::temp_directory_path() / ("satdump_test_resources_" + std::to_string(rng()) + "_" + name);
    }

    struct TempDir
    {
        std::filesystem::path path;
        TempDir(const std::string &name) : path(uniqueTempPath(name)) { std::filesystem::create_directories(path); }
        ~TempDir()
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    };

    struct CwdGuard
    {
        std::filesystem::path old = std::filesystem::current_path();
        CwdGuard(const std::filesystem::path &p) { std::filesystem::current_path(p); }
        ~CwdGuard() { std::filesystem::current_path(old); }
    };

    struct ResPathGuard
    {
        std::string old = satdump::RESPATH;
        ResPathGuard(const std::string &p) { satdump::RESPATH = p; }
        ~ResPathGuard() { satdump::RESPATH = old; }
    };

    void touch(const std::filesystem::path &p)
    {
        std::filesystem::create_directories(p.parent_path());
        std::ofstream f(p);
        f << "x";
    }
} // namespace

TEST_CASE("resources prefer a resources folder in the current directory", "[core][resources]")
{
    TempDir cwd("cwd");
    touch(cwd.path / "resources" / "maps" / "local.json");
    TempDir respath("respath");
    touch(respath.path / "resources" / "maps" / "system.json");

    CwdGuard cg(cwd.path);
    ResPathGuard rg(respath.path.string() + "/");

    CHECK(resources::resourceExists("maps/local.json"));
    CHECK(resources::resourceExists("maps"));
    // The system location is not consulted at all while ./resources exists
    CHECK_FALSE(resources::resourceExists("maps/system.json"));
    CHECK(resources::getResourcePath("maps/local.json") == "resources/maps/local.json");
    // Missing resources still get a path (and an error is logged)
    CHECK(resources::getResourcePath("maps/system.json") == "resources/maps/system.json");
}

TEST_CASE("resources fall back to RESPATH without a local resources folder", "[core][resources]")
{
    TempDir cwd("empty_cwd");
    TempDir respath("respath");
    touch(respath.path / "resources" / "fonts" / "font.ttf");

    CwdGuard cg(cwd.path);
    ResPathGuard rg(respath.path.string() + "/");

    CHECK(resources::resourceExists("fonts/font.ttf"));
    CHECK_FALSE(resources::resourceExists("fonts/missing.ttf"));
    CHECK(resources::getResourcePath("fonts/font.ttf") == respath.path.string() + "/resources/fonts/font.ttf");
    CHECK(resources::getResourcePath("fonts/missing.ttf") == respath.path.string() + "/resources/fonts/missing.ttf");
}

TEST_CASE("resources resolve the repository's shipped resources", "[core][resources]")
{
    TempDir cwd("empty_cwd2");
    CwdGuard cg(cwd.path);
    ResPathGuard rg(std::string(SATDUMP_TEST_SOURCE_DIR) + "/");

    CHECK(resources::resourceExists("themes"));
    CHECK(std::filesystem::is_directory(resources::getResourcePath("themes")));
}
