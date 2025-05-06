#include "probe.h"
#include "dsp/device/dev.h"
#include "logger.h"

namespace satdump
{
    void probeDevices(bool tx, bool rx, bool params)
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