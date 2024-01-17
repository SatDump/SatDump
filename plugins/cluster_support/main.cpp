#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "cluster/module_cluster_instruments.h"

class CLUSTERSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "cluster_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }
    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, cluster::instruments::CLUSTERInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(CLUSTERSupport)