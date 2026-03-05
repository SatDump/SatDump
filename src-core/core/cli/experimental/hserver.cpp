#include "hserver.h"
#include "dsp/device/dev.h"
#include "logger.h"

namespace satdump
{
    void HServerCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_probe = app->add_subcommand("probe", "Probe for SDR Devices");
        // auto sub_probe_group = sub_probe->add_option_group("--tx,--rx");
        // sub_probe_group->add_flag("--tx", "Only list devices with at least 1 TX Channel");
        // sub_probe_group->add_flag("--rx", "Only list devices with at least 1 RX Channel");
        // sub_probe_group->require_option(0, 1);
        // sub_probe->add_flag("--params", "Dump full device parameters");
    }

    void HServerCmdHandler::run(CLI::App *app, CLI::App *subcom, bool is_gui)
    {
        // bool tx = subcom->count("--tx");
        // bool rx = subcom->count("--rx");
        // bool pa = subcom->count("--params");
    }
} // namespace satdump