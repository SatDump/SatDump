#include "instruments/avhrr/avhrr_calibrator.h"
#include "instruments/mhs/mhs_calibrator.h"
#include "noaa/instruments/hirs/hirs_calibrator.h"
#include "metop/instruments/iasi/iasi_img_calibrator.h"

#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"
#include "products/image_products.h"

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
        satdump::eventBus->register_handler<satdump::ImageProducts::RequestCalibratorEvent>(provideImageCalibratorHandler);
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

    static void provideImageCalibratorHandler(const satdump::ImageProducts::RequestCalibratorEvent &evt)
    {
        if (evt.id == "noaa_avhrr3")
            evt.calibrators.push_back(std::make_shared<NoaaAVHRR3Calibrator>(evt.calib, evt.products));
        else if (evt.id == "noaa_mhs" || evt.id == "noaa_amsu")
            evt.calibrators.push_back(std::make_shared<NoaaMHSCalibrator>(evt.calib, evt.products));
        else if (evt.id == "metop_iasi_img")
            evt.calibrators.push_back(std::make_shared<metop::iasi::MetOpIASIImagingCalibrator>(evt.calib, evt.products));
        else if (evt.id == "noaa_hirs")
            evt.calibrators.push_back(std::make_shared<NoaaHIRSCalibrator>(evt.calib, evt.products));
    }
};

PLUGIN_LOADER(NOAAMetOpSupport)