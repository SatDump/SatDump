#include "core/plugin.h"
#include "logger.h"

#include "wsfm/module_wsfm_instruments.h"

class WSFSupport : public satdump::Plugin
{
public:
    std::string getID() { return "wsf_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, wsfm::WSFMInstrumentsDecoderModule); }
};

PLUGIN_LOADER(WSFSupport)