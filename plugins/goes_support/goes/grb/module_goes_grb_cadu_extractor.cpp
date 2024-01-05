#include "module_goes_grb_cadu_extractor.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"

#define BBFRAME_SIZE (58192 / 8) // BBFrame size
#define CADU_SIZE 2048           // CADU Size

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace goes
{
    namespace grb
    {
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
            return {DATA_FILE, DATA_STREAM};
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
            if (output_data_type == DATA_FILE)
            {
                data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
                d_output_files.push_back(d_output_file_hint + ".cadu");
            }

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

                // data_out.write((char *)&bb_buffer[10], BBFRAME_SIZE - 10);

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
                    for (int i = 0; i < CADU_SIZE - (int)sizeof(CADU_HEADER); i++)
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
                        std::memmove(cadu_buffer, &cadu_buffer[best_pos], CADU_SIZE - best_pos);

                        memcpy(&cadu_buffer[CADU_SIZE - best_pos], &caduVector[0], best_pos);
                        caduVector.erase(caduVector.begin(), caduVector.begin() + best_pos);
                    }

                    module_stats["correlation"] = best_cor;
                    module_stats["synced"] = cadu_sync;

                    if (output_data_type == DATA_FILE)
                        data_out.write((char *)cadu_buffer, CADU_SIZE);
                    else
                        output_fifo->write((uint8_t *)cadu_buffer, CADU_SIZE);
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    std::string lock_state_ca = cadu_sync ? "SYNCED" : "NOSYNC";
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, CADU : " + lock_state_ca);
                }
            }

            if (output_data_type == DATA_FILE)
                data_out.close();
            if (input_data_type == DATA_FILE)
                data_in.close();
        }

        void GOESGRBCADUextractor::drawUI(bool window)
        {
            ImGui::Begin("GOES GRB CADU Extractor", NULL, window ? 0 : NOWINDOW_FLAGS);
            ImGui::BeginGroup();
            {
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
                ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

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