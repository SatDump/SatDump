#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "hinode/module_hinode_instruments.h"

class HinodeSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "hinode_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, hinode::instruments::HinodeInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(HinodeSupport)