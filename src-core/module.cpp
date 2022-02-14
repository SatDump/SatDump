#define SATDUMP_DLL_EXPORT 1
#include "module.h"
#include "logger.h"
#include "plugin.h"

SATDUMP_DLL float ui_scale = 1;                 // UI Scaling factor, set to 1 (no scaling) by default
SATDUMP_DLL int demod_constellation_size = 200; // Demodulator constellation size

ProcessingModule::ProcessingModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : d_input_file(input_file),
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
SATDUMP_DLL std::map<std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, nlohmann::json)>> modules_registry;

#include "modules/module_qpsk_demod.h"
#include "modules/module_oqpsk_demod.h"
#include "modules/module_bpsk_demod.h"
#include "modules/module_fsk_demod.h"
#include "modules/module_8psk_demod.h"
#include "modules/module_pm_demod.h"

#include "modules/metop/module_metop_ahrpt_decoder.h"
#include "modules/metop/module_metop_dump_decoder.h"
#include "modules/metop/instruments/avhrr/module_metop_avhrr.h"
#include "modules/metop/instruments/mhs/module_metop_mhs.h"
#include "modules/metop/instruments/iasi/module_metop_iasi.h"
#include "modules/metop/instruments/amsu/module_metop_amsu.h"
#include "modules/metop/instruments/gome/module_metop_gome.h"
#include "modules/metop/instruments/ascat/module_metop_ascat.h"
#include "modules/metop/instruments/admin_msg/module_metop_admin_msg.h"
#include "modules/metop/module_metop_satid.h"
#include "modules/metop/instruments/sem/module_metop_sem.h"

#include "modules/fengyun3/module_new_fengyun_ahrpt_decoder.h"
#include "modules/fengyun3/module_fengyun_ahrpt_decoder.h"
#include "modules/fengyun3/module_new_fengyun_mpt_decoder.h"
#include "modules/fengyun3/module_fengyun_mpt_decoder.h"
#include "modules/fengyun3/instruments/virr/module_fengyun_virr.h"
#include "modules/fengyun3/instruments/mersi1/module_fengyun_mersi1.h"
#include "modules/fengyun3/instruments/mersi2/module_fengyun_mersi2.h"
#include "modules/fengyun3/instruments/erm/module_fengyun_erm.h"
#include "modules/fengyun3/instruments/mwhs/module_fengyun_mwhs.h"
#include "modules/fengyun3/instruments/waai/module_fengyun_waai.h"
#include "modules/fengyun3/instruments/mwri/module_fengyun_mwri.h"
#include "modules/fengyun3/instruments/mwts/module_fengyun_mwts.h"
#include "modules/fengyun3/instruments/mwhs2/module_fengyun_mwhs2.h"
#include "modules/fengyun3/module_fengyun_satid.h"
#include "modules/fengyun3/instruments/mersill/module_fengyun_mersill.h"
#include "modules/fengyun3/instruments/windrad/module_fengyun_windrad.h"
#include "modules/fengyun3/instruments/mwts2/module_fengyun_mwts2.h"
#include "modules/fengyun3/instruments/mwts3/module_fengyun_mwts3.h"
#include "modules/fengyun3/instruments/xeuvi/module_fengyun_xeuvi.h"

#include "modules/aqua/module_aqua_db_decoder.h"
#include "modules/aqua/instruments/airs/module_aqua_airs.h"
#include "modules/aqua/instruments/amsu/module_aqua_amsu.h"
#include "modules/aqua/instruments/ceres/module_aqua_ceres.h"

#include "modules/aura/instruments/omi/module_aura_omi.h"

#include "modules/eos/instruments/modis/module_eos_modis.h"

#include "modules/noaa/module_noaa_hrpt_decoder.h"
#include "modules/noaa/module_noaa_gac_decoder.h"
#include "modules/noaa/module_noaa_dsb_decoder.h"
#include "modules/noaa/instruments/avhrr/module_noaa_avhrr.h"
#include "modules/noaa/module_noaa_extractor.h"
#include "modules/noaa/instruments/hirs/module_noaa_hirs.h"
#include "modules/noaa/instruments/mhs/module_noaa_mhs.h"
#include "modules/noaa/instruments/amsu/module_noaa_amsu.h"

#include "modules/meteor/module_meteor_hrpt_decoder.h"
#include "modules/meteor/module_meteor_lrpt_decoder.h"
#include "modules/meteor/instruments/msumr/module_meteor_msumr.h"
#include "modules/meteor/instruments/msumr/module_meteor_msumr_lrpt.h"
#include "modules/meteor/instruments/mtvza/module_meteor_mtvza.h"

#include "modules/jpss/instruments/atms/module_jpss_atms.h"
#include "modules/jpss/instruments/viirs/module_jpss_viirs.h"
#include "modules/jpss/instruments/omps/module_jpss_omps.h"
#include "modules/jpss/module_jpss_satid.h"

#include "modules/proba/instruments/swap/module_proba_swap.h"
#include "modules/proba/instruments/chris/module_proba_chris.h"
#include "modules/proba/instruments/hrc/module_proba_hrc.h"
#include "modules/proba/instruments/vegetation/module_proba_vegetation.h"

#include "modules/elektro_arktika/instruments/msugs/module_msugs.h"
#include "modules/elektro_arktika/module_tlm_demod.h"
#include "modules/elektro_arktika/lrit/module_elektro_lrit_data_decoder.h"

#include "modules/terra/module_terra_db_demod.h"

#include "modules/spacex/module_spacex_decoder.h"
#include "modules/spacex/module_falcon_decoder.h"

#include "modules/oceansat/module_oceansat2_db_decoder.h"
#include "modules/oceansat/instruments/ocm/module_oceansat_ocm.h"

#include "modules/saral/argos/module_saral_argos.h"

#include "modules/goes/gvar/module_gvar_decoder.h"
#include "modules/goes/gvar/module_gvar_image_decoder.h"
#include "modules/goes/hrit/module_goes_lrit_data_decoder.h"
#include "modules/goes/grb/module_goes_grb_cadu_extractor.h"
#include "modules/goes/grb/module_goes_grb_data_decoder.h"

#include "modules/xrit/module_goesrecv_publisher.h"

#include "modules/fengyun2/svissr/module_svissr_decoder.h"
#include "modules/fengyun2/svissr/module_svissr_image_decoder.h"

#include "modules/jason3/instruments/poseidon/module_jason3_poseidon.h"
#include "modules/jason3/instruments/amr2/module_jason3_amr2.h"
#include "modules/jason3/instruments/lpt/module_jason3_lpt.h"

#include "modules/angels/argos/module_angels_argos.h"

#include "modules/ccsds/module_ccsds_analyzer.h"
#include "modules/ccsds/module_ccsds_conv_r2_concat_decoder.h"
#include "modules/ccsds/module_ccsds_simple_psk_decoder.h"

#include "modules/gk2a/module_gk2a_lrit_data_decoder.h"

#include "modules/cryosat/instruments/siral/module_cryosat_siral.h"

#include "modules/coriolis/instruments/windsat/module_coriolis_windsat.h"

#include "modules/cloudsat/instruments/cpr/module_cloudsat_cpr.h"

//#include "modules/iris/module_iris_dump_decoder.h"

void registerModules()
{
    // Register modules

    // Demods
    REGISTER_MODULE(QPSKDemodModule);
    REGISTER_MODULE(OQPSKDemodModule);
    REGISTER_MODULE(BPSKDemodModule);
    REGISTER_MODULE(FSKDemodModule);
    REGISTER_MODULE(PSK8DemodModule);
    REGISTER_MODULE(PMDemodModule);

    // MetOp
    REGISTER_MODULE(metop::MetOpAHRPTDecoderModule);
    REGISTER_MODULE(metop::MetOpDumpDecoderModule);
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
    REGISTER_MODULE(fengyun3::NewFengyunAHRPTDecoderModule);
    REGISTER_MODULE(fengyun3::FengyunAHRPTDecoderModule);
    REGISTER_MODULE(fengyun3::NewFengyunMPTDecoderModule);
    REGISTER_MODULE(fengyun3::FengyunMPTDecoderModule);
    REGISTER_MODULE(fengyun3::virr::FengyunVIRRDecoderModule);
    REGISTER_MODULE(fengyun3::mersi1::FengyunMERSI1DecoderModule);
    REGISTER_MODULE(fengyun3::mersi2::FengyunMERSI2DecoderModule);
    REGISTER_MODULE(fengyun3::erm::FengyunERMDecoderModule);
    REGISTER_MODULE(fengyun3::mwhs::FengyunMWHSDecoderModule);
    REGISTER_MODULE(fengyun3::waai::FengyunWAAIDecoderModule);
    REGISTER_MODULE(fengyun3::mwri::FengyunMWRIDecoderModule);
    REGISTER_MODULE(fengyun3::mwts::FengyunMWTSDecoderModule);
    REGISTER_MODULE(fengyun3::mwhs2::FengyunMWHS2DecoderModule);
    REGISTER_MODULE(fengyun3::satid::FengYunSatIDModule);
    REGISTER_MODULE(fengyun3::mersill::FengyunMERSILLDecoderModule);
    REGISTER_MODULE(fengyun3::windrad::FengyunWindRADDecoderModule);
    REGISTER_MODULE(fengyun3::mwts2::FengyunMWTS2DecoderModule);
    REGISTER_MODULE(fengyun3::mwts3::FengyunMWTS3DecoderModule);
    REGISTER_MODULE(fengyun3::xeuvi::FengyunXEUVIDecoderModule);

    // Aqua
    REGISTER_MODULE(aqua::AquaDBDecoderModule);
    REGISTER_MODULE(aqua::airs::AquaAIRSDecoderModule);
    REGISTER_MODULE(aqua::amsu::AquaAMSUDecoderModule);
    REGISTER_MODULE(aqua::ceres::AquaCERESDecoderModule);

    // Aura
    REGISTER_MODULE(aura::omi::AuraOMIDecoderModule);

    // EOS
    REGISTER_MODULE(eos::modis::EOSMODISDecoderModule);

    // NOAA
    REGISTER_MODULE(noaa::NOAAHRPTDecoderModule);
    REGISTER_MODULE(noaa::NOAAGACDecoderModule);
    REGISTER_MODULE(noaa::NOAADSBDecoderModule);
    REGISTER_MODULE(noaa::avhrr::NOAAAVHRRDecoderModule);
    REGISTER_MODULE(noaa::NOAAExtractorModule);
    REGISTER_MODULE(noaa::hirs::NOAAHIRSDecoderModule);
    REGISTER_MODULE(noaa::mhs::NOAAMHSDecoderModule);
    REGISTER_MODULE(noaa::amsu::NOAAAMSUDecoderModule);

    // METEOR
    REGISTER_MODULE(meteor::METEORHRPTDecoderModule);
    REGISTER_MODULE(meteor::METEORLRPTDecoderModule);
    REGISTER_MODULE(meteor::msumr::METEORMSUMRDecoderModule);
    REGISTER_MODULE(meteor::msumr::METEORMSUMRLRPTDecoderModule);
    REGISTER_MODULE(meteor::mtvza::METEORMTVZADecoderModule);

    // JPSS
    REGISTER_MODULE(jpss::satid::JPSSSatIDModule);
    REGISTER_MODULE(jpss::atms::JPSSATMSDecoderModule);
    REGISTER_MODULE(jpss::viirs::JPSSVIIRSDecoderModule);
    REGISTER_MODULE(jpss::omps::JPSSOMPSDecoderModule);

    // Proba
    REGISTER_MODULE(proba::swap::ProbaSWAPDecoderModule);
    REGISTER_MODULE(proba::chris::ProbaCHRISDecoderModule);
    REGISTER_MODULE(proba::hrc::ProbaHRCDecoderModule);
    REGISTER_MODULE(proba::vegetation::ProbaVegetationDecoderModule);

    // ELEKTRO
    REGISTER_MODULE(elektro_arktika::msugs::MSUGSDecoderModule);
    REGISTER_MODULE(elektro_arktika::TLMDemodModule);
    REGISTER_MODULE(elektro::lrit::ELEKTROLRITDataDecoderModule);
    // REGISTER_MODULE(elektro_arktika::ggak_e::GGAKEDecoderModule);

    // Terra
    REGISTER_MODULE(terra::TerraDBDemodModule);

    // Falcon & Starship
    REGISTER_MODULE(spacex::SpaceXDecoderModule);
    REGISTER_MODULE(spacex::FalconDecoderModule);

    // Oceansat
    REGISTER_MODULE(oceansat::Oceansat2DBDecoderModule);
    REGISTER_MODULE(oceansat::ocm::OceansatOCMDecoderModule);

    // Saral
    REGISTER_MODULE(saral::argos::SaralArgosDecoderModule);

    // xRIT
    REGISTER_MODULE(xrit::GOESRecvPublisherModule);

    // GOES - GVAR / HRIT / GRB
    REGISTER_MODULE(goes::gvar::GVARDecoderModule);
    REGISTER_MODULE(goes::gvar::GVARImageDecoderModule);
    REGISTER_MODULE(goes::hrit::GOESLRITDataDecoderModule);
    REGISTER_MODULE(goes::grb::GOESGRBCADUextractor);
    REGISTER_MODULE(goes::grb::GOESGRBDataDecoderModule);

    // FengYun2 - S-VISSR / LRIT
    REGISTER_MODULE(fengyun_svissr::SVISSRDecoderModule);
    REGISTER_MODULE(fengyun_svissr::SVISSRImageDecoderModule);

    // Jason-3
    REGISTER_MODULE(jason3::poseidon::Jason3PoseidonDecoderModule);
    REGISTER_MODULE(jason3::amr2::Jason3AMR2DecoderModule);
    REGISTER_MODULE(jason3::lpt::Jason3LPTDecoderModule);

    // ANGELS
    REGISTER_MODULE(angels::argos::AngelsArgosDecoderModule);

    // CCSDS
    REGISTER_MODULE(ccsds::analyzer::CCSDSAnalyzerModule);
    REGISTER_MODULE(ccsds::CCSDSConvR2ConcatDecoderModule);
    REGISTER_MODULE(ccsds::CCSDSSimplePSKDecoderModule);

    // GK-2A
    REGISTER_MODULE(gk2a::lrit::GK2ALRITDataDecoderModule);

    // CFOSAT

    // CRYOSAT
    REGISTER_MODULE(cryosat::siral::CryoSatSIRALDecoderModule);

    // Coriolis
    REGISTER_MODULE(coriolis::windsat::CoriolisWindSatDecoderModule);

    // CloudSat
    REGISTER_MODULE(cloudsat::cpr::CloudSatCPRDecoderModule);

    // IRIS
    // REGISTER_MODULE(iris::IRISDumpDecoderModule);

    // CHEOPS
    // REGISTER_MODULE(cheops::telescope::CHEOPSTelescopeDecoderModule);

    // Plugin modules
    satdump::eventBus->fire_event<RegisterModulesEvent>({modules_registry});

    // Log them out
    logger->debug("Registered modules (" + std::to_string(modules_registry.size()) + ") : ");
    for (std::pair<const std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, nlohmann::json)>> &it : modules_registry)
        logger->debug(" - " + it.first);
}