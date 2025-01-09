#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "jpss/module_jpss_instruments.h"

#include "jpss/instruments/viirs/viirs_proj.h"

#include "jpss/instruments/atms/atms_calibrator.h"

class JPSSSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "jpss_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::proj::RequestSatelliteRaytracerEvent>(provideSatProjHandler);
        satdump::eventBus->register_handler<satdump::products::RequestImageCalibratorEvent>(provideImageCalibratorHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, jpss::instruments::JPSSInstrumentsDecoderModule);
    }

    static void provideSatProjHandler(const satdump::proj::RequestSatelliteRaytracerEvent &evt)
    {
        if (evt.id == "viirs_single_line")
            evt.r.push_back(std::make_shared<jpss::VIIRSNormalLineSatProj>(evt.cfg));
    }

    static void provideImageCalibratorHandler(const satdump::products::RequestImageCalibratorEvent &evt)
    {
        if (evt.id == "jpss_atms")
            evt.calibrators.push_back(std::make_shared<jpss::atms::JpssATMSCalibrator>(evt.products, evt.calib));
    }
};

PLUGIN_LOADER(JPSSSupport)