#include "plugin.h"
#include "logger.h"
#include "module.h"

#include "metop/module_metop_ahrpt_decoder.h"
#include "metop/module_metop_dump_decoder.h"
#include "metop/instruments/avhrr/module_metop_avhrr.h"
#include "metop/instruments/mhs/module_metop_mhs.h"
#include "metop/instruments/iasi/module_metop_iasi.h"
#include "metop/instruments/amsu/module_metop_amsu.h"
#include "metop/instruments/gome/module_metop_gome.h"
#include "metop/instruments/ascat/module_metop_ascat.h"
#include "metop/instruments/admin_msg/module_metop_admin_msg.h"
#include "metop/module_metop_satid.h"
#include "metop/instruments/sem/module_metop_sem.h"

#include "noaa/module_noaa_hrpt_decoder.h"
#include "noaa/module_noaa_gac_decoder.h"
#include "noaa/module_noaa_dsb_decoder.h"
#include "noaa/instruments/avhrr/module_noaa_avhrr.h"
#include "noaa/module_noaa_extractor.h"
#include "noaa/instruments/hirs/module_noaa_hirs.h"
#include "noaa/instruments/mhs/module_noaa_mhs.h"
#include "noaa/instruments/amsu/module_noaa_amsu.h"

class NOAAMetOpSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "noaa_metop_support";
    }

    void init()
    {
        logger->info("NOAAMetOpSupport plugin loaded!");

        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::MetOpAHRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::MetOpDumpDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::avhrr::MetOpAVHRRDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::mhs::MetOpMHSDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::iasi::MetOpIASIDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::amsu::MetOpAMSUDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::gome::MetOpGOMEDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::ascat::MetOpASCATDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::admin_msg::MetOpAdminMsgDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::satid::MetOpSatIDModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, metop::sem::MetOpSEMDecoderModule);

        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::NOAAHRPTDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::NOAAGACDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::NOAADSBDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::avhrr::NOAAAVHRRDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::NOAAExtractorModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::hirs::NOAAHIRSDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::mhs::NOAAMHSDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, noaa::amsu::NOAAAMSUDecoderModule);
    }
};

PLUGIN_LOADER(NOAAMetOpSupport)