#define SATDUMP_DLL_EXPORT 1

#include "module.h"
#include "core/exception.h"
#include "core/plugin.h"
#include "logger.h"

///////////////////////////

#include "modules/demod/module_fsk_demod.h"
#include "modules/demod/module_pm_demod.h"
#include "modules/demod/module_psk_demod.h"
#include "modules/demod/module_sdpsk_demod.h"
#include "modules/demod/module_xfsk_burst_demod.h"

#include "modules/network/module_network_client.h"
#include "modules/network/module_network_server.h"

#include "modules/xrit/module_goesrecv_publisher.h"
#include "modules/xrit/module_s2udp_xrit_cadu_extractor.h"

#include "modules/ccsds/module_ccsds_conv_concat_decoder.h"
#include "modules/ccsds/module_ccsds_ldpc_decoder.h"
#include "modules/ccsds/module_ccsds_simple_psk_decoder.h"
#include "modules/ccsds/module_ccsds_turbo_decoder.h"

#include "modules/products/module_products_processor.h"

#include "modules/generic/module_soft2hard.h"

namespace satdump
{
    namespace pipeline
    {
        ProcessingModule::ProcessingModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : d_parameters(parameters), d_input_file(input_file), d_output_file_hint(output_file_hint)
        {
            input_active = false;
            d_is_streaming_input = false;
        }

        std::string ProcessingModule::getOutput() { return d_output_file; }

        void ProcessingModule::setInputType(ModuleDataType type)
        {
            input_data_type = type;
            d_is_streaming_input = type != DATA_FILE;

            bool found = false;
            for (auto &t : getInputTypes())
                if (t == type)
                    found = true;
            if (!found)
                throw satdump_exception("Module input type not supported!");
        }

        void ProcessingModule::setOutputType(ModuleDataType type)
        {
            output_data_type = type;

            bool found = false;
            for (auto &t : getOutputTypes())
                if (t == type)
                    found = true;
            if (!found)
                throw satdump_exception("Module output type not supported!");
        }

        ModuleDataType ProcessingModule::getInputType() { return input_data_type; }

        ModuleDataType ProcessingModule::getOutputType() { return output_data_type; }

        void ProcessingModule::init() {}

        void ProcessingModule::stop() {}

        void ProcessingModule::drawUI(bool /*window*/) {}

        /////////////////////////////////// Registry

        SATDUMP_DLL std::vector<ModuleEntry> modules_registry;

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
            logger->debug("Registered modules TODOREWORK (" + std::to_string(modules_registry.size()) + ") : ");
            for (auto &it : modules_registry)
                logger->debug(" - " + it.id);
        }

        std::shared_ptr<ProcessingModule> getModuleInstance(std::string id, std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            for (auto &m : modules_registry)
                if (m.id == id)
                    return m.inst(input_file, output_file_hint, parameters);
            throw satdump_exception("Could not find module " + id);
        }

        bool moduleExists(std::string id)
        {
            for (auto &m : modules_registry)
                if (m.id == id)
                    return true;
            return false;
        }
    } // namespace pipeline
} // namespace satdump