#pragma once

#include "core/cli/subcommand.h"

namespace satdump
{
    class GGAKMergerCmdHandler : public CmdHandler
    {
    private:
        std::string mqtt_server;
        std::string mqtt_port;
        std::string satellite;
        std::string dump_file;

    public:
        GGAKMergerCmdHandler() : CmdHandler("ggak_merger") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom, bool is_gui);
    };
} // namespace satdump