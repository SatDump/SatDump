#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "himawaricast/module_himawaricast_data_decoder.h"

class HimawariSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "himawari_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, himawari::himawaricast::HimawariCastDataDecoderModule);
    }
};

PLUGIN_LOADER(HimawariSupport)