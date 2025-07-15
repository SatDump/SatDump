#pragma once

#include "subcommand.h"

namespace satdump
{
    class ModuleCmdHandler : public CmdHandler
    {
    private:
        std::map<std::string, std::map<std::string, std::shared_ptr<std::string>>> modules_opts;

    public:
        ModuleCmdHandler() : CmdHandler("module") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom);
    };
} // namespace satdump