#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "saral/argos/module_saral_argos.h"

#include "angels/argos/module_angels_argos.h"

#include "cryosat/instruments/siral/module_cryosat_siral.h"

#include "coriolis/instruments/windsat/module_coriolis_windsat.h"

#include "cloudsat/instruments/cpr/module_cloudsat_cpr.h"

#include "scisat1/module_scisat1_instruments.h"

#include "orb/module_orb_decoder.h"

#include "nat2pro/module_nat2pro.h"
#include "nat2pro/mhs_nat_calibrator.h"
#include "nat2pro/msg_nat_calibrator.h"

class OthersSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "others_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::ImageProducts::RequestCalibratorEvent>(provideImageCalibratorHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, saral::argos::SaralArgosDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, angels::argos::AngelsArgosDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, cryosat::siral::CryoSatSIRALDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, coriolis::windsat::CoriolisWindSatDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, cloudsat::cpr::CloudSatCPRDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, scisat1::instruments::SciSat1InstrumentsDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, orb::ORBDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, nat2pro::Nat2ProModule);
    }

    static void provideImageCalibratorHandler(const satdump::ImageProducts::RequestCalibratorEvent &evt)
    {
        if (evt.id == "metop_mhs_nat")
            evt.calibrators.push_back(std::make_shared<nat2pro::MHSNatCalibrator>(evt.calib, evt.products));
        else if (evt.id == "msg_nat_seviri")
            evt.calibrators.push_back(std::make_shared<nat2pro::MSGNatCalibrator>(evt.calib, evt.products));
    }
};

PLUGIN_LOADER(OthersSupport)