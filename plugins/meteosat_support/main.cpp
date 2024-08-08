#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "msg/module_msg_instruments_decoder.h"

class OthersSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "meteosat_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
        // satdump::eventBus->register_handler<satdump::ImageProducts::RequestCalibratorEvent>(provideImageCalibratorHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteosat::MSGInstrumentsDecoderModule);
    }

    // static void provideImageCalibratorHandler(const satdump::ImageProducts::RequestCalibratorEvent &evt)
    // {
    //     // if (evt.id == "metop_mhs_nat")
    //     //     evt.calibrators.push_back(std::make_shared<nat2pro::MHSNatCalibrator>(evt.calib, evt.products));
    // }
};

PLUGIN_LOADER(OthersSupport)