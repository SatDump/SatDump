#include "module_proba_s_decoder.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/rotation.h"
#include "common/codings/randomization.h"
#include "imgui/imgui.h"
#include <cmath>
#include "common/codings/correlator.h"
#include "common/codings/reedsolomon/reedsolomon.h"

#define FRAME_SIZE 1279
#define ENCODED_FRAME_SIZE 1279 * 8 * 2

// Return filesize
size_t getFilesize(std::string filepath);

namespace proba
{
    ProbaSDecoderModule::ProbaSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                derandomize(parameters["derandomize"].get<bool>()),
                                                                                                                                viterbi(ENCODED_FRAME_SIZE / 2, viterbi::CCSDS_R2_K7_POLYS)
    {
        buffer = new uint8_t[ENCODED_FRAME_SIZE];
    }

    std::vector<ModuleDataType> ProbaSDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> ProbaSDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    ProbaSDecoderModule::~ProbaSDecoderModule()
    {
        delete[] buffer;
    }

    void ProbaSDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        // Correlator
        Correlator correlator(BPSK, 0xa9f7e368e558c2c1);

        // Viterbi, rs, etc
        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        // Other buffers
        uint8_t frameBuffer[FRAME_SIZE];

        phase_t phase;
        bool swap;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)buffer, ENCODED_FRAME_SIZE);
            else
                input_fifo->read((uint8_t *)buffer, ENCODED_FRAME_SIZE);

            int pos = correlator.correlate((int8_t *)buffer, phase, swap, cor, ENCODED_FRAME_SIZE);

            locked = pos == 0; // Update locking state

            if (pos != 0 && pos < ENCODED_FRAME_SIZE) // Safety
            {
                std::memmove(buffer, &buffer[pos], pos);

                if (input_data_type == DATA_FILE)
                    data_in.read((char *)&buffer[ENCODED_FRAME_SIZE - pos], pos);
                else
                    input_fifo->read((uint8_t *)&buffer[ENCODED_FRAME_SIZE - pos], pos);
            }

            // Correct phase ambiguity
            // It is using inverted vectors in the Viterbi.... Hence that weird crap
            rotate_soft((int8_t *)buffer, ENCODED_FRAME_SIZE, phase ? PHASE_90 : PHASE_270, true);

            // Viterbi
            viterbi.work((int8_t *)buffer, frameBuffer);

            if (derandomize)
            {
                // Derandomize that frame
                derand_ccsds(&frameBuffer[4], FRAME_SIZE - 4);
            }

            // RS Correction
            rs.decode_interlaved(&frameBuffer[4], true, 5, errors);

            // Write it out
            if (cor > 50 || pos == 0)
            {
                data_out.put(0x1a);
                data_out.put(0xcf);
                data_out.put(0xfc);
                data_out.put(0x1d);
                data_out.write((char *)&frameBuffer[4], FRAME_SIZE - 4);
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string lock_state = locked ? "SYNCED" : "NOSYNC";
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi BER : " + std::to_string(viterbi.ber() * 100) + "%, Lock : " + lock_state);
            }
        }

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void ProbaSDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Proba S-Band Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

        float ber = viterbi.ber();

        ImGui::BeginGroup();
        {
            // Constellation
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                         ImColor::HSV(0, 0, 0));

                for (int i = 0; i < 2048; i++)
                {
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)buffer)[i] / 127.0) * 130 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 14 * ui_scale) % int(200 * ui_scale)),
                                               2 * ui_scale,
                                               ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                }

                ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
            }
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Correlator", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Corr  : ");
                ImGui::SameLine();
                ImGui::TextColored(locked ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(cor));

                std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                cor_history[200 - 1] = cor;

                ImGui::PlotLines("", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 40.0f, 64.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Spacing();

            ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(ber < 0.22 ? IMCOLOR_SYNCED : IMCOLOR_NOSYNC, UITO_C_STR(ber));

                std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                ber_history[200 - 1] = ber;

                ImGui::PlotLines("", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Spacing();

            ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("RS    : ");
                for (int i = 0; i < 5; i++)
                {
                    ImGui::SameLine();

                    if (errors[i] == -1)
                        ImGui::TextColored(IMCOLOR_NOSYNC, "%i ", i);
                    else if (errors[i] > 0)
                        ImGui::TextColored(IMCOLOR_SYNCING, "%i ", i);
                    else
                        ImGui::TextColored(IMCOLOR_SYNCED, "%i ", i);
                }
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string ProbaSDecoderModule::getID()
    {
        return "proba_s_decoder";
    }

    std::vector<std::string> ProbaSDecoderModule::getParameters()
    {
        return {"derandomize"};
    }

    std::shared_ptr<ProcessingModule> ProbaSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<ProbaSDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace proba