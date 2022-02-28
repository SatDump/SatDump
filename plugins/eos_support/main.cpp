#include "plugin.h"
#include "logger.h"
#include "module.h"

#include "terra/module_terra_db_demod.h"
#include "aqua/module_aqua_db_decoder.h"
#include "eos/module_eos_instruments.h"

class EOSSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "eos_support";
    }

    void init()
    {
        logger->info("EOSSupport plugin loaded!");

        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, aqua::AquaDBDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, terra::TerraDBDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, eos::instruments::EOSInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(EOSSupport)