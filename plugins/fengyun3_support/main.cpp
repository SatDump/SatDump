#include "core/plugin.h"
#include "logger.h"

#include "fengyun3/module_fengyun_ahrpt_decoder.h"
#include "fengyun3/module_fengyun_mpt_decoder.h"
#include "fengyun3/module_fy3_instruments.h"

class FengYun3Support : public satdump::Plugin
{
public:
    std::string getID() { return "fengyun3_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::FengyunAHRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::FengyunMPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::instruments::FY3InstrumentsDecoderModule);
    }
};

PLUGIN_LOADER(FengYun3Support)