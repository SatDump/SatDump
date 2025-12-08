#pragma once

#include "core/cli/subcommand.h"

namespace satdump
{
    class BitViewCmdHandler : public CmdHandler
    {
    private:
        int period;

    public:
        BitViewCmdHandler() : CmdHandler("bitview") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom, bool is_gui);
    };
} // namespace satdump