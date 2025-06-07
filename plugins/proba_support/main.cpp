#include "core/plugin.h"
#include "logger.h"

#include "proba/module_proba_instruments.h"

class ProbaSupport : public satdump::Plugin
{
public:
    std::string getID() { return "proba_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, proba::instruments::PROBAInstrumentsDecoderModule); }
};

PLUGIN_LOADER(ProbaSupport)