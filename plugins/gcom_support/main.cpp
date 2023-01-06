#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "gcom1/module_gcom1_instruments.h"

class GCOMSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "gcom_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, gcom1::instruments::GCOM1InstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(GCOMSupport)