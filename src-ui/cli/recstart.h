#pragma once

#include "core/cli/subcommand.h"
#include <cstdint>

namespace satdump
{
    class RecStartCmdHandler : public CmdHandler
    {
    private:
        uint64_t samplerate = 0;
        bool start_recorder_device = false;
        bool engage_autotrack = false;

    public:
        RecStartCmdHandler() : CmdHandler("recstart") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom, bool is_gui);
    };
} // namespace satdump