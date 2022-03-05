#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "jpss/instruments/atms/module_jpss_atms.h"
#include "jpss/instruments/viirs/module_jpss_viirs.h"
#include "jpss/instruments/omps/module_jpss_omps.h"
#include "jpss/module_jpss_satid.h"

class JPSSSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "spacex_support";
    }

    void init()
    {
        logger->info("JPSSSupport plugin loaded!");

        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, jpss::satid::JPSSSatIDModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, jpss::atms::JPSSATMSDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, jpss::viirs::JPSSVIIRSDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, jpss::omps::JPSSOMPSDecoderModule);
    }
};

PLUGIN_LOADER(JPSSSupport)