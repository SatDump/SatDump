#include "core/plugin.h"
#include "dsp/flowgraph/dsp_flowgraph_register.h"
#include "dsp/flowgraph/flowgraph.h"
#include "litexm2sdr_dev.h"
#include "logger.h"

class LiteXM2SDRSDRSupport : public satdump::Plugin
{
public:
    std::string getID() { return "litexm2sdr_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceListEvent>(registerDevs);
        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceInstanceEvent>(provideDeviceInstance);

        satdump::eventBus->register_handler<satdump::ndsp::flowgraph::RegisterNodesEvent>(registerNodes);
    }

    static void registerDevs(const satdump::ndsp::RequestDeviceListEvent &evt)
    {
        if (evt.m != satdump::ndsp::DeviceBlock::MODE_SINGLE_RX && //
            evt.m != satdump::ndsp::DeviceBlock::MODE_SINGLE_TX && //
            evt.m != satdump::ndsp::DeviceBlock::MODE_RX_TX &&     //
            evt.m != satdump::ndsp::DeviceBlock::MODE_NORMAL)
            return;

        auto d = satdump::ndsp::LiteXM2SDRDevBlock::listDevs();
        evt.i.insert(evt.i.end(), d.begin(), d.end());
    }

    static void provideDeviceInstance(const satdump::ndsp::RequestDeviceInstanceEvent &evt)
    {
        if (evt.info.type == "litexm2sdr")
            evt.i.push_back(std::make_shared<satdump::ndsp::LiteXM2SDRDevBlock>());
    }

    static void registerNodes(const satdump::ndsp::flowgraph::RegisterNodesEvent &evt)
    {
        evt.r.insert({"litexm2sdr_cc",
                      {"Device/LiteXM2SDR Dev", [](const satdump::ndsp::flowgraph::Flowgraph *f)
                       { return std::make_shared<satdump::ndsp::flowgraph::NodeInternal>(f, std::make_shared<satdump::ndsp::LiteXM2SDRDevBlock>()); }}});
    }
};

PLUGIN_LOADER(LiteXM2SDRSDRSupport)