#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"
#include "common/cpu_features.h"

#include "dvbs/module_dvbs_demod.h"
#include "dvbs2/module_dvbs2_demod.h"
#include "dvbs2/module_s2_ts_extractor.h"

class DVBSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "dvb_support";
    }

    void init()
    {
#ifdef DVB_HAS_SSE41
        if (!cpu_features::get_cpu_features().CPU_X86_SSE41)
        {
            logger->error("CPU Does not support SSE4_1. Extension plugin NOT loading!");
            return;
        }
#endif
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, dvb::DVBSDemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, dvb::DVBS2DemodModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, dvbs2::S2TStoTCPModule);
    }
};

PLUGIN_LOADER(DVBSupport)