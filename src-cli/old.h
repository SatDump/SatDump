#pragma once

#include "core/cli/subcommand.h"

namespace satdump
{
    class OldCmdHandler : public CmdHandler
    {
    private:
        std::vector<std::string> args;

    public:
        OldCmdHandler() : CmdHandler("old") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom, bool is_gui);
    };
} // namespace satdump