#include "core/plugin.h"
#include "logger.h"

#include "old/off2pro/module_off2pro.h"
#include "processors/hdf/gpm/gpm_calibrator.h"

#include "processors/nc/goes/abi_nc_calibrator.h"
#include "processors/nc/mtg/fci_nc_calibrator.h"

#include "old/nc2pro/jpss/viirs_nc_calibrator.h"

//////////

#include "processors/nat/metop/amsu_nat_calibrator.h"
#include "processors/nat/metop/avhrr_nat_calibrator.h"
#include "processors/nat/metop/mhs_nat_calibrator.h"
#include "processors/nat/msg/msg_nat_calibrator.h"

class OfficalProductsSupport : public satdump::Plugin
{
public:
    std::string getID() { return "official_products_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::products::RequestImageCalibratorEvent>(provideImageCalibratorHandler);
    }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, off2pro::Off2ProModule); }

    static void provideImageCalibratorHandler(const satdump::products::RequestImageCalibratorEvent &evt)
    {
        if (evt.id == "metop_avhrr_nat")
            evt.calibrators.push_back(std::make_shared<nat2pro::AVHRRNatCalibrator>(evt.products, evt.calib));
        else if (evt.id == "msg_nat_seviri")
            evt.calibrators.push_back(std::make_shared<nat2pro::MSGNatCalibrator>(evt.products, evt.calib));
        else if (evt.id == "mtg_nc_fci")
            evt.calibrators.push_back(std::make_shared<nc2pro::FCINcCalibrator>(evt.products, evt.calib));
        else if (evt.id == "goes_nc_abi")
            evt.calibrators.push_back(std::make_shared<nc2pro::ABINcCalibrator>(evt.products, evt.calib));
        else if (evt.id == "metop_mhs_nat")
            evt.calibrators.push_back(std::make_shared<nat2pro::MHSNatCalibrator>(evt.products, evt.calib));
        else if (evt.id == "metop_amsu_nat")
            evt.calibrators.push_back(std::make_shared<nat2pro::AMSUNatCalibrator>(evt.products, evt.calib));
        else if (evt.id == "jpss_nc_viirs")
            evt.calibrators.push_back(std::make_shared<nc2pro::VIIRSNcCalibrator>(evt.products, evt.calib));
        else if (evt.id == "gpm_hdf")
            evt.calibrators.push_back(std::make_shared<satdump::official::GPMHdfCalibrator>(evt.products, evt.calib));
    }
};

PLUGIN_LOADER(OfficalProductsSupport)