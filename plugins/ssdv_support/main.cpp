#include "core/plugin.h"
#include "logger.h"

#include "pipeline/module.h"
#include "ssdv/ssdv_decode.h"

class SSDVSupport : public satdump::Plugin
{
public:
    std::string getID() { return "ssdv_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    // static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, ssdv::instruments::SSDVInstrumentsDecoderModule); }
    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, ssdv::SSDVInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(SSDVSupport)
