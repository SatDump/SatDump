#include "CLI11.hpp"
#include "init.h"
#include "logger.h"
#include "module.h"
#include "pipeline.h"
#include "probe.h"
#include "process.h"
#include "run_as.h"
#include "satdump_vars.h"
#include "subcommand.h"
#include <memory>

int main(int argc, char *argv[])
{
    // Init logger
    initLogger();

    // Basic flags
    bool verbose = false;
    for (int i = 0; i < argc; i++)
        if (std::string(argv[i]) == "-v" || std::string(argv[i]) == "--verbose")
            verbose = true;

    // Init SatDump, silent or verbose as requested
    if (!verbose)
        logger->set_level(slog::LOG_WARN);
    satdump::initSatdump();
    completeLoggerInit();
    if (!verbose)
        logger->set_level(slog::LOG_TRACE);

    // Command basics
    CLI::App app("SatDump v" + satdump::SATDUMP_VERSION);
    app.set_help_all_flag("--help-all", "Expand all help");
    app.add_flag("-v,--verbose", verbose, "Make the logger more verbose");
    app.require_subcommand();

    // All possible subcommands
    std::vector<std::shared_ptr<satdump::CmdHandler>> cmd_handlers;

    cmd_handlers.push_back(std::make_shared<satdump::ScriptCmdHandler>());
    cmd_handlers.push_back(std::make_shared<satdump::ModuleCmdHandler>());
    cmd_handlers.push_back(std::make_shared<satdump::ProbeCmdHandler>());
    cmd_handlers.push_back(std::make_shared<satdump::PipelineCmdHandler>());
    cmd_handlers.push_back(std::make_shared<satdump::ProcessCmdHandler>());

    for (auto &c : cmd_handlers)
        c->reg(&app);

    CLI11_PARSE(app, argc, argv);

    for (auto *subcom : app.get_subcommands())
    {
        //  std::cout << "Subcommand: " << subcom->get_name() << '\n';

        for (auto &c : cmd_handlers)
            if (subcom->get_name() == c->cmd)
                c->run(&app, subcom);
    }
}
