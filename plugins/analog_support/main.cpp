#include "core/plugin.h"
#include "logger.h"

#include "noaa_apt/module_noaa_apt_decoder.h"
#include "noaa_apt/module_noaa_apt_demod.h"
#include "noaa_apt/noaa_apt_proj.h"

#include "generic/module_generic_analog_demod.h"

#include "sstv/module_sstv_decoder.h"

class AnalogSupport : public satdump::Plugin
{
public:
    std::string getID() { return "analog_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::projection::RequestSatelliteRaytracerEvent>(provideSatProjHandler);
    }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa_apt::NOAAAPTDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa_apt::NOAAAPTDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, generic_analog::GenericAnalogDemodModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, sstv::SSTVDecoderModule);
    }

    static void provideSatProjHandler(const satdump::projection::RequestSatelliteRaytracerEvent &evt)
    {
        if (evt.id == "noaa_apt_single_line")
            evt.r.push_back(std::make_shared<noaa_apt::NOAA_APT_SatProj>(evt.cfg));
    }
};

PLUGIN_LOADER(AnalogSupport)