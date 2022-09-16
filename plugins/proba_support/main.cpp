#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "proba/module_proba_instruments.h"

class ProbaSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "proba_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, proba::instruments::PROBAInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(ProbaSupport)