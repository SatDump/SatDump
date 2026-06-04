#pragma once

#include "../subcommand.h"

namespace satdump
{
    class DspBenchCmdHandler : public CmdHandler
    {
    private:
        std::vector<std::string> cats_to_use;

    public:
        DspBenchCmdHandler() : CmdHandler("dsp_bench") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom, bool is_gui);
    };
} // namespace satdump