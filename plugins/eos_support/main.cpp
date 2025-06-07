#include "core/plugin.h"
#include "logger.h"

#include "aqua/module_aqua_db_decoder.h"
#include "eos/module_eos_instruments.h"
#include "terra/module_terra_db_demod.h"

#include "eos/instruments/modis/calibrator/modis_calibrator.h"
// #include "eos/instruments/modis/day_fire.h"

class EOSSupport : public satdump::Plugin
{
public:
    std::string getID() { return "eos_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::products::RequestImageCalibratorEvent>(provideImageCalibratorHandler);
        logger->critical("TODOREWORK NO MORE CPP COMPOSITES!!!"); //   satdump::eventBus->register_handler<satdump::RequestCppCompositeEvent>(provideCppCompositeHandler);
    }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, aqua::AquaDBDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, terra::TerraDBDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, eos::instruments::EOSInstrumentsDecoderModule);
    }

    static void provideImageCalibratorHandler(const satdump::products::RequestImageCalibratorEvent &evt)
    {
        if (evt.id == "eos_modis")
            evt.calibrators.push_back(std::make_shared<eos::modis::EosMODISCalibrator>(evt.products, evt.calib));
    }

    // static void provideCppCompositeHandler(const satdump::RequestCppCompositeEvent &evt)
    // {
    //     if (evt.id == "day_fire")
    //         evt.compositors.push_back(modis::dayFireCompositor);
    // }
};

PLUGIN_LOADER(EOSSupport)
