#include "cli.h"
#include "core/plugin.h"
#include "libs/CLI11.hpp"
#include "logger.h"
#include "module.h"
#include "pipeline.h"
#include "probe.h"
#include "process.h"
#include "run_as.h"
#include "satdump_vars.h"
#include "subcommand.h"
#include <exception>
#include <string>

namespace satdump
{
    namespace cli
    {
        bool checkVerbose(int argc, char *argv[])
        {
            bool verbose = false;
            for (int i = 0; i < argc; i++)
                if (std::string(argv[i]) == "-v" || std::string(argv[i]) == "--verbose")
                    verbose = true;
            return verbose;
        }

        CommandHandler::CommandHandler(bool is_gui) : is_gui(is_gui), app("SatDump v" + satdump::SATDUMP_VERSION)
        {
            // Command basics
            app.set_help_all_flag("--help-all", "Expand all help");
            bool verbose = false;
            app.add_flag("-v,--verbose", verbose, "Make the logger more verbose");
            app.require_subcommand();

            // Register all possible subcommands

            // Everything
            cmd_handlers.push_back(std::make_shared<satdump::PipelineCmdHandler>());

            if (is_gui) // GUI Only
            {
            }
            else // Non-GUI only
            {
                cmd_handlers.push_back(std::make_shared<satdump::ScriptCmdHandler>());
                cmd_handlers.push_back(std::make_shared<satdump::ModuleCmdHandler>());
                cmd_handlers.push_back(std::make_shared<satdump::ProbeCmdHandler>());
                cmd_handlers.push_back(std::make_shared<satdump::ProcessCmdHandler>());
            }

            eventBus->fire_event<RegisterSubcommandEvent>({cmd_handlers, is_gui});

            for (auto &c : cmd_handlers)
                c->reg(&app);
        }

        int CommandHandler::parse(int argc, char *argv[])
        {
            CLI11_PARSE(app, argc, argv);
            return 0;
        }

        bool CommandHandler::run()
        {
            for (auto *subcom : app.get_subcommands())
            {
                //  std::cout << "Subcommand: " << subcom->get_name() << '\n';

                try
                {
                    for (auto &c : cmd_handlers)
                        if (subcom->get_name() == c->cmd)
                            c->run(&app, subcom, is_gui);
                }
                catch (std::exception &e)
                {
                    logger->error("Error running command : %s", e.what());
                    return true;
                }
            }

            return false;
        }
    } // namespace cli
} // namespace satdump