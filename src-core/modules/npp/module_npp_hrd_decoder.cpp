#include "module_npp_hrd_decoder.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"
#include "common/codings/randomization.h"

#define FRAME_SIZE 1024
#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace npp
{
    NPPHRDDecoderModule::NPPHRDDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                d_viterbi_outsync_after(parameters["viterbi_outsync_after"].get<int>()),
                                                                                                                                d_viterbi_ber_threasold(parameters["viterbi_ber_thresold"].get<float>()),
                                                                                                                                deframer(FRAME_SIZE * 8),
                                                                                                                                viterbi(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE, {PHASE_0, PHASE_90})
    {
        viterbi_out = new uint8_t[BUFFER_SIZE * 2];
        soft_buffer = new int8_t[BUFFER_SIZE];
    }

    std::vector<ModuleDataType> NPPHRDDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> NPPHRDDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    NPPHRDDecoderModule::~NPPHRDDecoderModule()
    {
        delete[] viterbi_out;
        delete[] soft_buffer;
    }

    void NPPHRDDecoderModule::process()
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

        diff::NRZMDiff diff;

        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        int vout;
        uint8_t frame_buffer[FRAME_SIZE * 10];

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)soft_buffer, BUFFER_SIZE);

            int vitout = viterbi.work((int8_t *)soft_buffer, BUFFER_SIZE, viterbi_out);

            // Diff decoding
            diff.decode_bits(viterbi_out, vitout);

            int frames = deframer.work(viterbi_out, vitout, frame_buffer);

            for (int i = 0; i < frames; i++)
            {
                uint8_t *cadu = &frame_buffer[i * FRAME_SIZE];

                // Derand
                derand_ccsds(&cadu[4], FRAME_SIZE - 4);

                // RS Correction
                rs.decode_interlaved(&cadu[4], true, 4, errors);

                // Write it out
                data_out.write((char *)cadu, FRAME_SIZE);
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string viterbi_state = viterbi.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi : " + viterbi_state + " BER : " + std::to_string(viterbi.ber()) + ", Deframer : " + deframer_state);
            }
        }

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void NPPHRDDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("NPP HRD Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);
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
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
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

    std::string NPPHRDDecoderModule::getID()
    {
        return "npp_hrd_decoder";
    }

    std::vector<std::string> NPPHRDDecoderModule::getParameters()
    {
        return {"viterbi_outsync_after", "viterbi_ber_thresold"};
    }

    std::shared_ptr<ProcessingModule> NPPHRDDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<NPPHRDDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace npp