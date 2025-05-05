#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "off2pro/module_off2pro.h"

#include "nc2pro/fci_nc_calibrator.h"
#include "nc2pro/abi_nc_calibrator.h"

//////////

#include "nat2pro/mhs_nat_calibrator.h"
#include "nat2pro/msg_nat_calibrator.h"
#include "nat2pro/amsu_nat_calibrator.h"
#include "nat2pro/avhrr_nat_calibrator.h"

class OfficalProductsSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "official_products_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::ImageProducts::RequestCalibratorEvent>(provideImageCalibratorHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, off2pro::Off2ProModule);
    }

    static void provideImageCalibratorHandler(const satdump::ImageProducts::RequestCalibratorEvent &evt)
    {
        if (evt.id == "metop_mhs_nat")
            evt.calibrators.push_back(std::make_shared<nat2pro::MHSNatCalibrator>(evt.calib, evt.products));
        else if (evt.id == "metop_amsu_nat")
            evt.calibrators.push_back(std::make_shared<nat2pro::AMSUNatCalibrator>(evt.calib, evt.products));
        else if (evt.id == "metop_avhrr_nat")
            evt.calibrators.push_back(std::make_shared<nat2pro::AVHRRNatCalibrator>(evt.calib, evt.products));
        else if (evt.id == "msg_nat_seviri")
            evt.calibrators.push_back(std::make_shared<nat2pro::MSGNatCalibrator>(evt.calib, evt.products));
        //
        else if (evt.id == "mtg_nc_fci")
            evt.calibrators.push_back(std::make_shared<nc2pro::FCINcCalibrator>(evt.calib, evt.products));
        else if (evt.id == "goes_nc_abi")
            evt.calibrators.push_back(std::make_shared<nc2pro::ABINcCalibrator>(evt.calib, evt.products));
    }
};

PLUGIN_LOADER(OfficalProductsSupport)