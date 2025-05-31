#include "core/plugin.h"
#include "logger.h"

#include "tubin/module_tubin_decoder.h"

class TUBSATSupport : public satdump::Plugin
{
public:
    std::string getID() { return "tubsat_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, tubin::TUBINDecoderModule); }
};

PLUGIN_LOADER(TUBSATSupport)