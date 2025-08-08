#include "module_elektro_lrit_data_decoder.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "xrit/processor/xrit_channel_processor_render.h"
#include "xrit/transport/xrit_demux.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace elektro
{
    namespace lrit
    {
        ELEKTROLRITDataDecoderModule::ELEKTROLRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
            processor.directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IMAGES";
        }

        ELEKTROLRITDataDecoderModule::~ELEKTROLRITDataDecoderModule() {}

        void ELEKTROLRITDataDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            this->directory = directory;

            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            satdump::xrit::XRITDemux lrit_demux;

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1024);

                std::vector<satdump::xrit::XRITFile> files = lrit_demux.work(cadu);

                for (auto &file : files)
                    processLRITFile(file);
            }

            cleanup();

            processor.flush();
        }

        void ELEKTROLRITDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("ELEKTRO-L LRIT Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            satdump::xrit::renderAllTabsFromProcessors({&processor});

            drawProgressBar();

            ImGui::End();
        }

        std::string ELEKTROLRITDataDecoderModule::getID() { return "elektro_lrit_data_decoder"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> ELEKTROLRITDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<ELEKTROLRITDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace lrit
} // namespace elektro