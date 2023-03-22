#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "lucky7/module_lucky7_demod.h"
#include "lucky7/module_lucky7_decoder.h"

class CubeSatSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "cubesat_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, lucky7::Lucky7DemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, lucky7::Lucky7DecoderModule);
    }
};

PLUGIN_LOADER(CubeSatSupport)