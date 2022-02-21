#include "plugin.h"
#include "logger.h"
#include "module.h"

#include "meteor/module_meteor_hrpt_decoder.h"
#include "meteor/module_meteor_lrpt_decoder.h"
#include "meteor/instruments/msumr/module_meteor_msumr.h"
#include "meteor/instruments/msumr/module_meteor_msumr_lrpt.h"
#include "meteor/instruments/mtvza/module_meteor_mtvza.h"

class MeteorSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "meteor_support";
    }

    void init()
    {
        logger->info("MeteorSupport plugin loaded!");

        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::METEORHRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::METEORLRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::msumr::METEORMSUMRDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::msumr::METEORMSUMRLRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::mtvza::METEORMTVZADecoderModule);
    }
};

PLUGIN_LOADER(MeteorSupport)