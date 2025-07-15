#pragma once

#include "subcommand.h"

namespace satdump
{
    class ProbeCmdHandler : public CmdHandler
    {
    public:
        ProbeCmdHandler() : CmdHandler("probe") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom);

        void probeDevices(bool tx, bool rx, bool params);
    };
} // namespace satdump