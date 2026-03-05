#pragma once

#include "../subcommand.h"

namespace satdump
{
    class HServerCmdHandler : public CmdHandler
    {
    public:
        HServerCmdHandler() : CmdHandler("hserver_exp") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom, bool is_gui);
    };
} // namespace satdump