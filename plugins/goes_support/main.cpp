#include "core/config.h"
#include "core/module.h"
#include "core/plugin.h"

#include "goes/grb/module_goes_grb_cadu_extractor.h"
#include "goes/grb/module_goes_grb_data_decoder.h"
#include "goes/gvar/module_gvar_decoder.h"
#include "goes/gvar/module_gvar_image_decoder.h"
#include "goes/hrit/module_goes_lrit_data_decoder.h"
#include "goes/mdl/module_goes_mdl_decoder.h"
#include "goes/raw/module_goesr_instruments.h"
#include "goes/sd/module_goesn_sd_decoder.h"
#include "goes/sd/module_sd_image_decoder.h"

#include "goes/hrit/dcs/dcs_settings.h"

class GOESSupport : public satdump::Plugin
{
public:
    std::string getID() { return "goes_support"; }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
        satdump::eventBus->register_handler<satdump::config::RegisterPluginConfigHandlersEvent>(registerConfigHandler);
        satdump::eventBus->register_handler<goes::hrit::GOESLRITDataDecoderModule::DCPUpdateEvent>(goes::hrit::GOESLRITDataDecoderModule::updateDCPs);
        goes::hrit::initDcsConfig();
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::gvar::GVARDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::gvar::GVARImageDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::hrit::GOESLRITDataDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::grb::GOESGRBCADUextractor);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::grb::GOESGRBDataDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::mdl::GOESMDLDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::sd::GOESNSDDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::sd::SDImageDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, goes::instruments::GOESRInstrumentsDecoderModule);
    }

    static void registerConfigHandler(const satdump::config::RegisterPluginConfigHandlersEvent &evt)
    {
        evt.plugin_config_handlers.push_back({"GOES HRIT DCS Parser", goes::hrit::renderDcsConfig, goes::hrit::saveDcsConfig});
    }
};

PLUGIN_LOADER(GOESSupport)