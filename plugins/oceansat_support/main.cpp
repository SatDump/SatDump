#include "plugin.h"
#include "logger.h"
#include "module.h"

#include "oceansat/module_oceansat2_db_decoder.h"
#include "oceansat/instruments/ocm/module_oceansat_ocm.h"

class OceansatSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "oceansat_support";
    }

    void init()
    {
        logger->info("OceansatSupport plugin loaded!");

        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, oceansat::Oceansat2DBDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, oceansat::ocm::OceansatOCMDecoderModule);
    }
};

PLUGIN_LOADER(OceansatSupport)