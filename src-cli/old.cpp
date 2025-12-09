#include "old.h"
#include "logger.h"

int main_old(int argc, char *argv[]);

namespace satdump
{
    void OldCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub = app->add_subcommand("old", "Run command ");
        sub->add_option("args", args, "Old-style arguments [Will be removed sooner or later]");
    }

    void OldCmdHandler::run(CLI::App *app, CLI::App *subcom, bool is_gui)
    {
        std::vector<char *> vec;
        for (auto &a : args)
            vec.push_back((char *)a.c_str());

        exit(main_old(vec.size(), vec.data()));
    }
} // namespace satdump