#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "fengyun3/module_fengyun_ahrpt_decoder.h"
#include "fengyun3/module_fengyun_mpt_decoder.h"
#include "fengyun3/instruments/virr/module_fengyun_virr.h"
#include "fengyun3/instruments/mersi1/module_fengyun_mersi1.h"
#include "fengyun3/instruments/mersi2/module_fengyun_mersi2.h"
#include "fengyun3/instruments/erm/module_fengyun_erm.h"
#include "fengyun3/instruments/mwhs/module_fengyun_mwhs.h"
#include "fengyun3/instruments/waai/module_fengyun_waai.h"
#include "fengyun3/instruments/mwri/module_fengyun_mwri.h"
#include "fengyun3/instruments/mwts/module_fengyun_mwts.h"
#include "fengyun3/instruments/mwhs2/module_fengyun_mwhs2.h"
#include "fengyun3/module_fengyun_satid.h"
#include "fengyun3/instruments/mersill/module_fengyun_mersill.h"
#include "fengyun3/instruments/windrad/module_fengyun_windrad.h"
#include "fengyun3/instruments/mwts2/module_fengyun_mwts2.h"
#include "fengyun3/instruments/mwts3/module_fengyun_mwts3.h"
#include "fengyun3/instruments/xeuvi/module_fengyun_xeuvi.h"

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
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::virr::FengyunVIRRDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mersi1::FengyunMERSI1DecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mersi2::FengyunMERSI2DecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::erm::FengyunERMDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mwhs::FengyunMWHSDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::waai::FengyunWAAIDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mwri::FengyunMWRIDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mwts::FengyunMWTSDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mwhs2::FengyunMWHS2DecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::satid::FengYunSatIDModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mersill::FengyunMERSILLDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::windrad::FengyunWindRADDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mwts2::FengyunMWTS2DecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::mwts3::FengyunMWTS3DecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun3::xeuvi::FengyunXEUVIDecoderModule);
    }
};

PLUGIN_LOADER(FengYun3Support)