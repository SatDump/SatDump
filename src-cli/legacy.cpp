#include "legacy.h"
#include "logger.h"

int main_old(int argc, char *argv[]);

namespace satdump
{
    void LegacyCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub = app->add_subcommand("legacy", "Run legacy command from pre-2.0.0-style satdump CLI");
        sub->prefix_command();
        sub->set_help_flag();
    }

    void LegacyCmdHandler::run(CLI::App *app, CLI::App *subcom, bool is_gui)
    {
        auto args = subcom->remaining();

        std::vector<char *> vec;
        args.insert(args.begin(), "satdump legacy");
        for (auto &a : args)
        {
            logger->critical(a);
            vec.push_back((char *)a.c_str());
        }

        exit(main_old(vec.size(), vec.data()));
    }
} // namespace satdump