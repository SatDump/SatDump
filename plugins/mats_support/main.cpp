#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "mats/module_mats_instruments.h"

class MATSSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "mats_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, mats::instruments::MATSInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(MATSSupport)