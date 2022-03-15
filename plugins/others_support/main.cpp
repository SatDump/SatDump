#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "saral/argos/module_saral_argos.h"

#include "angels/argos/module_angels_argos.h"

#include "cryosat/instruments/siral/module_cryosat_siral.h"

#include "coriolis/instruments/windsat/module_coriolis_windsat.h"

#include "cloudsat/instruments/cpr/module_cloudsat_cpr.h"

class OthersSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "others_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, saral::argos::SaralArgosDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, angels::argos::AngelsArgosDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, cryosat::siral::CryoSatSIRALDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, coriolis::windsat::CoriolisWindSatDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, cloudsat::cpr::CloudSatCPRDecoderModule);
    }
};

PLUGIN_LOADER(OthersSupport)