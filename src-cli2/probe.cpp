#include "probe.h"
#include "dsp/device/dev.h"
#include "logger.h"

namespace satdump
{
    void ProbeCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_probe = app->add_subcommand("probe", "Probe for SDR Devices");
        auto sub_probe_group = sub_probe->add_option_group("--tx,--rx");
        sub_probe_group->add_flag("--tx", "Only list devices with at least 1 TX Channel");
        sub_probe_group->add_flag("--rx", "Only list devices with at least 1 RX Channel");
        sub_probe_group->require_option(0, 1);
        sub_probe->add_flag("--params", "Dump full device parameters");
    }

    void ProbeCmdHandler::run(CLI::App *app, CLI::App *subcom)
    {
        bool tx = subcom->count("--tx");
        bool rx = subcom->count("--rx");
        bool pa = subcom->count("--params");
        probeDevices(tx, rx, pa);
    }

    void ProbeCmdHandler::probeDevices(bool tx, bool rx, bool params)
    {
        ndsp::DeviceBlock::device_mode_t m = ndsp::DeviceBlock::MODE_NORMAL;
        if (tx)
            m = satdump::ndsp::DeviceBlock::MODE_SINGLE_TX;
        if (rx)
            m = satdump::ndsp::DeviceBlock::MODE_SINGLE_RX;
        auto devs = satdump::ndsp::getDeviceList(m);

        if (params)
        {
            for (auto &d : devs)
            {
                auto i = satdump::ndsp::getDeviceInstanceFromInfo(d, satdump::ndsp::DeviceBlock::MODE_SINGLE_TX);
                d.params = i->get_cfg_list();
                logger->debug("\n" + nlohmann::json(d).dump(4) + "\n");
            }
        }
    }
} // namespace satdump