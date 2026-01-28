#include "open.h"
#include "main_ui.h"

namespace satdump
{
    void OpenCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_module = app->add_subcommand("open", "Try to load (any) file in the explorer");
        sub_module->add_option("file_to_open");
    }

    void OpenCmdHandler::run(CLI::App *, CLI::App *subcom, bool)
    {
        std::string file = subcom->get_option("file_to_open")->as<std::string>();

        explorer_app->tryOpenFileInExplorer(file);
    }
} // namespace satdump