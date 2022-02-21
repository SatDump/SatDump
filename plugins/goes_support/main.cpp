#include "plugin.h"
#include "logger.h"
#include "module.h"

#include "goes/gvar/module_gvar_decoder.h"
#include "goes/gvar/module_gvar_image_decoder.h"
#include "goes/hrit/module_goes_lrit_data_decoder.h"
#include "goes/grb/module_goes_grb_cadu_extractor.h"
#include "goes/grb/module_goes_grb_data_decoder.h"

class GOESSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "spacex_support";
    }

    void init()
    {
        logger->info("GOESSupport plugin loaded!");

        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::gvar::GVARDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::gvar::GVARImageDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::hrit::GOESLRITDataDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::grb::GOESGRBCADUextractor);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::grb::GOESGRBDataDecoderModule);
    }
};

PLUGIN_LOADER(GOESSupport)