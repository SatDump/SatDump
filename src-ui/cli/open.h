#pragma once

#include "core/cli/subcommand.h"

namespace satdump
{
    class OpenCmdHandler : public CmdHandler
    {
    private:
        std::string file_to_open;

    public:
        OpenCmdHandler() : CmdHandler("open") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom, bool is_gui);
    };
} // namespace satdump