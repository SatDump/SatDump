#pragma once

#include "core/cli/subcommand.h"

namespace satdump
{
    class DiscoverProductsCmdHandler : public CmdHandler
    {
    private:
        int period;

    public:
        DiscoverProductsCmdHandler() : CmdHandler("prodisc") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom, bool is_gui);
    };
} // namespace satdump