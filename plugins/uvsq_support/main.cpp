#include "core/plugin.h"
#include "logger.h"

#include "module_uvsqsatng_instruments.h"

class UVSQSupport : public satdump::Plugin
{
public:
    std::string getID() { return "uvsq_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, uvsq::UVSQSatNGInstrumentsDecoderModule); }
};

PLUGIN_LOADER(UVSQSupport)