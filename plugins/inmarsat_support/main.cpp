#include "core/plugin.h"
#include "logger.h"

#include "stdc/module_stdc_decoder.h"
#include "stdc/module_stdc_parser.h"

#include "aero/module_aero_decoder.h"
#include "aero/module_aero_parser.h"

class InmarsatSupport : public satdump::Plugin
{
public:
    std::string getID() { return "inmarsat_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, inmarsat::stdc::STDCDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, inmarsat::stdc::STDCParserModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, inmarsat::aero::AeroDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, inmarsat::aero::AeroParserModule);
    }
};

PLUGIN_LOADER(InmarsatSupport)