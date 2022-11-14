#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "fy4/module_fy4_lrit_data_decoder.h"

class FengYun4Support : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "fenyun4_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fy4::lrit::FY4LRITDataDecoderModule);
    }
};

PLUGIN_LOADER(FengYun4Support)