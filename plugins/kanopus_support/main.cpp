#include "core/plugin.h"
#include "logger.h"

#include "kanopus/module_kanopusv_decoder.h"
#include "kanopus/module_kanopusv_instruments.h"

class KanopusSupport : public satdump::Plugin
{
public:
    std::string getID() { return "kanopus_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, kanopus::KanopusVDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, kanopus::KanopusVInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(KanopusSupport)