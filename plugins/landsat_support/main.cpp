#include "core/plugin.h"
#include "logger.h"

#include "ldcm/module_ldcm_instruments.h"

class LandSatSupport : public satdump::Plugin
{
public:
    std::string getID() { return "landsat_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, ldcm::instruments::LDCMInstrumentsDecoderModule); }
};

PLUGIN_LOADER(LandSatSupport)