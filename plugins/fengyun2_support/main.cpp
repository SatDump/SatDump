#include "core/plugin.h"

#include "fengyun2/svissr/module_svissr_decoder.h"
#include "fengyun2/svissr/module_svissr_image_decoder.h"
#include "fengyun2/svissr/svissr_calibrator.h"

class FengYun2Support : public satdump::Plugin
{
public:
    std::string getID() { return "fengyun2_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); 
        satdump::eventBus->register_handler<satdump::products::RequestImageCalibratorEvent>(provideImageCalibratorHandler);
    }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun_svissr::SVISSRDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun_svissr::SVISSRImageDecoderModule);
    }

    static void provideImageCalibratorHandler(const satdump::products::RequestImageCalibratorEvent &evt)
    {
        if (evt.id == "fy2_svissr")
            evt.calibrators.push_back(std::make_shared<fengyun_svissr::SvissrCalibrator>(evt.products, evt.calib));
    }

};

PLUGIN_LOADER(FengYun2Support)