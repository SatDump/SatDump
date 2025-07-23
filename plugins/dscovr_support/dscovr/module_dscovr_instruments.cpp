#include "module_dscovr_instruments.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace dscovr
{
    namespace instruments
    {
        DSCOVRInstrumentsDecoderModule::DSCOVRInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void DSCOVRInstrumentsDecoderModule::process()
        {
            std::string epic_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/EPIC";

            if (!std::filesystem::exists(epic_directory))
                std::filesystem::create_directory(epic_directory);

            epic_reader.directory = epic_directory;

            uint8_t cadu[1264];

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1264);

                // Parse this transport frame
                ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

                uint8_t *payload = &cadu[18];

                if (vcdu.vcid == 2)
                {
                    epic_reader.work(payload);
                }
            }

            cleanup();

#if 0
            // CIPS
            {
                for (int i = 0; i < 4; i++)
                {
                    cips_status[i] = SAVING;

                    logger->info("----------- CIPS %d", i + 1);
                    logger->info("Images : %d", cips_readers[i].images.size());

                    std::string cips_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CIPS-" + std::to_string(i + 1);

                    if (!std::filesystem::exists(cips_directory))
                        std::filesystem::create_directory(cips_directory);

                    int img_cnt = 0;
                    for (auto &img : cips_readers[i].images)
                    {
                        img.save_img(cips_directory + "/CIPS_" + std::to_string(img_cnt++ + 1));
                    }

                    cips_status[i] = DONE;
                }
            }
#endif

            // dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void DSCOVRInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("DSCOVR Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##dscovrinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Images / Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                for (int i = 0; i < 4; i++)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("EPIC %d", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", epic_reader.img_c);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(epic_status);
                }

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string DSCOVRInstrumentsDecoderModule::getID() { return "dscovr_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> DSCOVRInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<DSCOVRInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace dscovr