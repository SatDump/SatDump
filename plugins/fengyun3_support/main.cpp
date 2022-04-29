#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "fengyun3/module_fengyun_ahrpt_decoder.h"
#include "fengyun3/module_fengyun_mpt_decoder.h"

#include "fengyun3/instruments/mersi1/module_fengyun_mersi1.h"
#include "fengyun3/instruments/mersi2/module_fengyun_mersi2.h"

#include "fengyun3/instruments/mersill/module_fengyun_mersill.h"

#include "fengyun3/module_fy3_instruments.h"

class FengYun3Support : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "fengyun3_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::FengyunAHRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::FengyunMPTDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mersi1::FengyunMERSI1DecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mersi2::FengyunMERSI2DecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mersill::FengyunMERSILLDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::instruments::FY3InstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(FengYun3Support)