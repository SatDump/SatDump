#define SATDUMP_DLL_EXPORT 1
#include "module.h"
#include "logger.h"

ProcessingModule::ProcessingModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : d_input_file(input_file),
                                                                                                                                          d_output_file_hint(output_file_hint),
                                                                                                                                          d_parameters(parameters)
{
    input_active = false;
}

std::vector<std::string> ProcessingModule::getOutputs()
{
    return d_output_files;
}

void ProcessingModule::setInputType(ModuleDataType type)
{
    input_data_type = type;
}

void ProcessingModule::setOutputType(ModuleDataType type)
{
    output_data_type = type;
}

void ProcessingModule::drawUI()
{
}

// Registry
SATDUMP_DLL std::map<std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, std::map<std::string, std::string>)>> modules_registry;

#define REGISTER_MODULE(module) modules_registry.emplace(module::getID(), module::getInstance)

#include "modules/module_qpsk_demod.h"
#include "modules/module_oqpsk_demod.h"
#include "modules/module_bpsk_demod.h"
#include "modules/module_fsk_demod.h"

#include "modules/metop/module_metop_ahrpt_decoder.h"
#include "modules/metop/instruments/avhrr/module_metop_avhrr.h"
#include "modules/metop/instruments/mhs/module_metop_mhs.h"
#include "modules/metop/instruments/iasi/module_metop_iasi.h"
#include "modules/metop/instruments/amsu/module_metop_amsu.h"
#include "modules/metop/instruments/gome/module_metop_gome.h"

#include "modules/fengyun/module_fengyun_ahrpt_decoder.h"
#include "modules/fengyun/module_fengyun_mpt_decoder.h"
#include "modules/fengyun/instruments/virr/module_fengyun_virr.h"
#include "modules/fengyun/instruments/mersi1/module_fengyun_mersi1.h"
#include "modules/fengyun/instruments/mersi2/module_fengyun_mersi2.h"

#include "modules/aqua/module_aqua_db_decoder.h"
#include "modules/aqua/instruments/airs/module_aqua_airs.h"
#include "modules/aqua/instruments/amsu/module_aqua_amsu.h"
#include "modules/aqua/instruments/ceres/module_aqua_ceres.h"

#include "modules/eos/instruments/modis/module_eos_modis.h"
#include "modules/eos/instruments/modis/module_eos_modis_extractor.h"

#include "modules/noaa/module_noaa_hrpt_demod.h"
#include "modules/noaa/module_noaa_dsb_demod.h"
#include "modules/noaa/instruments/avhrr/module_noaa_avhrr.h"

#include "modules/meteor/module_meteor_hrpt_demod.h"
#include "modules/meteor/module_meteor_hrpt_decoder.h"
#include "modules/meteor/module_meteor_lrpt_decoder.h"
#include "modules/meteor/instruments/msumr/module_meteor_msumr.h"
#include "modules/meteor/instruments/msumr/module_meteor_msumr_lrpt.h"
#include "modules/meteor/instruments/mtvza/module_meteor_mtvza.h"

#include "modules/npp/module_npp_hrd_decoder.h"

#include "modules/jpss/instruments/atms/module_jpss_atms.h"
#include "modules/jpss/instruments/viirs/module_jpss_viirs.h"

#include "modules/proba/module_proba_s_decoder.h"
#include "modules/proba/instruments/swap/module_proba_swap.h"
#include "modules/proba/instruments/chris/module_proba_chris.h"
#include "modules/proba/instruments/hrc/module_proba_hrc.h"

#include "modules/elektro/module_elektro_rdas_decoder.h"
#include "modules/elektro/instruments/msugs/module_elektro_msugs.h"

#include "modules/terra/module_terra_db_demod.h"
#include "modules/terra/module_terra_db_decoder.h"

#include "modules/falcon/module_falcon_tlm_decoder.h"
#include "modules/falcon/module_falcon_decoder.h"

#include "modules/smap/module_smap_s_decoder.h"

#include "modules/oceansat/module_oceansat2_db_decoder.h"
#include "modules/oceansat/instruments/ocm/module_oceansat_ocm.h"

void registerModules()
{
    // Register modules

    // Demods
    REGISTER_MODULE(QPSKDemodModule);
    REGISTER_MODULE(OQPSKDemodModule);
    REGISTER_MODULE(BPSKDemodModule);
    REGISTER_MODULE(FSKDemodModule);

    // MetOp
    REGISTER_MODULE(metop::MetOpAHRPTDecoderModule);
    REGISTER_MODULE(metop::avhrr::MetOpAVHRRDecoderModule);
    REGISTER_MODULE(metop::mhs::MetOpMHSDecoderModule);
    REGISTER_MODULE(metop::iasi::MetOpIASIDecoderModule);
    REGISTER_MODULE(metop::amsu::MetOpAMSUDecoderModule);
    REGISTER_MODULE(metop::gome::MetOpGOMEDecoderModule);

    // FengYun-3
    REGISTER_MODULE(fengyun::FengyunAHRPTDecoderModule);
    REGISTER_MODULE(fengyun::FengyunMPTDecoderModule);
    REGISTER_MODULE(fengyun::virr::FengyunVIRRDecoderModule);
    REGISTER_MODULE(fengyun::mersi1::FengyunMERSI1DecoderModule);
    REGISTER_MODULE(fengyun::mersi2::FengyunMERSI2DecoderModule);

    // Aqua
    REGISTER_MODULE(aqua::AquaDBDecoderModule);
    REGISTER_MODULE(aqua::airs::AquaAIRSDecoderModule);
    REGISTER_MODULE(aqua::amsu::AquaAMSUDecoderModule);
    REGISTER_MODULE(aqua::ceres::AquaCERESDecoderModule);

    // EOS
    REGISTER_MODULE(eos::modis::EOSMODISDecoderModule);
    REGISTER_MODULE(eos::modis::EOSMODISExtractorModule);

    // NOAA
    REGISTER_MODULE(noaa::NOAAHRPTDemodModule);
    REGISTER_MODULE(noaa::NOAADSBDemodModule);
    REGISTER_MODULE(noaa::avhrr::NOAAAVHRRDecoderModule);

    // METEOR
    REGISTER_MODULE(meteor::METEORHRPTDemodModule);
    REGISTER_MODULE(meteor::METEORHRPTDecoderModule);
    REGISTER_MODULE(meteor::METEORLRPTDecoderModule);
    REGISTER_MODULE(meteor::msumr::METEORMSUMRDecoderModule);
    REGISTER_MODULE(meteor::msumr::METEORMSUMRLRPTDecoderModule);
    REGISTER_MODULE(meteor::mtvza::METEORMTVZADecoderModule);

    // S-NPP
    REGISTER_MODULE(npp::NPPHRDDecoderModule);

    // JPSS
    REGISTER_MODULE(jpss::atms::JPSSATMSDecoderModule);
    REGISTER_MODULE(jpss::viirs::JPSSVIIRSDecoderModule);

    // Proba
    REGISTER_MODULE(proba::ProbaSDecoderModule);
    REGISTER_MODULE(proba::swap::ProbaSWAPDecoderModule);
    REGISTER_MODULE(proba::chris::ProbaCHRISDecoderModule);
    REGISTER_MODULE(proba::hrc::ProbaHRCDecoderModule);

    // ELEKTRO
    REGISTER_MODULE(elektro::ElektroRDASDecoderModule);
    REGISTER_MODULE(elektro::msugs::ELEKTROMSUGSDecoderModule);

    // Terra
    REGISTER_MODULE(terra::TerraDBDemodModule);
    REGISTER_MODULE(terra::TerraDBDecoderModule);

    // Falcon
    REGISTER_MODULE(falcon::FalconTLMDecoderModule);
    REGISTER_MODULE(falcon::FalconDecoderModule);

    // SMAP
    REGISTER_MODULE(smap::SMAPSDecoderModule);

    // Oceansat
    REGISTER_MODULE(oceansat::Oceansat2DBDecoderModule);
    REGISTER_MODULE(oceansat::ocm::OceansatOCMDecoderModule);

    // Log them out
    logger->debug("Registered modules (" + std::to_string(modules_registry.size()) + ") : ");
    for (std::pair<const std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, std::map<std::string, std::string>)>> &it : modules_registry)
        logger->debug(" - " + it.first);
}