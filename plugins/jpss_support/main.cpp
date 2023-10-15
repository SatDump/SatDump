#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "jpss/module_jpss_instruments.h"

#include "jpss/instruments/viirs/viirs_proj.h"

class JPSSSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "jpss_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::RequestSatProjEvent>(provideSatProjHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, jpss::instruments::JPSSInstrumentsDecoderModule);
    }

    static void provideSatProjHandler(const satdump::RequestSatProjEvent &evt)
    {
        if (evt.id == "viirs_single_line")
            evt.projs.push_back(std::make_shared<VIIRSNormalLineSatProj>(evt.cfg, evt.tle, evt.timestamps_raw));
    }
};

PLUGIN_LOADER(JPSSSupport)