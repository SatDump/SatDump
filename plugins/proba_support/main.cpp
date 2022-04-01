#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "proba/instruments/swap/module_proba_swap.h"
#include "proba/instruments/chris/module_proba_chris.h"
#include "proba/instruments/hrc/module_proba_hrc.h"

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
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, proba::swap::ProbaSWAPDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, proba::chris::ProbaCHRISDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, proba::hrc::ProbaHRCDecoderModule);
    }
};

PLUGIN_LOADER(ProbaSupport)