#include "module.h"
#include "logger.h"

ProcessingModule::ProcessingModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : d_input_file(input_file),
                                                                                                                                          d_output_file_hint(output_file_hint),
                                                                                                                                          d_parameters(parameters)
{
}

std::vector<std::string> ProcessingModule::getOutputs()
{
    return d_output_files;
}

// Registry
std::map<std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, std::map<std::string, std::string>)>> modules_registry;

#include "modules/module_qpsk_demod.h"
#include "modules/module_oqpsk_demod.h"
#include "modules/module_bpsk_demod.h"

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

#include "modules/noaa/module_noaa_hrpt_demod.h"
#include "modules/noaa/instruments/avhrr/module_noaa_avhrr.h"

#include "modules/meteor/module_meteor_hrpt_demod.h"
#include "modules/meteor/module_meteor_hrpt_decoder.h"
#include "modules/meteor/instruments/msumr/module_meteor_msumr.h"

#include "modules/npp/module_npp_hrd_decoder.h"

#include "modules/jpss/instruments/atms/module_jpss_atms.h"
#include "modules/jpss/instruments/viirs/module_jpss_viirs.h"

#include "modules/proba/module_proba_s_decoder.h"

void registerModules()
{
    // Register modules
    modules_registry.emplace(QPSKDemodModule::getID(), QPSKDemodModule::getInstance);
    modules_registry.emplace(OQPSKDemodModule::getID(), OQPSKDemodModule::getInstance);
    modules_registry.emplace(BPSKDemodModule::getID(), BPSKDemodModule::getInstance);

    modules_registry.emplace(metop::MetOpAHRPTDecoderModule::getID(), metop::MetOpAHRPTDecoderModule::getInstance);
    modules_registry.emplace(metop::avhrr::MetOpAVHRRDecoderModule::getID(), metop::avhrr::MetOpAVHRRDecoderModule::getInstance);
    modules_registry.emplace(metop::mhs::MetOpMHSDecoderModule::getID(), metop::mhs::MetOpMHSDecoderModule::getInstance);
    modules_registry.emplace(metop::iasi::MetOpIASIDecoderModule::getID(), metop::iasi::MetOpIASIDecoderModule::getInstance);
    modules_registry.emplace(metop::amsu::MetOpAMSUDecoderModule::getID(), metop::amsu::MetOpAMSUDecoderModule::getInstance);
    modules_registry.emplace(metop::gome::MetOpGOMEDecoderModule::getID(), metop::gome::MetOpGOMEDecoderModule::getInstance);

    modules_registry.emplace(fengyun::FengyunAHRPTDecoderModule::getID(), fengyun::FengyunAHRPTDecoderModule::getInstance);
    modules_registry.emplace(fengyun::FengyunMPTDecoderModule::getID(), fengyun::FengyunMPTDecoderModule::getInstance);
    modules_registry.emplace(fengyun::virr::FengyunVIRRDecoderModule::getID(), fengyun::virr::FengyunVIRRDecoderModule::getInstance);
    modules_registry.emplace(fengyun::mersi1::FengyunMERSI1DecoderModule::getID(), fengyun::mersi1::FengyunMERSI1DecoderModule::getInstance);
    modules_registry.emplace(fengyun::mersi2::FengyunMERSI2DecoderModule::getID(), fengyun::mersi2::FengyunMERSI2DecoderModule::getInstance);

    modules_registry.emplace(aqua::AquaDBDecoderModule::getID(), aqua::AquaDBDecoderModule::getInstance);
    modules_registry.emplace(aqua::airs::AquaAIRSDecoderModule::getID(), aqua::airs::AquaAIRSDecoderModule::getInstance);
    modules_registry.emplace(aqua::amsu::AquaAMSUDecoderModule::getID(), aqua::amsu::AquaAMSUDecoderModule::getInstance);
    modules_registry.emplace(aqua::ceres::AquaCERESDecoderModule::getID(), aqua::ceres::AquaCERESDecoderModule::getInstance);

    modules_registry.emplace(eos::modis::EOSMODISDecoderModule::getID(), eos::modis::EOSMODISDecoderModule::getInstance);

    modules_registry.emplace(noaa::NOAAHRPTDemodModule::getID(), noaa::NOAAHRPTDemodModule::getInstance);
    modules_registry.emplace(noaa::avhrr::NOAAAVHRRDecoderModule::getID(), noaa::avhrr::NOAAAVHRRDecoderModule::getInstance);

    modules_registry.emplace(meteor::METEORHRPTDemodModule::getID(), meteor::METEORHRPTDemodModule::getInstance);
    modules_registry.emplace(meteor::METEORHRPTDecoderModule::getID(), meteor::METEORHRPTDecoderModule::getInstance);
    modules_registry.emplace(meteor::msumr::METEORMSUMRDecoderModule::getID(), meteor::msumr::METEORMSUMRDecoderModule::getInstance);

    modules_registry.emplace(npp::NPPHRDDecoderModule::getID(), npp::NPPHRDDecoderModule::getInstance);

    modules_registry.emplace(jpss::atms::JPSSATMSDecoderModule::getID(), jpss::atms::JPSSATMSDecoderModule::getInstance);
    modules_registry.emplace(jpss::viirs::JPSSVIIRSDecoderModule::getID(), jpss::viirs::JPSSVIIRSDecoderModule::getInstance);

    modules_registry.emplace(proba::ProbaSDecoderModule::getID(), proba::ProbaSDecoderModule::getInstance);

    // Log them out
    logger->debug("Registered modules (" + std::to_string(modules_registry.size()) + ") : ");
    for (std::pair<const std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, std::map<std::string, std::string>)>> &it : modules_registry)
        logger->debug(" - " + it.first);
}