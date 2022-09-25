#include "module_ccsds_turbo_r6_k8920_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/codings/rotation.h"
#include "common/codings/randomization.h"
#include "common/codings/turbo/ccsds_turbo.h"
#include "common/codings/crc/crc_generic.h"

#define ENCODED_FRAME_SIZE 53736
#define ENCODED_ASM_SIZE 192

namespace ccsds
{
    CCSDSTurboR6K8920DecoderModule::CCSDSTurboR6K8920DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        buffer_soft = new int8_t[ENCODED_FRAME_SIZE];
        buffer_floats = new float[ENCODED_FRAME_SIZE];

        d_turbo_iters = parameters["turbo_iters"].get<int>();

        // Generate ASM softs. 0x25D5C0CE8990F6C9461BF79CDA2A3F31766F0936B9E40863
        uint8_t asm_bits[] = {0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0,
                              0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1,
                              1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0,
                              1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1,
                              1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1,
                              1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1,
                              0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0,
                              1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0,
                              1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0,
                              1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1};
        for (int i = 0; i < 192; i++)
            asm_softs[i] = asm_bits[i] ? 0.5f : -0.5f;
    }

    std::vector<ModuleDataType> CCSDSTurboR6K8920DecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> CCSDSTurboR6K8920DecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    CCSDSTurboR6K8920DecoderModule::~CCSDSTurboR6K8920DecoderModule()
    {
        delete[] buffer_soft;
        delete[] buffer_floats;
    }

    void CCSDSTurboR6K8920DecoderModule::process()
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

        codings::turbo::CCSDSTurbo turbo_dec(codings::turbo::BASE_1115, codings::turbo::RATE_1_6);
        codings::crc::GenericCRC crc_check(16, 0x1021, 0xFFFF, 0x0, false, false);
        uint8_t output_frame_buffer[1115 + 4];

        time_t lastTime = 0;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)buffer_soft, ENCODED_FRAME_SIZE);
            else
                input_fifo->read((uint8_t *)buffer_soft, ENCODED_FRAME_SIZE);
            volk_8i_s32f_convert_32f(buffer_floats, buffer_soft, 127, ENCODED_FRAME_SIZE);

            // Correlate. Slow, but Turbo rate 1/6 shouldn't be used on fast links
            int best_pos = 0;
            float best_cor = 0;
            bool best_swapped = false;
            for (int pos = 0; pos < ENCODED_FRAME_SIZE - ENCODED_ASM_SIZE; pos++)
            {
                float corr_nwap = 0, corr_swap = 0;
                for (int i = 0; i < 192; i++)
                {
                    corr_nwap += fabs(buffer_floats[pos + i] + asm_softs[i]);
                    corr_swap += fabs(buffer_floats[pos + i] - asm_softs[i]);
                }

                if (corr_nwap > best_cor)
                {
                    best_cor = corr_nwap;
                    best_pos = pos;
                    best_swapped = false;
                }

                if (corr_swap > best_cor)
                {
                    best_cor = corr_swap;
                    best_pos = pos;
                    best_swapped = true;
                }

                // logger->critical("{:d} {:f} {:f}", pos, corr_nwap, corr_swap);
            }

            locked = best_pos == 0; // Update locking state
            cor = best_cor;

            if (best_pos != 0 && best_pos < ENCODED_FRAME_SIZE) // Safety
            {
                std::memmove(buffer_soft, &buffer_soft[best_pos], best_pos);

                if (input_data_type == DATA_FILE)
                    data_in.read((char *)&buffer_soft[ENCODED_FRAME_SIZE - best_pos], best_pos);
                else
                    input_fifo->read((uint8_t *)&buffer_soft[ENCODED_FRAME_SIZE - best_pos], best_pos);
            }

            derand_ccsds_soft(&buffer_soft[192], 53544);
            volk_8i_s32f_convert_32f(buffer_floats, buffer_soft, 127.0, ENCODED_FRAME_SIZE);

            // logger->critical(best_pos);

            if (best_swapped) // Swap if required
                for (int i = 0; i < ENCODED_FRAME_SIZE; i++)
                    buffer_floats[i] = -buffer_floats[i];

            turbo_dec.decode(&buffer_floats[192], &output_frame_buffer[4], 10);

            output_frame_buffer[0] = 0x1a;
            output_frame_buffer[1] = 0xcf;
            output_frame_buffer[2] = 0xfc;
            output_frame_buffer[3] = 0x1d;

            // Check CRC
            uint16_t compu_crc = crc_check.compute(&output_frame_buffer[4], 1115 - 2);
            uint16_t local_crc = output_frame_buffer[1119 - 2] << 8 | output_frame_buffer[1119 - 1];

            crc_lock = compu_crc == local_crc;

            if (crc_lock)
                data_out.write((char *)output_frame_buffer, (1115 + 4));

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string lock_state = locked ? "SYNCED" : "NOSYNC";
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Lock : " + lock_state);
            }
        }

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void CCSDSTurboR6K8920DecoderModule::drawUI(bool window)
    {
        ImGui::Begin("CCSDS Turbo R6 K8920 Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        {
            ImGui::Button("Correlator", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Corr  : ");
                ImGui::SameLine();
                ImGui::TextColored(locked ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(cor));

                std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                cor_history[200 - 1] = cor;

                ImGui::PlotLines("", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 100.0f, 230.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Button("CRC Check", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Check  : ");
                ImGui::SameLine();
                ImGui::TextColored(crc_lock ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, crc_lock ? "PASS" : "FAIL");
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string CCSDSTurboR6K8920DecoderModule::getID()
    {
        return "ccsds_turbo_r6_k8920_decoder";
    }

    std::vector<std::string> CCSDSTurboR6K8920DecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> CCSDSTurboR6K8920DecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<CCSDSTurboR6K8920DecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor