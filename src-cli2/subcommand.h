#pragma once

#include "CLI11.hpp"

namespace satdump
{
    class CmdHandler
    {
    public:
        const std::string cmd;

    public:
        CmdHandler(std::string cmd) : cmd(cmd) {}

        virtual void reg(CLI::App *app) = 0;
        virtual void run(CLI::App *app, CLI::App *subcom) = 0;
    };
} // namespace satdump