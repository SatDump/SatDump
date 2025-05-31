#include "core/plugin.h"
#include "logger.h"

#include "geonetcast/module_geonetcast_decoder.h"

class GeoNetCastSupport : public satdump::Plugin
{
public:
    std::string getID() { return "geonetcast_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, geonetcast::GeoNetCastDecoderModule); }
};

PLUGIN_LOADER(GeoNetCastSupport)