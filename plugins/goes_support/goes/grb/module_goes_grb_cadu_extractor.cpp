#include "module_goes_grb_cadu_extractor.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"

#define BBFRAME_SIZE (58192 / 8) // BBFrame size
#define CADU_SIZE 2048           // CADU Size

// Return filesize
size_t getFilesize(std::string filepath);

namespace goes
{
    namespace grb
    {
        // This header is static, and while that is NOT the best way, using it as a syncword is very effective
        const uint8_t BBFRAME_HEADER[] = {0x71, 0x00, 0x00, 0x00, 0xe3, 0x00, 0x00, 0xff, 0xff, 0xb9};
        const uint8_t CADU_HEADER[] = {0x1a, 0xcf, 0xfc, 0x1d};

        GOESGRBCADUextractor::GOESGRBCADUextractor(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
            bb_buffer = new uint8_t[BBFRAME_SIZE];
            cadu_buffer = new uint8_t[CADU_SIZE];
        }

        std::vector<ModuleDataType> GOESGRBCADUextractor::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> GOESGRBCADUextractor::getOutputTypes()
        {
            return {DATA_FILE};
        }

        GOESGRBCADUextractor::~GOESGRBCADUextractor()
        {
            delete[] bb_buffer;
            delete[] cadu_buffer;
        }

        void GOESGRBCADUextractor::process()
        {
            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);
            data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".cadu");

            logger->info("Using input bbframes " + d_input_file);
            logger->info("Decoding to " + d_output_file_hint + ".cadu");

            time_t lastTime = 0;

            std::vector<uint8_t> caduVector;

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)bb_buffer, BBFRAME_SIZE);
                else
                    input_fifo->read((uint8_t *)bb_buffer, BBFRAME_SIZE);

                // Correlate over the entire frame
                int best_pos = 0;
                int best_cor = 0;
                for (int i = 0; i < BBFRAME_SIZE; i++)
                {
                    int correlation = 0;
                    for (int y = 0; y < (int)sizeof(BBFRAME_HEADER); y++)
                        correlation += (bb_buffer[i + y] == BBFRAME_HEADER[y]);

                    if (correlation > best_cor)
                    {
                        best_cor = correlation;
                        best_pos = i;
                    }

                    if (correlation == sizeof(BBFRAME_HEADER)) // If we have a perfect match, exit
                        break;
                }

                // Feed in more data
                if (best_pos != 0 && best_pos < BBFRAME_SIZE)
                {
                    std::memmove(bb_buffer, &bb_buffer[best_pos], best_pos);

                    if (input_data_type == DATA_FILE)
                        data_in.read((char *)&bb_buffer[BBFRAME_SIZE - best_pos], best_pos);
                    else
                        input_fifo->read((uint8_t *)&bb_buffer[BBFRAME_SIZE - best_pos], best_pos);
                }

                bb_cor = best_cor;
                bb_sync = best_pos == 0;

                // Buffer it
                caduVector.insert(caduVector.end(), &bb_buffer[10], &bb_buffer[BBFRAME_SIZE]);

                // Correlate against bits again, but this time CADUs
                while (caduVector.size() >= CADU_SIZE * 2)
                {
                    memcpy(cadu_buffer, &caduVector[0], CADU_SIZE);
                    caduVector.erase(caduVector.begin(), caduVector.begin() + CADU_SIZE);

                    // Correlate over the entire frame
                    int best_pos = 0;
                    int best_cor = 0;
                    for (int i = 0; i < CADU_SIZE; i++)
                    {
                        int correlation = 0;
                        for (int y = 0; y < (int)sizeof(CADU_HEADER); y++)
                            correlation += (cadu_buffer[i + y] == CADU_HEADER[y]);

                        if (correlation > best_cor)
                        {
                            best_cor = correlation;
                            best_pos = i;
                        }

                        if (correlation == sizeof(CADU_HEADER)) // If we have a perfect match, exit
                            break;
                    }

                    cadu_cor = best_cor;
                    cadu_sync = best_pos == 0;

                    // Feed in more data
                    if (best_pos != 0 && best_pos < CADU_SIZE)
                    {
                        std::memmove(cadu_buffer, &cadu_buffer[best_pos], best_pos);

                        memcpy(&cadu_buffer[BBFRAME_SIZE - best_pos], &caduVector[0], CADU_SIZE);
                        caduVector.erase(caduVector.begin(), caduVector.begin() + best_pos);
                    }

                    data_out.write((char *)cadu_buffer, CADU_SIZE);
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    std::string lock_state_bb = bb_sync ? "SYNCED" : "NOSYNC";
                    std::string lock_state_ca = cadu_sync ? "SYNCED" : "NOSYNC";
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, BBFrame : " + lock_state_bb + ", CADU : " + lock_state_ca);
                }
            }

            data_out.close();
            if (output_data_type == DATA_FILE)
                data_in.close();
        }

        void GOESGRBCADUextractor::drawUI(bool window)
        {
            ImGui::Begin("GOES GRB CADU Extractor", NULL, window ? NULL : NOWINDOW_FLAGS);
            ImGui::BeginGroup();
            {
                ImGui::Button("BBFrame Correlator", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("Corr  : ");
                    ImGui::SameLine();
                    ImGui::TextColored(bb_sync ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(bb_cor));

                    std::memmove(&cor_history_bb[0], &cor_history_bb[1], (200 - 1) * sizeof(float));
                    cor_history_bb[200 - 1] = bb_cor;

                    ImGui::PlotLines("##bbcor", cor_history_bb, IM_ARRAYSIZE(cor_history_bb), 0, "", 40.0f, 10.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                }

                ImGui::Spacing();

                ImGui::Button("CADU Correlator", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("Corr  : ");
                    ImGui::SameLine();
                    ImGui::TextColored(cadu_sync ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(cadu_cor));

                    std::memmove(&cor_history_ca[0], &cor_history_ca[1], (200 - 1) * sizeof(float));
                    cor_history_ca[200 - 1] = cadu_cor;

                    ImGui::PlotLines("##caducor", cor_history_ca, IM_ARRAYSIZE(cor_history_ca), 0, "", 40.0f, 4.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                }
            }
            ImGui::EndGroup();

            if (!streamingInput)
                ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GOESGRBCADUextractor::getID()
        {
            return "goes_grb_cadu_extractor";
        }

        std::vector<std::string> GOESGRBCADUextractor::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> GOESGRBCADUextractor::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESGRBCADUextractor>(input_file, output_file_hint, parameters);
        }
    } // namespace aqua
}