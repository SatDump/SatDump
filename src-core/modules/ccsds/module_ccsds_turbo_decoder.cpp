#include "module_ccsds_turbo_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/codings/rotation.h"
#include "common/codings/randomization.h"
#include "common/codings/turbo/ccsds_turbo.h"
#include "common/codings/crc/crc_generic.h"

namespace ccsds
{
    CCSDSTurboDecoderModule::CCSDSTurboDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters)
    {
        if (parameters.contains("turbo_rate"))
            d_turbo_rate = parameters["turbo_rate"].get<std::string>();
        else
            throw std::runtime_error("Turbo Rate is required!");

        if (parameters.contains("turbo_base"))
            d_turbo_base = parameters["turbo_base"].get<int>();
        else
            throw std::runtime_error("Turbo Base is required!");

        if (parameters.contains("turbo_iters"))
            d_turbo_iters = parameters["turbo_iters"].get<int>();
        else
            throw std::runtime_error("Turbo Iters is required!");

        if (!(d_turbo_base == 223 || d_turbo_base == 446 || d_turbo_base == 892 || d_turbo_base == 1115))
            throw std::runtime_error("Turbo Base must be 223, 446, 892 or 1115!");

        if (d_turbo_rate == "1/2")
        {
            // Generate ASM softs. 0x034776C7272895B0
            uint8_t asm_bits[] = {0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1,
                                  0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1,
                                  0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1,
                                  0, 0, 0, 0};
            for (int i = 0; i < 64; i++)
                asm_softs[i] = asm_bits[i] ? 1.0f : -1.0f;

            d_asm_size = 64;

            if (d_turbo_base == 223)
                d_codeword_size = 3576;
            else if (d_turbo_base == 446)
                d_codeword_size = 7144;
            else if (d_turbo_base == 892)
                d_codeword_size = 14280;
            else if (d_turbo_base == 1115)
                d_codeword_size = 17848;
        }
        else if (d_turbo_rate == "1/3")
        {
            // Generate ASM softs. 0x25D5C0CE8990F6C9461BF79C
            uint8_t asm_bits[] = {0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0,
                                  0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1,
                                  1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0,
                                  1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1,
                                  1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0};
            for (int i = 0; i < 96; i++)
                asm_softs[i] = asm_bits[i] ? 1.0f : -1.0f;

            d_asm_size = 96;

            if (d_turbo_base == 223)
                d_codeword_size = 5364;
            else if (d_turbo_base == 446)
                d_codeword_size = 10716;
            else if (d_turbo_base == 892)
                d_codeword_size = 21420;
            else if (d_turbo_base == 1115)
                d_codeword_size = 26772;
        }
        else if (d_turbo_rate == "1/4")
        {
            // Generate ASM softs. 0x034776C7272895B0FCB88938D8D76A4F
            uint8_t asm_bits[] = {0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1,
                                  0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1,
                                  0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1,
                                  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0,
                                  1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1,
                                  1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0,
                                  0, 1, 0, 0, 1, 1, 1, 1};
            for (int i = 0; i < 128; i++)
                asm_softs[i] = asm_bits[i] ? 1.0f : -1.0f;

            d_asm_size = 128;

            if (d_turbo_base == 223)
                d_codeword_size = 7152;
            else if (d_turbo_base == 446)
                d_codeword_size = 14288;
            else if (d_turbo_base == 892)
                d_codeword_size = 28560;
            else if (d_turbo_base == 1115)
                d_codeword_size = 35696;
        }

        else if (d_turbo_rate == "1/6")
        {
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
                asm_softs[i] = asm_bits[i] ? 1.0f : -1.0f;

            d_asm_size = 192;

            if (d_turbo_base == 223)
                d_codeword_size = 10728;
            else if (d_turbo_base == 446)
                d_codeword_size = 21432;
            else if (d_turbo_base == 892)
                d_codeword_size = 42840;
            else if (d_turbo_base == 1115)
                d_codeword_size = 53544;
        }
        else
            throw std::runtime_error("Invalid Turbo Rate!");

        d_frame_size = d_codeword_size + d_asm_size;

        buffer_soft = new int8_t[d_frame_size];
        buffer_floats = new float[d_frame_size];

        window_name = "CCSDS Turbo r=" + d_turbo_rate + " b=" + std::to_string(d_turbo_base) + " Decoder";
    }

    std::vector<ModuleDataType> CCSDSTurboDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> CCSDSTurboDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    CCSDSTurboDecoderModule::~CCSDSTurboDecoderModule()
    {
        delete[] buffer_soft;
        delete[] buffer_floats;
    }

    void CCSDSTurboDecoderModule::process()
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

        codings::turbo::turbo_rate_t frate = codings::turbo::RATE_1_2;

        if (d_turbo_rate == "1/2")
            frate = codings::turbo::RATE_1_2;
        else if (d_turbo_rate == "1/3")
            frate = codings::turbo::RATE_1_3;
        else if (d_turbo_rate == "1/4")
            frate = codings::turbo::RATE_1_4;
        else if (d_turbo_rate == "1/6")
            frate = codings::turbo::RATE_1_6;

        codings::turbo::CCSDSTurbo turbo_dec((codings::turbo::turbo_base_t)d_turbo_base, frate);
        codings::crc::GenericCRC crc_check(16, 0x1021, 0xFFFF, 0x0, false, false);
        uint8_t output_frame_buffer[1115 + 4];

        time_t lastTime = 0;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)buffer_soft, d_frame_size);
            else
                input_fifo->read((uint8_t *)buffer_soft, d_frame_size);
            volk_8i_s32f_convert_32f(buffer_floats, buffer_soft, 127, d_frame_size);

            // Correlate. Slow, but Turbo rate 1/6 shouldn't be used on fast links
            int best_pos = 0;
            float best_cor = 0;
            bool best_swapped = false;
            float corr_value = 0;
            for (int pos = 0; pos < d_frame_size - d_asm_size; pos++)
            {
                volk_32f_x2_dot_prod_32f(&corr_value, &buffer_floats[pos], asm_softs, d_asm_size);

                if (corr_value > best_cor)
                {
                    best_cor = corr_value;
                    best_pos = pos;
                    best_swapped = false;
                }

                if (-corr_value > best_cor)
                {
                    best_cor = -corr_value;
                    best_pos = pos;
                    best_swapped = true;
                }

                // logger->critical("%d %f %f", pos, corr_nwap, corr_swap);
            }

            locked = best_pos == 0; // Update locking state
            cor = best_cor;

            if (best_pos != 0 && best_pos < d_frame_size) // Safety
            {
                std::memmove(buffer_soft, &buffer_soft[best_pos], best_pos);

                if (input_data_type == DATA_FILE)
                    data_in.read((char *)&buffer_soft[d_frame_size - best_pos], best_pos);
                else
                    input_fifo->read((uint8_t *)&buffer_soft[d_frame_size - best_pos], best_pos);
            }

            derand_ccsds_soft(&buffer_soft[d_asm_size], d_codeword_size);
            volk_8i_s32f_convert_32f(buffer_floats, buffer_soft, 127.0, d_frame_size);

            // logger->critical(best_pos);

            if (best_swapped) // Swap if required
                for (int i = 0; i < d_frame_size; i++)
                    buffer_floats[i] = -buffer_floats[i];

            turbo_dec.decode(&buffer_floats[d_asm_size], &output_frame_buffer[4], d_turbo_iters);

            output_frame_buffer[0] = 0x1a;
            output_frame_buffer[1] = 0xcf;
            output_frame_buffer[2] = 0xfc;
            output_frame_buffer[3] = 0x1d;

            // Check CRC
            uint16_t compu_crc = crc_check.compute(&output_frame_buffer[4], d_turbo_base - 2);
            uint16_t local_crc = output_frame_buffer[(d_turbo_base + 4) - 2] << 8 | output_frame_buffer[(d_turbo_base + 4) - 1];

            crc_lock = compu_crc == local_crc;

            if (crc_lock)
                data_out.write((char *)output_frame_buffer, (d_turbo_base + 4));

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string lock_state = locked ? "SYNCED" : "NOSYNC";
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0f) / 10.0f) + "%%, Lock : " + lock_state);
            }
        }

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void CCSDSTurboDecoderModule::drawUI(bool window)
    {
        ImGui::Begin(window_name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        {
            ImGui::Button("Correlator", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Corr  : ");
                ImGui::SameLine();
                ImGui::TextColored(locked ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(cor));

                std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                cor_history[200 - 1] = cor;

                ImGui::PlotLines("", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 0.0f, 100.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
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
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string CCSDSTurboDecoderModule::getID()
    {
        return "ccsds_turbo_decoder";
    }

    std::vector<std::string> CCSDSTurboDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> CCSDSTurboDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<CCSDSTurboDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor