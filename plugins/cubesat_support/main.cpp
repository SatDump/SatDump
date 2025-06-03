#include "core/plugin.h"
#include "logger.h"

#include "lucky7/module_lucky7_decoder.h"
#include "lucky7/module_lucky7_demod.h"

#include "common/module_ax25_decoder.h"

#include "spino/module_spino_decoder.h"

#include "geoscan/module_geoscan_data_decoder.h"
#include "geoscan/module_geoscan_decoder.h"

class CubeSatSupport : public satdump::Plugin
{
public:
    std::string getID() { return "cubesat_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, lucky7::Lucky7DemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, lucky7::Lucky7DecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, ax25::AX25DecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, spino::SpinoDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, geoscan::GEOSCANDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, geoscan::GEOSCANDataDecoderModule);
    }
};

PLUGIN_LOADER(CubeSatSupport)