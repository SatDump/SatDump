#include "plugin.h"
#include "logger.h"
#include "module.h"

#include "terra/module_terra_db_demod.h"

#include "aqua/module_aqua_db_decoder.h"
#include "aqua/instruments/airs/module_aqua_airs.h"
#include "aqua/instruments/amsu/module_aqua_amsu.h"
#include "aqua/instruments/ceres/module_aqua_ceres.h"

#include "aura/instruments/omi/module_aura_omi.h"

#include "eos/instruments/modis/module_eos_modis.h"

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
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, aqua::airs::AquaAIRSDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, aqua::amsu::AquaAMSUDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, aqua::ceres::AquaCERESDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, aura::omi::AuraOMIDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, eos::modis::EOSMODISDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, terra::TerraDBDemodModule);
    }
};

PLUGIN_LOADER(EOSSupport)