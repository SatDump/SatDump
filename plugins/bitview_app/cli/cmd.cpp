#include "cmd.h"
#include "bit_container.h"
#include "bitview.h"
#include "core/plugin.h"
#include "explorer/explorer.h"

namespace satdump
{
    void BitViewCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_module = app->add_subcommand("bitview", "Open a file in a BitView");
        sub_module->add_option("file");
        sub_module->add_flag("--period", period, "Bit period to use")->default_val(256);
    }

    void BitViewCmdHandler::run(CLI::App *app, CLI::App *subcom, bool is_gui)
    {
        std::string file = subcom->get_option("file")->as<std::string>();

        auto container = std::make_shared<satdump::BitContainer>(                                           //
            std::filesystem::path(file).stem().string() + std::filesystem::path(file).extension().string(), //
            file);

        // Set params
        container->d_bitperiod = period;

        // Init
        container->init_bitperiod();
        eventBus->fire_event<explorer::ExplorerAddHandlerEvent>({std::make_shared<satdump::BitViewHandler>(container), true});
    }
} // namespace satdump