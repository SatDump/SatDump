#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "meteor/module_meteor_hrpt_decoder.h"
#include "meteor/module_meteor_lrpt_decoder.h"
#include "meteor/module_meteor_dump_decoder.h"
#include "meteor/instruments/msumr/module_meteor_msumr_lrpt.h"
#include "meteor/module_meteor_instruments.h"
#include "meteor/module_meteor_dump_instruments.h"

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
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::METEORHRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::METEORLRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::METEORDumpDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::msumr::METEORMSUMRLRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::instruments::MeteorInstrumentsDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, meteor::instruments::MeteorDumpInstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(MeteorSupport)