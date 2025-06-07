#include "module_sd_image_decoder.h"
#include "common/repack.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <filesystem>

#define FRAME_SIZE 60
#define FRAME_SIZE_WORDS 48

namespace goes
{
    namespace sd
    {
        SDImageDecoderModule::SDImageDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            frame = new uint8_t[FRAME_SIZE];
            frame_words = new uint16_t[FRAME_SIZE_WORDS];
            fsfsm_enable_output = false;
        }

        SDImageDecoderModule::~SDImageDecoderModule()
        {
            delete[] frame;
            delete[] frame_words;
        }

        void SDImageDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

            while (should_run())
            {
                read_data((uint8_t *)frame, FRAME_SIZE);

                repackBytesTo10bits(frame, 60, frame_words);

                img_reader.work(frame_words);

                img_reader.try_save(directory);
            }

            cleanup();

            img_reader.try_save(directory, true);
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
                    ImGui::TextColored(style::theme.green, "Writing images...");
                else if (isImageInProgress)
                    ImGui::TextColored(style::theme.orange, "Receiving...");
                else
                    ImGui::TextColored(style::theme.red, "IDLE");
            }
            ImGui::EndGroup();
#endif

            drawProgressBar();

            ImGui::End();
        }

        std::string SDImageDecoderModule::getID() { return "goes_sd_image_decoder"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> SDImageDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<SDImageDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace sd
} // namespace goes