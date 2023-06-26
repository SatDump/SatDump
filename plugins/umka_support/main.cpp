#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "umka/module_umka1_decoder.h"

class UmKASupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "umka_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, umka::UmKA1DecoderModule);
    }
};

PLUGIN_LOADER(UmKASupport)