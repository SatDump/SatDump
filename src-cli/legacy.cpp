#include "legacy.h"
#include "logger.h"

int main_old(int argc, char *argv[]);

namespace satdump
{
    void LegacyCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub = app->add_subcommand("legacy", "Run legacy command from pre-2.0.0-style satdump CLI");
        sub->add_option("args", args, "Legacy-style arguments [Will be removed sooner or later]");
    }

    void LegacyCmdHandler::run(CLI::App *app, CLI::App *subcom, bool is_gui)
    {
        std::vector<char *> vec;
        for (auto &a : args)
            vec.push_back((char *)a.c_str());

        exit(main_old(vec.size(), vec.data()));
    }
} // namespace satdump