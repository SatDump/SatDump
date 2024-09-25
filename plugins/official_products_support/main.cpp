#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "nc2pro/module_nc2pro.h"

#include "nc2pro/fci_nc_calibrator.h"

class OthersSupport : public satdump::Plugin
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
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, nc2pro::Nc2ProModule);
    }

    static void provideImageCalibratorHandler(const satdump::ImageProducts::RequestCalibratorEvent &evt)
    {
        if (evt.id == "mtg_nc_fci")
            evt.calibrators.push_back(std::make_shared<nc2pro::FCINcCalibrator>(evt.calib, evt.products));
    }
};

PLUGIN_LOADER(OthersSupport)