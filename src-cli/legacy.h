#pragma once

#include "core/cli/subcommand.h"

namespace satdump
{
    class LegacyCmdHandler : public CmdHandler
    {
    private:
        std::vector<std::string> args;

    public:
        LegacyCmdHandler() : CmdHandler("legacy") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom, bool is_gui);
    };
} // namespace satdump