#define SATDUMP_DLL_EXPORT 1
#include "module.h"
#include "logger.h"

SATDUMP_DLL float ui_scale = 1;                 // UI Scaling factor, set to 1 (no scaling) by default
SATDUMP_DLL int demod_constellation_size = 200; // Demodulator constellation size

ProcessingModule::ProcessingModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : d_input_file(input_file),
                                                                                                                                          d_output_file_hint(output_file_hint),
                                                                                                                                          d_parameters(parameters)
{
    input_active = false;
    streamingInput = false;
}

std::vector<std::string> ProcessingModule::getOutputs()
{
    return d_output_files;
}

void ProcessingModule::setInputType(ModuleDataType type)
{
    input_data_type = type;

    if (type != DATA_FILE)
        streamingInput = true;
}

void ProcessingModule::setOutputType(ModuleDataType type)
{
    output_data_type = type;
}

ModuleDataType ProcessingModule::getInputType()
{
    return input_data_type;
}

ModuleDataType ProcessingModule::getOutputType()
{
    return output_data_type;
}

void ProcessingModule::init()
{
}

void ProcessingModule::stop()
{
}

void ProcessingModule::drawUI(bool /*window*/)
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
#include "modules/metop/module_new_metop_ahrpt_decoder.h"
#include "modules/metop/instruments/avhrr/module_metop_avhrr.h"
#include "modules/metop/instruments/mhs/module_metop_mhs.h"
#include "modules/metop/instruments/iasi/module_metop_iasi.h"
#include "modules/metop/instruments/amsu/module_metop_amsu.h"
#include "modules/metop/instruments/gome/module_metop_gome.h"
#include "modules/metop/instruments/ascat/module_metop_ascat.h"
#include "modules/metop/instruments/admin_msg/module_metop_admin_msg.h"
#include "modules/metop/module_metop_satid.h"
#include "modules/metop/instruments/sem/module_metop_sem.h"

#include "modules/fengyun/module_new_fengyun_ahrpt_decoder.h"
#include "modules/fengyun/module_fengyun_ahrpt_decoder.h"
#include "modules/fengyun/module_new_fengyun_mpt_decoder.h"
#include "modules/fengyun/module_fengyun_mpt_decoder.h"
#include "modules/fengyun/instruments/virr/module_fengyun_virr.h"
#include "modules/fengyun/instruments/mersi1/module_fengyun_mersi1.h"
#include "modules/fengyun/instruments/mersi2/module_fengyun_mersi2.h"
#include "modules/fengyun/instruments/erm/module_fengyun_erm.h"
#include "modules/fengyun/instruments/mwhs/module_fengyun_mwhs.h"
#include "modules/fengyun/instruments/waai/module_fengyun_waai.h"
#include "modules/fengyun/instruments/mwri/module_fengyun_mwri.h"
#include "modules/fengyun/instruments/mwts/module_fengyun_mwts.h"
#include "modules/fengyun/instruments/mwhs2/module_fengyun_mwhs2.h"
#include "modules/fengyun/module_fengyun_satid.h"
#include "modules/fengyun/instruments/mersill/module_fengyun_mersill.h"
#include "modules/fengyun/instruments/windrad/module_fengyun_windrad.h"
#include "modules/fengyun/instruments/mwts3/module_fengyun_mwts3.h"

#include "modules/aqua/module_aqua_db_decoder.h"
#include "modules/aqua/instruments/airs/module_aqua_airs.h"
#include "modules/aqua/instruments/amsu/module_aqua_amsu.h"
#include "modules/aqua/instruments/ceres/module_aqua_ceres.h"

#include "modules/aura/instruments/omi/module_aura_omi.h"

#include "modules/eos/instruments/modis/module_eos_modis.h"
#include "modules/eos/instruments/modis/module_eos_modis_extractor.h"

#include "modules/noaa/module_noaa_hrpt_demod.h"
#include "modules/noaa/module_noaa_dsb_demod.h"
#include "modules/noaa/instruments/avhrr/module_noaa_avhrr.h"
#include "modules/noaa/module_noaa_extractor.h"
#include "modules/noaa/instruments/hirs/module_noaa_hirs.h"
#include "modules/noaa/instruments/mhs/module_noaa_mhs.h"
#include "modules/noaa/instruments/amsu/module_noaa_amsu.h"

#include "modules/meteor/module_meteor_hrpt_demod.h"
#include "modules/meteor/module_meteor_hrpt_decoder.h"
#include "modules/meteor/module_meteor_lrpt_decoder.h"
#include "modules/meteor/instruments/msumr/module_meteor_msumr.h"
#include "modules/meteor/instruments/msumr/module_meteor_msumr_lrpt.h"
#include "modules/meteor/instruments/mtvza/module_meteor_mtvza.h"

#include "modules/npp/module_npp_hrd_decoder.h"
#include "modules/npp/module_new_npp_hrd_decoder.h"

#include "modules/jpss/instruments/atms/module_jpss_atms.h"
#include "modules/jpss/instruments/viirs/module_jpss_viirs.h"
#include "modules/jpss/instruments/cris/module_jpss_cris.h"

#include "modules/proba/module_proba_s_decoder.h"
#include "modules/proba/instruments/swap/module_proba_swap.h"
#include "modules/proba/instruments/chris/module_proba_chris.h"
#include "modules/proba/instruments/hrc/module_proba_hrc.h"
#include "modules/proba/instruments/vegetation/module_proba_vegetation.h"

#include "modules/elektro_arktika/module_rdas_decoder.h"
#include "modules/elektro_arktika/instruments/msugs/module_msugs.h"
#include "modules/elektro_arktika/module_tlm_demod.h"

#include "modules/terra/module_terra_db_demod.h"
#include "modules/terra/module_terra_db_decoder.h"

#include "modules/spacex/module_spacex_decoder.h"
#include "modules/spacex/module_falcon_decoder.h"

#include "modules/smap/module_smap_s_decoder.h"

#include "modules/oceansat/module_oceansat2_db_decoder.h"
#include "modules/oceansat/instruments/ocm/module_oceansat_ocm.h"

#include "modules/saral/module_saral_decoder.h"
#include "modules/saral/argos/module_saral_argos.h"

#include "modules/goes/gvar/module_gvar_decoder.h"
#include "modules/goes/gvar/module_gvar_image_decoder.h"

#include "modules/goes/hrit/module_goes_hrit_decoder.h"
#include "modules/goes/hrit/module_goes_lrit_data_decoder.h"

#include "modules/fengyun_svissr/module_svissr_decoder.h"
#include "modules/fengyun_svissr/module_svissr_image_decoder.h"

#include "modules/jason3/module_jason3_decoder.h"
#include "modules/jason3/instruments/poseidon/module_jason3_poseidon.h"
#include "modules/jason3/instruments/amr2/module_jason3_amr2.h"

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
    REGISTER_MODULE(metop::NewMetOpAHRPTDecoderModule);
    REGISTER_MODULE(metop::avhrr::MetOpAVHRRDecoderModule);
    REGISTER_MODULE(metop::mhs::MetOpMHSDecoderModule);
    REGISTER_MODULE(metop::iasi::MetOpIASIDecoderModule);
    REGISTER_MODULE(metop::amsu::MetOpAMSUDecoderModule);
    REGISTER_MODULE(metop::gome::MetOpGOMEDecoderModule);
    REGISTER_MODULE(metop::ascat::MetOpASCATDecoderModule);
    REGISTER_MODULE(metop::admin_msg::MetOpAdminMsgDecoderModule);
    REGISTER_MODULE(metop::satid::MetOpSatIDModule);
    REGISTER_MODULE(metop::sem::MetOpSEMDecoderModule);

    // FengYun-3
    REGISTER_MODULE(fengyun::NewFengyunAHRPTDecoderModule);
    REGISTER_MODULE(fengyun::FengyunAHRPTDecoderModule);
    REGISTER_MODULE(fengyun::NewFengyunMPTDecoderModule);
    REGISTER_MODULE(fengyun::FengyunMPTDecoderModule);
    REGISTER_MODULE(fengyun::virr::FengyunVIRRDecoderModule);
    REGISTER_MODULE(fengyun::mersi1::FengyunMERSI1DecoderModule);
    REGISTER_MODULE(fengyun::mersi2::FengyunMERSI2DecoderModule);
    REGISTER_MODULE(fengyun::erm::FengyunERMDecoderModule);
    REGISTER_MODULE(fengyun::mwhs::FengyunMWHSDecoderModule);
    REGISTER_MODULE(fengyun::waai::FengyunWAAIDecoderModule);
    REGISTER_MODULE(fengyun::mwri::FengyunMWRIDecoderModule);
    REGISTER_MODULE(fengyun::mwts::FengyunMWTSDecoderModule);
    REGISTER_MODULE(fengyun::mwhs2::FengyunMWHS2DecoderModule);
    REGISTER_MODULE(fengyun::satid::FengYunSatIDModule);
    REGISTER_MODULE(fengyun::mersill::FengyunMERSILLDecoderModule);
    REGISTER_MODULE(fengyun::windrad::FengyunWindRADDecoderModule);
    REGISTER_MODULE(fengyun::mwts3::FengyunMWTS3DecoderModule);

    // Aqua
    REGISTER_MODULE(aqua::AquaDBDecoderModule);
    REGISTER_MODULE(aqua::airs::AquaAIRSDecoderModule);
    REGISTER_MODULE(aqua::amsu::AquaAMSUDecoderModule);
    REGISTER_MODULE(aqua::ceres::AquaCERESDecoderModule);

    // Aura
    REGISTER_MODULE(aura::omi::AuraOMIDecoderModule);

    // EOS
    REGISTER_MODULE(eos::modis::EOSMODISDecoderModule);
    REGISTER_MODULE(eos::modis::EOSMODISExtractorModule);

    // NOAA
    REGISTER_MODULE(noaa::NOAAHRPTDemodModule);
    REGISTER_MODULE(noaa::NOAADSBDemodModule);
    REGISTER_MODULE(noaa::avhrr::NOAAAVHRRDecoderModule);
    REGISTER_MODULE(noaa::NOAAExtractorModule);
    REGISTER_MODULE(noaa::hirs::NOAAHIRSDecoderModule);
    REGISTER_MODULE(noaa::mhs::NOAAMHSDecoderModule);
    REGISTER_MODULE(noaa::amsu::NOAAAMSUDecoderModule);

    // METEOR
    REGISTER_MODULE(meteor::METEORHRPTDemodModule);
    REGISTER_MODULE(meteor::METEORHRPTDecoderModule);
    REGISTER_MODULE(meteor::METEORLRPTDecoderModule);
    REGISTER_MODULE(meteor::msumr::METEORMSUMRDecoderModule);
    REGISTER_MODULE(meteor::msumr::METEORMSUMRLRPTDecoderModule);
    REGISTER_MODULE(meteor::mtvza::METEORMTVZADecoderModule);

    // S-NPP
    REGISTER_MODULE(npp::NPPHRDDecoderModule);
    REGISTER_MODULE(npp::NewNPPHRDDecoderModule);

    // JPSS
    REGISTER_MODULE(jpss::atms::JPSSATMSDecoderModule);
    REGISTER_MODULE(jpss::viirs::JPSSVIIRSDecoderModule);
    REGISTER_MODULE(jpss::cris::JPSSCrISDecoderModule);

    // Proba
    REGISTER_MODULE(proba::ProbaSDecoderModule);
    REGISTER_MODULE(proba::swap::ProbaSWAPDecoderModule);
    REGISTER_MODULE(proba::chris::ProbaCHRISDecoderModule);
    REGISTER_MODULE(proba::hrc::ProbaHRCDecoderModule);
    REGISTER_MODULE(proba::vegetation::ProbaVegetationDecoderModule);

    // ELEKTRO
    REGISTER_MODULE(elektro_arktika::RDASDecoderModule);
    REGISTER_MODULE(elektro_arktika::msugs::MSUGSDecoderModule);
    REGISTER_MODULE(elektro_arktika::TLMDemodModule);

    // Terra
    REGISTER_MODULE(terra::TerraDBDemodModule);
    REGISTER_MODULE(terra::TerraDBDecoderModule);

    // Falcon & Starship
    REGISTER_MODULE(spacex::SpaceXDecoderModule);
    REGISTER_MODULE(spacex::FalconDecoderModule);

    // SMAP
    REGISTER_MODULE(smap::SMAPSDecoderModule);

    // Oceansat
    REGISTER_MODULE(oceansat::Oceansat2DBDecoderModule);
    REGISTER_MODULE(oceansat::ocm::OceansatOCMDecoderModule);

    // Saral
    REGISTER_MODULE(saral::SaralDecoderModule);
    REGISTER_MODULE(saral::argos::SaralArgosDecoderModule);

    // GOES - GVAR
    REGISTER_MODULE(goes::gvar::GVARDecoderModule);
    REGISTER_MODULE(goes::gvar::GVARImageDecoderModule);

    // GOES - HRIT
    REGISTER_MODULE(goes::hrit::GOESHRITDecoderModule);
    REGISTER_MODULE(goes::hrit::GOESLRITDataDecoderModule);

    // FengYun2 - S-VISSR
    REGISTER_MODULE(fengyun_svissr::SVISSRDecoderModule);
    REGISTER_MODULE(fengyun_svissr::SVISSRImageDecoderModule);

    // Jason-3
    REGISTER_MODULE(jason3::Jason3DecoderModule);
    REGISTER_MODULE(jason3::poseidon::Jason3PoseidonDecoderModule);
    REGISTER_MODULE(jason3::amr2::Jason3AMR2DecoderModule);

    // Log them out
    logger->debug("Registered modules (" + std::to_string(modules_registry.size()) + ") : ");
    for (std::pair<const std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, std::map<std::string, std::string>)>> &it : modules_registry)
        logger->debug(" - " + it.first);
}