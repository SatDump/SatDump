#pragma once

#include "core/cli/subcommand.h"
#include <memory>
#include <vector>

namespace satdump
{
    namespace cli
    {
        struct RegisterSubcommandEvent
        {
            std::vector<std::shared_ptr<satdump::CmdHandler>> &cmd_handlers;
            bool is_gui;
        };

        bool checkVerbose(int argc, char *argv[]);

        class CommandHandler
        {
        private:
            const bool is_gui;
            CLI::App app;

            std::vector<std::shared_ptr<satdump::CmdHandler>> cmd_handlers;

        public:
            CommandHandler(bool is_gui = false);
            int parse(int argc, char *argv[]);
            bool run();
        };
    } // namespace cli
} // namespace satdump