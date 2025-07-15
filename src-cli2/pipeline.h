#pragma once

#include "subcommand.h"

namespace satdump
{
    class PipelineCmdHandler : public CmdHandler
    {
    private:
        std::map<std::string, std::map<std::string, std::shared_ptr<std::string>>> pipeline_opts;

    public:
        PipelineCmdHandler() : CmdHandler("pipeline") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom);
    };
} // namespace satdump