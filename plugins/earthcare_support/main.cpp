#include "core/plugin.h"
#include "logger.h"

#include "earthcare/module_earthcare_instruments.h"

class EarthCARESupport : public satdump::Plugin
{
public:
    std::string getID() { return "earthcare_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, earthcare::instruments::EarthCAREInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(EarthCARESupport)