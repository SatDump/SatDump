#include "plugin.h"
#include "logger.h"
#include "module.h"

#include "proba/instruments/swap/module_proba_swap.h"
#include "proba/instruments/chris/module_proba_chris.h"
#include "proba/instruments/hrc/module_proba_hrc.h"
#include "proba/instruments/vegetation/module_proba_vegetation.h"

class ProbaSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "proba_support";
    }

    void init()
    {
        logger->info("ProbaSupport plugin loaded!");

        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, proba::swap::ProbaSWAPDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, proba::chris::ProbaCHRISDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, proba::hrc::ProbaHRCDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, proba::vegetation::ProbaVegetationDecoderModule);
    }
};

PLUGIN_LOADER(ProbaSupport)