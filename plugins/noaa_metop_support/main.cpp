#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "metop/module_metop_ahrpt_decoder.h"
#include "metop/module_metop_dump_decoder.h"
#include "metop/module_metop_instruments.h"

#include "noaa/module_noaa_hrpt_decoder.h"
#include "noaa/module_noaa_gac_decoder.h"
#include "noaa/module_noaa_dsb_decoder.h"
#include "noaa/module_noaa_instruments.h"

class NOAAMetOpSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "noaa_metop_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::MetOpAHRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::MetOpDumpDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::instruments::MetOpInstrumentsDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::NOAAHRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::NOAAGACDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::NOAADSBDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::instruments::NOAAInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(NOAAMetOpSupport)