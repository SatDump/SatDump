#include "core/plugin.h"
#include "logger.h"

#include "mats/module_mats_instruments.h"

class MATSSupport : public satdump::Plugin
{
public:
    std::string getID() { return "mats_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, mats::instruments::MATSInstrumentsDecoderModule); }
};

PLUGIN_LOADER(MATSSupport)