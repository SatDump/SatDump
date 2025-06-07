#include "core/plugin.h"
#include "logger.h"

#include "dscovr/module_dscovr_instruments.h"

class DSCOVRSupport : public satdump::Plugin
{
public:
    std::string getID() { return "dscovr_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, dscovr::instruments::DSCOVRInstrumentsDecoderModule); }
};

PLUGIN_LOADER(DSCOVRSupport)