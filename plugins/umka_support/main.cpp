#include "core/plugin.h"
#include "logger.h"

#include "umka/module_umka1_decoder.h"

class UmKASupport : public satdump::Plugin
{
public:
    std::string getID() { return "umka_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, umka::UmKA1DecoderModule); }
};

PLUGIN_LOADER(UmKASupport)