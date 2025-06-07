#include "core/plugin.h"
#include "logger.h"

#include "jason3/module_jason3_instruments.h"

class Jason3Support : public satdump::Plugin
{
public:
    std::string getID() { return "jason3_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, jason3::instruments::Jason3InstrumentsDecoderModule); }
};

PLUGIN_LOADER(Jason3Support)