#include "c_plugin_loader.hpp"
#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"


class CPluginSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "c_plugin_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, c_plugin::CPluginLoader);
    }
};

PLUGIN_LOADER(CPluginSupport)
