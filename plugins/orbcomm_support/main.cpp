#include "core/plugin.h"
#include "logger.h"

#include "orbcomm/module_orbcomm_plotter.h"
#include "orbcomm/module_orbcomm_stx_auto_demod.h"
#include "orbcomm/module_orbcomm_stx_demod.h"

class OrbcommSupport : public satdump::Plugin
{
public:
    std::string getID() { return "orbcomm_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, orbcomm::OrbcommSTXDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, orbcomm::OrbcommSTXAutoDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, orbcomm::OrbcommPlotterModule);
    }
};

PLUGIN_LOADER(OrbcommSupport)