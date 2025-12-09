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
        };

        bool checkVerbose(int argc, char *argv[]);

        int handleCommand(int argc, char *argv[], bool is_gui = false);
    } // namespace cli
} // namespace satdump