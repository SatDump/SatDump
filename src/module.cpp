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

#include "modules/metop/module_metop_ahrpt_decoder.h"
#include "modules/metop/instruments/avhrr/module_metop_avhrr.h"
#include "modules/metop/instruments/mhs/module_metop_mhs.h"
#include "modules/metop/instruments/iasi/module_metop_iasi.h"
#include "modules/metop/instruments/amsu/module_metop_amsu.h"
#include "modules/metop/instruments/gome/module_metop_gome.h"

#include "modules/fengyun/module_fengyun_ahrpt_decoder.h"
#include "modules/fengyun/module_fengyun_mpt_decoder.h"
#include "modules/fengyun/instruments/virr/module_fengyun_virr.h"

#include "modules/aqua/module_aqua_db_decoder.h"
#include "modules/aqua/instruments/airs/module_aqua_airs.h"
#include "modules/aqua/instruments/amsu/module_aqua_amsu.h"

#include "modules/eos/instruments/modis/module_eos_modis.h"

#include "modules/noaa/module_noaa_hrpt_demod.h"
#include "modules/noaa/instruments/avhrr/module_noaa_avhrr.h"

void registerModules()
{
    // Register modules
    modules_registry.emplace(QPSKDemodModule::getID(), QPSKDemodModule::getInstance);
    modules_registry.emplace(OQPSKDemodModule::getID(), OQPSKDemodModule::getInstance);

    modules_registry.emplace(MetOpAHRPTDecoderModule::getID(), MetOpAHRPTDecoderModule::getInstance);
    modules_registry.emplace(MetOpAVHRRDecoderModule::getID(), MetOpAVHRRDecoderModule::getInstance);
    modules_registry.emplace(MetOpMHSDecoderModule::getID(), MetOpMHSDecoderModule::getInstance);
    modules_registry.emplace(MetOpIASIDecoderModule::getID(), MetOpIASIDecoderModule::getInstance);
    modules_registry.emplace(MetOpAMSUDecoderModule::getID(), MetOpAMSUDecoderModule::getInstance);
    modules_registry.emplace(MetOpGOMEDecoderModule::getID(), MetOpGOMEDecoderModule::getInstance);

    modules_registry.emplace(FengyunAHRPTDecoderModule::getID(), FengyunAHRPTDecoderModule::getInstance);
    modules_registry.emplace(FengyunMPTDecoderModule::getID(), FengyunMPTDecoderModule::getInstance);
    modules_registry.emplace(FengyunVIRRDecoderModule::getID(), FengyunVIRRDecoderModule::getInstance);

    modules_registry.emplace(AquaDBDecoderModule::getID(), AquaDBDecoderModule::getInstance);
    modules_registry.emplace(AquaAIRSDecoderModule::getID(), AquaAIRSDecoderModule::getInstance);
    modules_registry.emplace(AquaAMSUDecoderModule::getID(), AquaAMSUDecoderModule::getInstance);

    modules_registry.emplace(EOSMODISDecoderModule::getID(), EOSMODISDecoderModule::getInstance);

    modules_registry.emplace(NOAAHRPTDemodModule::getID(), NOAAHRPTDemodModule::getInstance);
    modules_registry.emplace(NOAAAVHRRDecoderModule::getID(), NOAAAVHRRDecoderModule::getInstance);

    // Log them out
    logger->debug("Registered modules (" + std::to_string(modules_registry.size()) + ") : ");
    for (std::pair<const std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, std::map<std::string, std::string>)>> &it : modules_registry)
        logger->debug(" - " + it.first);
}