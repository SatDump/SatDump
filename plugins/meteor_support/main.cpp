#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "meteor/module_meteor_hrpt_decoder.h"
#include "meteor/module_meteor_lrpt_decoder.h"
#include "meteor/module_meteor_xband_decoder.h"
#include "meteor/instruments/msumr/module_meteor_msumr_lrpt.h"
#include "meteor/module_meteor_instruments.h"
#include "meteor/module_meteor_xband_instruments.h"

#include "meteor/module_meteor_qpsk_kmss_decoder.h"

#include "meteor/instruments/msumr/msumr_calibrator.h"

class MeteorSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "meteor_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::products::RequestImageCalibratorEvent>(provideImageCalibratorHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::METEORHRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::METEORLRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::MeteorXBandDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::msumr::METEORMSUMRLRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::instruments::MeteorInstrumentsDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::instruments::MeteorXBandInstrumentsDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::MeteorQPSKKmssDecoderModule);
    }

    static void provideImageCalibratorHandler(const satdump::products::RequestImageCalibratorEvent &evt)
    {
        if (evt.id == "meteor_msumr")
            evt.calibrators.push_back(std::make_shared<meteor::MeteorMsuMrCalibrator>(evt.products, evt.calib));
    }
};

PLUGIN_LOADER(MeteorSupport)