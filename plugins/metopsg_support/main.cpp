#include "core/plugin.h"

#include "pipeline/module.h"

#include "metopsg/module_metopsg_instruments.h"

class MetOpSGSupport : public satdump::Plugin
{
public:
    std::string getID() { return "metopsg_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler);
        // satdump::eventBus->register_handler<satdump::products::RequestImageCalibratorEvent>(provideImageCalibratorHandler);
    }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, metopsg::instruments::MetOpSGInstrumentsDecoderModule); }

    // static void provideImageCalibratorHandler(const satdump::products::RequestImageCalibratorEvent &evt)
    // {
    //     if (evt.id == "noaa_avhrr3")
    //         evt.calibrators.push_back(std::make_shared<noaa_metop::NoaaAVHRR3Calibrator>(evt.products, evt.calib));
    //     else if (evt.id == "noaa_mhs" || evt.id == "noaa_amsu")
    //         evt.calibrators.push_back(std::make_shared<noaa_metop::NoaaMHSCalibrator>(evt.products, evt.calib));
    //     else if (evt.id == "metop_iasi_img")
    //         evt.calibrators.push_back(std::make_shared<metop::iasi::MetOpIASIImagingCalibrator>(evt.products, evt.calib));
    //     else if (evt.id == "metop_ascat")
    //         evt.calibrators.push_back(std::make_shared<metop::ascat::MetOpASCATCalibrator>(evt.products, evt.calib));
    //     else if (evt.id == "noaa_hirs")
    //         evt.calibrators.push_back(std::make_shared<noaa_metop::NoaaHIRSCalibrator>(evt.products, evt.calib));
    // }
};

PLUGIN_LOADER(MetOpSGSupport)