#define SATDUMP_DLL_EXPORT 1
#include "core/module.h"
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

#include "modules/demod/module_fsk_demod.h"
#include "modules/demod/module_pm_demod.h"
#include "modules/demod/module_psk_demod.h"
#include "modules/demod/module_dvbs2_demod.h"

#include "modules/xrit/module_goesrecv_publisher.h"

#include "modules/ccsds/module_ccsds_conv_r2_concat_decoder.h"
#include "modules/ccsds/module_ccsds_simple_psk_decoder.h"

void registerModules()
{
    // Register modules

    // Demods
    REGISTER_MODULE(demod::FSKDemodModule);
    REGISTER_MODULE(demod::PMDemodModule);
    REGISTER_MODULE(demod::PSKDemodModule);

    REGISTER_MODULE(demod::DVBS2DemodModule);

    // xRIT
    REGISTER_MODULE(xrit::GOESRecvPublisherModule);

    // CCSDS
    REGISTER_MODULE(ccsds::CCSDSConvR2ConcatDecoderModule);
    REGISTER_MODULE(ccsds::CCSDSSimplePSKDecoderModule);

    // Plugin modules
    satdump::eventBus->fire_event<RegisterModulesEvent>({modules_registry});

    // Log them out
    logger->debug("Registered modules (" + std::to_string(modules_registry.size()) + ") : ");
    for (std::pair<const std::string, std::function<std::shared_ptr<ProcessingModule>(std::string, std::string, nlohmann::json)>> &it : modules_registry)
        logger->debug(" - " + it.first);
}

void drawStatus(instrument_status_t status)
{
    if (status == DECODING)
        ImGui::TextColored(ImColor(255, 255, 0), "Decoding...");
    else if (status == PROCESSING)
        ImGui::TextColored(ImColor(200, 0, 255), "Processing...");
    else if (status == SAVING)
        ImGui::TextColored(ImColor(100, 255, 100), "Saving...");
    else if (status == DONE)
        ImGui::TextColored(ImColor(0, 255, 0), "Done");
    else
        ImGui::TextColored(ImColor(255, 0, 0), "Invalid!");
};