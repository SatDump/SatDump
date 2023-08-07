#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "dscovr/module_dscovr_instruments.h"

class DSCOVRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "dscovr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, dscovr::instruments::DSCOVRInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(DSCOVRSupport)