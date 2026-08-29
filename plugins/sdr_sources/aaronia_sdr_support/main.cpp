#include "aaronia_sdr.h"
#include "core/config.h"
#include "core/plugin.h"
#include "dsp/device/dev.h"
#include "dsp/flowgraph/dsp_flowgraph_register.h"
#include "dynload.h"
#include "spectran_v6_dev.h"
#include <exception>

class AaroniaSDRSupport : public satdump::Plugin
{
private:
    static void renderConfig() { ImGui::InputText("RTSA API Path", &rtsa_api_path); }

    static void save() { satdump::satdump_cfg.main_cfg["plugin_settings"]["aaronia_support"] = rtsa_api_path; }

public:
    std::string getID() { return "aaronia_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::config::RegisterPluginConfigHandlersEvent>(registerConfigHandler);

        satdump::eventBus->register_handler<satdump::ndsp::flowgraph::RegisterNodesEvent>(registerNodes);

        if (!satdump::satdump_cfg.main_cfg["plugin_settings"]["aaronia_support"].is_null())
            rtsa_api_path = satdump::satdump_cfg.main_cfg["plugin_settings"]["aaronia_support"];

        if (!std::filesystem::exists(rtsa_api_path))
        {
            logger->info("Couldn't find RTSA API at  '" + rtsa_api_path + "', not loading!");
            return;
        }

        try
        {
            rtsa_api = new RTSAApiInstance(rtsa_api_path);

            if (rtsa_api->isOpen())
            {
                satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);

                satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceListEvent>(registerDevs);
                satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceInstanceEvent>(provideDeviceInstance);
            }
        }
        catch (std::exception &e)
        {
            logger->error("Failed to load RTSA API! %s", e.what());
        }
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({AaroniaSource::getID(), {AaroniaSource::getInstance, AaroniaSource::getAvailableSources}});
    }

    static void registerConfigHandler(const satdump::config::RegisterPluginConfigHandlersEvent &evt)
    {
        evt.plugin_config_handlers.push_back({"Aaronia Support", AaroniaSDRSupport::renderConfig, AaroniaSDRSupport::save});
    }

    static void registerDevs(const satdump::ndsp::RequestDeviceListEvent &evt)
    {
        if (evt.m != satdump::ndsp::DeviceBlock::MODE_SINGLE_RX && evt.m != satdump::ndsp::DeviceBlock::MODE_NORMAL)
            return;

        auto d = satdump::ndsp::SpectranV6DevBlock::listDevs();
        evt.i.insert(evt.i.end(), d.begin(), d.end());
    }

    static void provideDeviceInstance(const satdump::ndsp::RequestDeviceInstanceEvent &evt)
    {
        if (evt.info.type == "spectran_v6")
            evt.i.push_back(std::make_shared<satdump::ndsp::SpectranV6DevBlock>());
    }

    static void registerNodes(const satdump::ndsp::flowgraph::RegisterNodesEvent &evt)
    {
        evt.r.insert({"spectran_v6_cc",
                      {"Device/Spectran V6 Dev", [](const satdump::ndsp::flowgraph::Flowgraph *f)
                       { return std::make_shared<satdump::ndsp::flowgraph::NodeInternal>(f, std::make_shared<satdump::ndsp::SpectranV6DevBlock>()); }}});
    }

    // ~AaroniaSDRSupport() { delete rtsa_api; }
};

#ifdef RTSA_LIB_PATH
std::string rtsa_api_path = RTSA_LIB_PATH;
#else
std::string rtsa_api_path = "/opt/aaronia-rtsa-suite/Aaronia-RTSA-Suite-PRO/libAaroniaRTSAAPI.so";
#endif

RTSAApiInstance *rtsa_api = nullptr;

PLUGIN_LOADER(AaroniaSDRSupport)