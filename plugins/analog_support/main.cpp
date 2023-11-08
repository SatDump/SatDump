#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "noaa_apt/module_noaa_apt_demod.h"
#include "noaa_apt/module_noaa_apt_decoder.h"
#include "noaa_apt/noaa_apt_proj.h"

class AnalogSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "analog_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::RequestSatProjEvent>(provideSatProjHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa_apt::NOAAAPTDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa_apt::NOAAAPTDecoderModule);
    }

    static void provideSatProjHandler(const satdump::RequestSatProjEvent &evt)
    {
        if (evt.id == "noaa_apt_single_line")
            evt.projs.push_back(std::make_shared<NOAA_APT_SatProj>(evt.cfg, evt.tle, evt.timestamps_raw));
    }
};

PLUGIN_LOADER(AnalogSupport)