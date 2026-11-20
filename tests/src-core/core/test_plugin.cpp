#include "core/plugin.h"
#include "helpers/guards.h"
#include "helpers/temp_files.h"
#include "satdump_vars.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>

// loadPlugins() is only pointed at a temporary ./plugins folder holding files that are not
// SatDump plugins, with LIBPATH redirected to an empty location so no installed plugins are
// touched. A private map is used instead of satdump::loaded_plugins.

namespace
{
    using test_helpers::CwdGuard;
    using test_helpers::TempDir;
    using test_helpers::writeFile;

    class DummyPlugin : public satdump::Plugin
    {
    public:
        std::string getID() { return "dummy_preexisting"; }
        void init() {}
    };
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
    test_helpers::ValueGuard lg(satdump::LIBPATH, (cwd.path / "no_system_libs").string() + "/");

    std::map<std::string, std::shared_ptr<satdump::Plugin>> plugins;
    CHECK_NOTHROW(loadPlugins(plugins));
    CHECK(plugins.empty());
}

TEST_CASE("loadPlugins skips non-plugin files and invalid libraries", "[core][plugin]")
{
    TempDir cwd("plugins");
    writeFile(cwd.path / "plugins" / "readme.txt", "not a library");
    writeFile(cwd.path / "plugins" / "broken.so", "not an ELF file either");
    writeFile(cwd.path / "plugins" / "nested" / "also_broken.so", "garbage");
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
    test_helpers::ValueGuard lg(satdump::LIBPATH, (cwd.path / "no_system_libs").string() + "/");

    std::map<std::string, std::shared_ptr<satdump::Plugin>> plugins;
    plugins["dummy_preexisting"] = std::make_shared<DummyPlugin>();
    CHECK_NOTHROW(loadPlugins(plugins));

    // Nothing was added and existing entries are kept
    REQUIRE(plugins.size() == 1);
    CHECK(plugins.count("dummy_preexisting") == 1);
}
