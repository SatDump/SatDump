#pragma once

#include "subcommand.h"

namespace satdump
{
    class ProcessCmdHandler : public CmdHandler
    {
    public:
        ProcessCmdHandler() : CmdHandler("process") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom);

        void processProductsOrDataset(std::string product, std::string directory);
    };
} // namespace satdump