#include "core/plugin.h"
#include "logger.h"

#include "oceansat/instruments/ocm/module_oceansat_ocm.h"
#include "oceansat/module_oceansat2_db_decoder.h"

class OceansatSupport : public satdump::Plugin
{
public:
    std::string getID() { return "oceansat_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, oceansat::Oceansat2DBDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, oceansat::ocm::OceansatOCMDecoderModule);
    }
};

PLUGIN_LOADER(OceansatSupport)