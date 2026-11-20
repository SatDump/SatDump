#include "cli_test_helpers.h"
#include "core/cli/cli.h"
#include "core/plugin.h"
#include "legacy.h"
#include <catch2/catch_test_macros.hpp>

// LegacyCmdHandler::run() ends with exit(), so only the registration of the "legacy"
// subcommand and the way it collects its arguments are tested.

using namespace cli_test;

namespace
{
    // Parses "satdump <args>" with an app holding only the legacy subcommand
    struct LegacyApp
    {
        CLI::App app{"test"};
        satdump::LegacyCmdHandler handler;

        LegacyApp() { handler.reg(&app); }

        void parse(std::vector<std::string> args)
        {
            args.insert(args.begin(), "satdump");
            Argv a(args);
            app.parse(a.argc(), a.argv());
        }

        CLI::App *sub() { return app.get_subcommand("legacy"); }
    };
} // namespace

TEST_CASE("LegacyCmdHandler registers the legacy subcommand", "[cli][legacy]")
{
    LegacyApp l;

    CHECK(l.handler.cmd == "legacy");
    REQUIRE(l.app.get_subcommands([](CLI::App *) { return true; }).size() == 1);
    CHECK(l.sub()->get_name() == "legacy");
    CHECK_FALSE(l.sub()->get_description().empty());
    CHECK_FALSE(l.sub()->parsed());
}

TEST_CASE("LegacyCmdHandler passes everything after legacy through untouched", "[cli][legacy]")
{
    LegacyApp l;

    SECTION("positionals and flags stay in order")
    {
        l.parse({"legacy", "metop_ahrpt", "baseband", "in.cs16", "out", "--samplerate", "6e6", "--dc_block"});
        CHECK(l.sub()->parsed());
        CHECK(l.sub()->remaining() == std::vector<std::string>{"metop_ahrpt", "baseband", "in.cs16", "out", "--samplerate", "6e6", "--dc_block"});
    }

    SECTION("no arguments at all")
    {
        l.parse({"legacy"});
        CHECK(l.sub()->parsed());
        CHECK(l.sub()->remaining().empty());
    }

    SECTION("arguments starting with a flag")
    {
        l.parse({"legacy", "--v"});
        CHECK(l.sub()->remaining() == std::vector<std::string>{"--v"});
    }

    SECTION("help flags are left to the legacy command")
    {
        // The subcommand has no help flag of its own, so -h is handed over like any other
        // argument instead of raising CLI::CallForHelp
        CHECK_NOTHROW(l.parse({"legacy", "-h"}));
        CHECK(l.sub()->remaining() == std::vector<std::string>{"-h"});
        CHECK(l.sub()->get_help_ptr() == nullptr);
    }

    SECTION("a name that is also a top-level word is not interpreted")
    {
        l.parse({"legacy", "live", "legacy", "--help"});
        CHECK(l.sub()->remaining() == std::vector<std::string>{"live", "legacy", "--help"});
    }
}

TEST_CASE("LegacyCmdHandler plugs into the CommandHandler like the satdump executable does", "[cli][legacy]")
{
    // Same registration as src-cli/main.cpp. reg() of the built-in commands reads (and with
    // operator[] may add to) the global config.
    EventBusGuard bus;
    ConfigGuard cfg;
    satdump::eventBus->register_handler<satdump::cli::RegisterSubcommandEvent>([](const satdump::cli::RegisterSubcommandEvent &evt)
                                                                               { evt.cmd_handlers.push_back(std::make_shared<satdump::LegacyCmdHandler>()); });

    satdump::cli::CommandHandler h;

    SECTION("legacy command lines are accepted")
    {
        Argv a({"satdump", "legacy", "record", "out", "--source", "file", "--samplerate", "1e6"});
        CHECK(h.parse(a.argc(), a.argv()) == 0);
        CHECK(h.has_command());
    }

    SECTION("legacy help is a plain argument")
    {
        Argv a({"satdump", "legacy", "help"});
        CHECK(h.parse(a.argc(), a.argv()) == 0);
        CHECK(h.has_command());
    }

    SECTION("legacy -h parses, but the command is then not run")
    {
        // CommandHandler::run() does nothing once -h / --help is anywhere on the command line,
        // so main_old() never gets to print the legacy help for "satdump legacy -h" (only
        // "satdump legacy help" does). If it did run, this would exit the test executable.
        Argv a({"satdump", "legacy", "-h"});
        CHECK(h.parse(a.argc(), a.argv()) == 0);
        CHECK(h.has_command());
        CHECK_FALSE(h.run());
    }
}
