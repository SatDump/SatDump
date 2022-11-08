#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "aim/module_aim_instruments.h"

class AIMSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "aim_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, aim::instruments::AIMInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(AIMSupport)