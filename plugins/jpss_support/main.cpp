#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "jpss/module_jpss_instruments.h"

class JPSSSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "spacex_support";
    }

    void init()
    {
        logger->info("JPSSSupport plugin loaded!");

        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, jpss::instruments::JPSSInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(JPSSSupport)