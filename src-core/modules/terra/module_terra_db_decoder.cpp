#include "module_terra_db_decoder.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/rotation.h"
#include "common/codings/randomization.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"

#define FRAME_SIZE 1024
#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace terra
{
    TerraDBDecoderModule::TerraDBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                  deframer(FRAME_SIZE * 8),
                                                                                                                                  viterbi(0.3, 20, BUFFER_SIZE, {PHASE_90})
    {
        buffer = new uint8_t[BUFFER_SIZE];
    }

    std::vector<ModuleDataType> TerraDBDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> TerraDBDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    TerraDBDecoderModule::~TerraDBDecoderModule()
    {
        delete[] buffer;
    }

    void TerraDBDecoderModule::process()
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

        // Viterbi, rs, etc
        diff::NRZMDiff diff;
        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        // Other buffers
        uint8_t frame_buffer[FRAME_SIZE * 10];
        uint8_t viterbi_out_buffer[BUFFER_SIZE];

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)buffer, BUFFER_SIZE);

            rotate_soft((int8_t *)buffer, BUFFER_SIZE, PHASE_0, true); // Symbols are swapped

            int vitout = viterbi.work((int8_t *)buffer, BUFFER_SIZE, viterbi_out_buffer);

            // Diff decoding
            diff.decode_bits(viterbi_out_buffer, vitout);

            int frames = deframer.work(viterbi_out_buffer, vitout, frame_buffer);

            for (int i = 0; i < frames; i++)
            {
                uint8_t *cadu = &frame_buffer[i * FRAME_SIZE];

                // RS Correction
                rs.decode_interlaved(&cadu[4], true, 4, errors);

                // Derand
                derand_ccsds(&cadu[10], 886);

                // Write it out
                data_out.write((char *)cadu, FRAME_SIZE);
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string viterbi_state = viterbi.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string deframer_state = deframer.getState() == deframer.STATE_NOSYNC ? "NOSYNC" : (deframer.getState() == deframer.STATE_SYNCING == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi : " + viterbi_state + " BER : " + std::to_string(viterbi.ber()) + ", Deframer : " + deframer_state);
            }
        }

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void TerraDBDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Terra DB Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

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
            ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi.getState() == 0)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi.getState() == 0 ? IMCOLOR_NOSYNC : IMCOLOR_SYNCED, UITO_C_STR(ber));

                std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                ber_history[200 - 1] = ber;

                ImGui::PlotLines("", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Spacing();

            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (deframer.getState() == deframer.STATE_NOSYNC)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else if (deframer.getState() == deframer.STATE_SYNCING)
                    ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");
            }

            ImGui::Spacing();

            ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("RS    : ");
                for (int i = 0; i < 4; i++)
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

    std::string TerraDBDecoderModule::getID()
    {
        return "terra_db_decoder";
    }

    std::vector<std::string> TerraDBDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> TerraDBDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<TerraDBDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor