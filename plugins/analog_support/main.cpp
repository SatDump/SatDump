#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "noaa_apt/module_noaa_apt_demod.h"
#include "noaa_apt/module_noaa_apt_decoder.h"

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
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa_apt::NOAAAPTDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa_apt::NOAAAPTDecoderModule);
    }
};

PLUGIN_LOADER(AnalogSupport)