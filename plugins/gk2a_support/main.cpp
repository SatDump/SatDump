#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "gk2a/module_gk2a_lrit_data_decoder.h"

class GK2ASupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "gk2a_support";
    }

    void init()
    {
        logger->info("GK2ASupport plugin loaded!");

        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, gk2a::lrit::GK2ALRITDataDecoderModule);
    }
};

PLUGIN_LOADER(GK2ASupport)