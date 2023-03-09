#include "module_sd_image_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <filesystem>
#include "common/utils.h"
#include "common/repack.h"

#define FRAME_SIZE 60
#define FRAME_SIZE_WORDS 48

namespace goes
{
    namespace sd
    {
        SDImageDecoderModule::SDImageDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
            frame = new uint8_t[FRAME_SIZE];
            frame_words = new uint16_t[FRAME_SIZE_WORDS];
        }

        std::vector<ModuleDataType> SDImageDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> SDImageDecoderModule::getOutputTypes()
        {
            return {DATA_FILE};
        }

        SDImageDecoderModule::~SDImageDecoderModule()
        {
            delete[] frame;
            delete[] frame_words;
        }

        void SDImageDecoderModule::process()
        {
            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)frame, FRAME_SIZE);
                else
                    input_fifo->read((uint8_t *)frame, FRAME_SIZE);

                repackBytesTo10bits(frame, 60, frame_words);

                img_reader.work(frame_words);

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                // Update module stats
                //   module_stats["full_disk_progress"] = approx_progess;

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) +
                                 "%"); //, Full Disk Progress : " + std::to_string(round(((float)approx_progess / 100.0f) * 1000.0f) / 10.0f) + "%");
                }
            }

            if (input_data_type == DATA_FILE)
                data_in.close();

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Saving VIS...");
            img_reader.getChannel(0).save_png(directory + "/VIS.png");

            logger->info("Saving IR1...");
            img_reader.getChannel(1).save_png(directory + "/IR1.png");

            logger->info("Saving IR2...");
            img_reader.getChannel(2).save_png(directory + "/IR2.png");

            logger->info("Saving IR3...");
            img_reader.getChannel(3).save_png(directory + "/IR3.png");

            logger->info("Saving IR4...");
            img_reader.getChannel(4).save_png(directory + "/IR4.png");
        }

        void SDImageDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("SD Image Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

#if 0
            ImGui::SameLine();

            ImGui::BeginGroup();
            {
                ImGui::Button("Full Disk Progress", {200 * ui_scale, 20 * ui_scale});
                ImGui::ProgressBar((float)approx_progess / 100.0f, ImVec2(200 * ui_scale, 20 * ui_scale));
                ImGui::Text("State : ");
                ImGui::SameLine();
                if (isSavingInProgress)
                    ImGui::TextColored(IMCOLOR_SYNCED, "Writing images...");
                else if (isImageInProgress)
                    ImGui::TextColored(IMCOLOR_SYNCING, "Receiving...");
                else
                    ImGui::TextColored(IMCOLOR_NOSYNC, "IDLE");
            }
            ImGui::EndGroup();
#endif

            if (!streamingInput)
                ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string SDImageDecoderModule::getID()
        {
            return "goes_sd_image_decoder";
        }

        std::vector<std::string> SDImageDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> SDImageDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<SDImageDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace elektro
}