#include "aws/instruments/mwr/mwr_calibrator.h"
#include "core/plugin.h"
#include "logger.h"

#include "aws/module_aws_instruments.h"

class AWSSupport : public satdump::Plugin
{
public:
    std::string getID() { return "aws_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::products::RequestImageCalibratorEvent>(provideImageCalibratorHandler);
    }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, aws::AWSInstrumentsDecoderModule); }

    static void provideImageCalibratorHandler(const satdump::products::RequestImageCalibratorEvent &evt)
    {
        if (evt.id == "aws_mwr")
            evt.calibrators.push_back(std::make_shared<aws::mwr::AWSMWRCalibrator>(evt.products, evt.calib));
    }
};

PLUGIN_LOADER(AWSSupport)