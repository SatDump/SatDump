#pragma once

#include "subcommand.h"

namespace satdump
{
    class ScriptCmdHandler : public CmdHandler
    {
    public:
        ScriptCmdHandler() : CmdHandler("run") {}

        void reg(CLI::App *app);
        void run(CLI::App *app, CLI::App *subcom);

        int runAngelScript(std::string file, bool lint, bool predef);
    };
} // namespace satdump