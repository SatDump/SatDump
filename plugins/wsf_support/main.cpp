#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "wsfm/module_wsfm_instruments.h"

class WSFSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "wsf_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, wsfm::WSFMInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(WSFSupport)