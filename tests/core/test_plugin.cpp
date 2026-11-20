#include "core/plugin.h"
#include "satdump_vars.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <random>

// loadPlugins() is only pointed at a temporary ./plugins folder holding files that are not
// SatDump plugins, with LIBPATH redirected to an empty location so no installed plugins are
// touched. A private map is used instead of satdump::loaded_plugins.

namespace
{
    std::filesystem::path uniqueTempPath(const std::string &name)
    {
        std::mt19937 rng(std::random_device{}());
        return std::filesystem::temp_directory_path() / ("satdump_test_plugin_" + std::to_string(rng()) + "_" + name);
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

    struct LibPathGuard
    {
        std::string old = satdump::LIBPATH;
        LibPathGuard(const std::string &p) { satdump::LIBPATH = p; }
        ~LibPathGuard() { satdump::LIBPATH = old; }
    };

    class DummyPlugin : public satdump::Plugin
    {
    public:
        std::string getID() { return "dummy_preexisting"; }
        void init() {}
    };

    void writeText(const std::filesystem::path &p, const std::string &s)
    {
        std::filesystem::create_directories(p.parent_path());
        std::ofstream f(p);
        f << s;
    }
} // namespace

TEST_CASE("Plugin globals are created at startup", "[core][plugin]")
{
    CHECK(satdump::eventBus != nullptr);
    CHECK(satdump::taskScheduler != nullptr);
    CHECK(PLUGIN_ABI_VERSION == 1);
}

TEST_CASE("loadPlugins with no plugin directory loads nothing", "[core][plugin]")
{
    TempDir cwd("nodir");
    CwdGuard cg(cwd.path);
    LibPathGuard lg((cwd.path / "no_system_libs").string() + "/");

    std::map<std::string, std::shared_ptr<satdump::Plugin>> plugins;
    CHECK_NOTHROW(loadPlugins(plugins));
    CHECK(plugins.empty());
}

TEST_CASE("loadPlugins skips non-plugin files and invalid libraries", "[core][plugin]")
{
    TempDir cwd("plugins");
    writeText(cwd.path / "plugins" / "readme.txt", "not a library");
    writeText(cwd.path / "plugins" / "broken.so", "not an ELF file either");
    writeText(cwd.path / "plugins" / "nested" / "also_broken.so", "garbage");
    std::filesystem::create_directories(cwd.path / "plugins" / "dir.so"); // A directory with the extension

    // A real shared library that is not a SatDump plugin (no SATDUMP_ABI_VERSION symbol)
    for (const char *candidate : {"/usr/lib/x86_64-linux-gnu/libz.so.1", "/lib/x86_64-linux-gnu/libz.so.1", "/usr/lib/libz.so.1", "/usr/lib64/libz.so.1"})
    {
        if (std::filesystem::exists(candidate))
        {
            std::filesystem::copy_file(std::filesystem::canonical(candidate), cwd.path / "plugins" / "notaplugin.so");
            break;
        }
    }

    CwdGuard cg(cwd.path);
    LibPathGuard lg((cwd.path / "no_system_libs").string() + "/");

    std::map<std::string, std::shared_ptr<satdump::Plugin>> plugins;
    plugins["dummy_preexisting"] = std::make_shared<DummyPlugin>();
    CHECK_NOTHROW(loadPlugins(plugins));

    // Nothing was added and existing entries are kept
    REQUIRE(plugins.size() == 1);
    CHECK(plugins.count("dummy_preexisting") == 1);
}
