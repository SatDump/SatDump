#include "core/plugin.h"
#include "logger.h"

#include "hinode/module_hinode_instruments.h"

class HinodeSupport : public satdump::Plugin
{
public:
    std::string getID() { return "hinode_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, hinode::instruments::HinodeInstrumentsDecoderModule); }
};

PLUGIN_LOADER(HinodeSupport)