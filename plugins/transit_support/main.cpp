#include "core/plugin.h"
#include "logger.h"

#include "module_transit_demod.h"
#include "module_transit_decoder.h"
#include "module_transit_data.h"

class TransitSupport : public satdump::Plugin
{
public:
    std::string getID() { return "transit_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler);
        // satdump::eventBus->register_handler<satdump::projection::RequestSatelliteRaytracerEvent>(provideSatProjHandler);
    }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, transit::TransitDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, transit::TransitDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, transit::TransitDataDecoderModule);
    }
};

PLUGIN_LOADER(TransitSupport)