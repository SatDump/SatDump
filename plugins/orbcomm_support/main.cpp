#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "orbcomm/module_orbcomm_stx_demod.h"
#include "orbcomm/module_orbcomm_stx_auto_demod.h"
#include "orbcomm/module_orbcomm_plotter.h"

class OrbcommSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "orbcomm_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, orbcomm::OrbcommSTXDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, orbcomm::OrbcommSTXAutoDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, orbcomm::OrbcommPlotterModule);
    }
};

PLUGIN_LOADER(OrbcommSupport)