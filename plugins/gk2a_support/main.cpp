#include "core/plugin.h"
#include "logger.h"

#include "gk2a/module_gk2a_lrit_data_decoder.h"

class GK2ASupport : public satdump::Plugin
{
public:
    std::string getID() { return "gk2a_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, gk2a::lrit::GK2ALRITDataDecoderModule); }
};

PLUGIN_LOADER(GK2ASupport)