#include "core/cli/cli.h"
#include "core/config.h"
#include "core/plugin.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Only argument parsing is exercised. CommandHandler::run() would actually probe SDRs, run
// pipelines, etc., so it is only called with --help (which runs nothing) or for a test-only
// subcommand injected through RegisterSubcommandEvent.

namespace
{
    // Builds a mutable argv from strings
    struct Argv
    {
        std::vector<std::string> storage;
        std::vector<char *> ptrs;
        Argv(std::vector<std::string> args) : storage(std::move(args))
        {
            for (auto &s : storage)
                ptrs.push_back(s.data());
            ptrs.push_back(nullptr);
        }
        int argc() { return (int)storage.size(); }
        char **argv() { return ptrs.data(); }
    };

    // PipelineCmdHandler::reg() reads (and with operator[] may add to) the global config when
    // pipelines are registered, so keep it unchanged for other tests
    struct ConfigGuard
    {
        nlohmann::ordered_json saved = satdump::satdump_cfg.main_cfg;
        ~ConfigGuard() { satdump::satdump_cfg.main_cfg = saved; }
    };

    int parse(satdump::cli::CommandHandler &h, std::vector<std::string> args)
    {
        args.insert(args.begin(), "satdump");
        Argv a(args);
        return h.parse(a.argc(), a.argv());
    }

    // A subcommand added through RegisterSubcommandEvent. EventBus handlers cannot be removed, so
    // the handler registered below only injects this command while a test has `enabled` set.
    struct TestCmdState
    {
        bool enabled = false;
        int runs = 0;
        std::string arg;
        bool flag = false;
        bool throw_on_run = false;
    };

    std::shared_ptr<TestCmdState> testCmdState()
    {
        static std::shared_ptr<TestCmdState> state = []()
        {
            auto st = std::make_shared<TestCmdState>();
            satdump::eventBus->register_handler<satdump::cli::RegisterSubcommandEvent>(
                [st](satdump::cli::RegisterSubcommandEvent evt)
                {
                    struct Cmd : public satdump::CmdHandler
                    {
                        std::shared_ptr<TestCmdState> st;
                        Cmd(std::shared_ptr<TestCmdState> s) : satdump::CmdHandler("unit_test_cmd"), st(s) {}
                        void reg(CLI::App *app)
                        {
                            auto sub = app->add_subcommand("unit_test_cmd", "Test only");
                            sub->add_option("arg")->required();
                            sub->add_flag("--flag");
                        }
                        void run(CLI::App *, CLI::App *subcom, bool)
                        {
                            st->runs++;
                            st->arg = subcom->get_option("arg")->as<std::string>();
                            st->flag = subcom->count("--flag") > 0;
                            if (st->throw_on_run)
                                throw std::runtime_error("unit test command failure");
                        }
                    };
                    if (st->enabled)
                        evt.cmd_handlers.push_back(std::make_shared<Cmd>(st));
                });
            return st;
        }();
        return state;
    }

    struct TestCmdEnabled
    {
        std::shared_ptr<TestCmdState> st = testCmdState();
        TestCmdEnabled() { *st = TestCmdState(), st->enabled = true; }
        ~TestCmdEnabled() { st->enabled = false; }
    };
} // namespace

TEST_CASE("cli::checkVerbose detects -v and --verbose", "[core][cli]")
{
    Argv none({"satdump", "probe"});
    CHECK_FALSE(satdump::cli::checkVerbose(none.argc(), none.argv()));
    Argv shortv({"satdump", "-v", "probe"});
    CHECK(satdump::cli::checkVerbose(shortv.argc(), shortv.argv()));
    Argv longv({"satdump", "probe", "--verbose"});
    CHECK(satdump::cli::checkVerbose(longv.argc(), longv.argv()));
    // Only exact matches count
    Argv similar({"satdump", "-vv", "--verbosity"});
    CHECK_FALSE(satdump::cli::checkVerbose(similar.argc(), similar.argv()));
    CHECK_FALSE(satdump::cli::checkVerbose(0, nullptr));
}

TEST_CASE("cli::checkHelp detects -h, --help and --help-all", "[core][cli]")
{
    for (std::string flag : {"-h", "--help", "--help-all"})
    {
        Argv a({"satdump", "pipeline", flag});
        CHECK(satdump::cli::checkHelp(a.argc(), a.argv()));
    }
    Argv none({"satdump", "probe", "--helpful"});
    CHECK_FALSE(satdump::cli::checkHelp(none.argc(), none.argv()));
}

TEST_CASE("CommandHandler requires a known subcommand", "[core][cli]")
{
    ConfigGuard cg;
    satdump::cli::CommandHandler h(false);
    CHECK(parse(h, {}) != 0);
    CHECK_FALSE(h.has_command());

    satdump::cli::CommandHandler h2(false);
    CHECK(parse(h2, {"definitely_not_a_command"}) != 0);
}

TEST_CASE("CommandHandler parses the CLI-only subcommands", "[core][cli]")
{
    ConfigGuard cg;
    {
        satdump::cli::CommandHandler h(false);
        CHECK(parse(h, {"probe", "--tx"}) == 0);
        CHECK(h.has_command());
    }
    {
        satdump::cli::CommandHandler h(false);
        CHECK(parse(h, {"-v", "probe", "--rx", "--params"}) == 0);
    }
    {
        satdump::cli::CommandHandler h(false);
        CHECK(parse(h, {"run", "script.as", "--lint"}) == 0);
    }
    {
        satdump::cli::CommandHandler h(false);
        CHECK(parse(h, {"process", "product.cbor", "out_dir"}) == 0);
    }
    {
        satdump::cli::CommandHandler h(false);
        CHECK(parse(h, {"dsp_bench", "--agc", "--rrc"}) == 0);
    }
}

TEST_CASE("CommandHandler rejects invalid subcommand arguments", "[core][cli]")
{
    ConfigGuard cg;
    {
        // --tx and --rx are mutually exclusive
        satdump::cli::CommandHandler h(false);
        CHECK(parse(h, {"probe", "--tx", "--rx"}) != 0);
    }
    {
        // process needs both a product and a directory
        satdump::cli::CommandHandler h(false);
        CHECK(parse(h, {"process", "product.cbor"}) != 0);
    }
    {
        // run needs a script
        satdump::cli::CommandHandler h(false);
        CHECK(parse(h, {"run"}) != 0);
    }
    {
        satdump::cli::CommandHandler h(false);
        CHECK(parse(h, {"dsp_bench", "--not_a_benchmark"}) != 0);
    }
}

TEST_CASE("CommandHandler in GUI mode only offers the shared subcommands", "[core][cli]")
{
    ConfigGuard cg;
    {
        satdump::cli::CommandHandler h(true);
        CHECK(parse(h, {"dsp_bench"}) == 0);
    }
    for (std::string cmd : {"probe", "module", "process"})
    {
        satdump::cli::CommandHandler h(true);
        CHECK(parse(h, {cmd}) != 0);
    }
}

TEST_CASE("CommandHandler help succeeds and run does nothing", "[core][cli]")
{
    ConfigGuard cg;
    satdump::cli::CommandHandler h(false);
    // CLI11 reports help as a successful exit
    CHECK(parse(h, {"probe", "--help"}) == 0);
    CHECK_FALSE(h.run());
}

TEST_CASE("CommandHandler runs subcommands registered through RegisterSubcommandEvent", "[core][cli]")
{
    ConfigGuard cg;
    TestCmdEnabled tc;

    satdump::cli::CommandHandler h(false);
    REQUIRE(parse(h, {"unit_test_cmd", "the_arg", "--flag"}) == 0);
    CHECK(h.has_command());
    CHECK_FALSE(h.run());
    CHECK(tc.st->runs == 1);
    CHECK(tc.st->arg == "the_arg");
    CHECK(tc.st->flag);

    SECTION("an exception from the command makes run report an error")
    {
        tc.st->throw_on_run = true;
        satdump::cli::CommandHandler h2(true);
        REQUIRE(parse(h2, {"unit_test_cmd", "x"}) == 0);
        CHECK(h2.run());
        CHECK(tc.st->arg == "x");
        CHECK_FALSE(tc.st->flag);
    }
}

TEST_CASE("CommandHandler test subcommand is absent unless enabled", "[core][cli]")
{
    ConfigGuard cg;
    testCmdState(); // Make sure the handler exists, but leave it disabled
    satdump::cli::CommandHandler h(false);
    CHECK(parse(h, {"unit_test_cmd", "x"}) != 0);
}
