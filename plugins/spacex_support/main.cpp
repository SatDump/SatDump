#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "spacex/module_spacex_decoder.h"
#include "spacex/module_falcon_decoder.h"

class SpaceXSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "spacex_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, spacex::SpaceXDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, spacex::FalconDecoderModule);
    }
};

PLUGIN_LOADER(SpaceXSupport)