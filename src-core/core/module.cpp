#define SATDUMP_DLL_EXPORT 1
#include "core/module.h"
#include "logger.h"
#include "plugin.h"

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
#include "modules/demod/module_sdpsk_demod.h"
#include "modules/demod/module_xfsk_burst_demod.h"

#include "modules/network/module_network_server.h"
#include "modules/network/module_network_client.h"

#include "modules/xrit/module_goesrecv_publisher.h"
#include "modules/xrit/module_s2udp_xrit_cadu_extractor.h"

#include "modules/ccsds/module_ccsds_conv_concat_decoder.h"
#include "modules/ccsds/module_ccsds_simple_psk_decoder.h"
#include "modules/ccsds/module_ccsds_ldpc_decoder.h"
#include "modules/ccsds/module_ccsds_turbo_decoder.h"

#include "modules/products/module_products_processor.h"

#include "modules/generic/module_soft2hard.h"

void registerModules()
{
    // Register modules

    // Demods
    REGISTER_MODULE(demod::FSKDemodModule);
    REGISTER_MODULE(demod::PMDemodModule);
    REGISTER_MODULE(demod::PSKDemodModule);
    REGISTER_MODULE(demod::SDPSKDemodModule);
    REGISTER_MODULE(demod::XFSKBurstDemodModule);

    // Network
    REGISTER_MODULE(network::NetworkServerModule);
    REGISTER_MODULE(network::NetworkClientModule);

    // xRIT
    REGISTER_MODULE(xrit::GOESRecvPublisherModule);
    REGISTER_MODULE(xrit::S2UDPxRITCADUextractor);

    // CCSDS
    REGISTER_MODULE(ccsds::CCSDSConvConcatDecoderModule);
    REGISTER_MODULE(ccsds::CCSDSSimplePSKDecoderModule);
    REGISTER_MODULE(ccsds::CCSDSLDPCDecoderModule);
    REGISTER_MODULE(ccsds::CCSDSTurboDecoderModule);

    // Products Processor. This one is a bit different!
    REGISTER_MODULE(products::ProductsProcessorModule);

    // Generic
    REGISTER_MODULE(generic::Soft2HardModule);

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
        ImGui::TextColored(style::theme.yellow, "Decoding...");
    else if (status == PROCESSING)
        ImGui::TextColored(style::theme.magenta, "Processing...");
    else if (status == SAVING)
        ImGui::TextColored(style::theme.light_green, "Saving...");
    else if (status == DONE)
        ImGui::TextColored(style::theme.green, "Done");
    else
        ImGui::TextColored(style::theme.red, "Invalid!");
};