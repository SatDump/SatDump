#include "module_ccsds_ldpc_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/codings/randomization.h"
#include "common/utils.h"
#include "common/codings/rotation.h"

namespace ccsds
{
    CCSDSLDPCDecoderModule::CCSDSLDPCDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters),
          is_ccsds(parameters.count("ccsds") > 0 ? parameters["ccsds"].get<bool>() : true),

          d_constellation_str(parameters["constellation"].get<std::string>()),
          // d_iq_invert(parameters.count("iq_invert") > 0 ? parameters["iq_invert"].get<bool>() : false),

          d_derand(parameters.count("derandomize") > 0 ? parameters["derandomize"].get<bool>() : true),

          d_ldpc_rate_str(parameters["ldpc_rate"].get<std::string>()),
          d_ldpc_block_size(parameters.count("ldpc_block_size") > 0 ? parameters["ldpc_block_size"].get<int>() : 0),
          d_ldpc_iterations(parameters["ldpc_iterations"].get<int>()),

          d_internal_stream(parameters.count("internal_stream") > 0 ? parameters["internal_stream"].get<bool>() : false),
          d_cadu_size(parameters.count("internal_stream") > 0 ? parameters["internal_cadu_size"].get<int>() : 0),
          d_cadu_bytes(ceil(d_cadu_size / 8.0)) // If we can't use complete bytes, add one and padding
    {

        // Get constellation
        if (d_constellation_str == "bpsk")
            d_constellation = dsp::BPSK;
        else if (d_constellation_str == "qpsk")
            d_constellation = dsp::QPSK;
        else if (d_constellation_str == "oqpsk")
            d_constellation = dsp::OQPSK;
        else
            throw std::runtime_error("CCSDS LDPC Decoder : invalid constellation type!");

        // Parse LDPC settings
        d_ldpc_rate = codings::ldpc::ldpc_rate_from_string(d_ldpc_rate_str);

        ldpc_dec = std::make_unique<codings::ldpc::CCSDSLDPC>(d_ldpc_rate, d_ldpc_block_size);
        d_ldpc_simd = ldpc_dec->simd();

        if (d_ldpc_rate == codings::ldpc::RATE_7_8)
            d_ldpc_asm_size = 32;
        else
            d_ldpc_asm_size = 64;

        d_ldpc_frame_size = ldpc_dec->frame_length() + d_ldpc_asm_size;
        d_ldpc_codeword_size = ldpc_dec->frame_length();
        d_ldpc_data_size = ldpc_dec->data_length();

        correlator = std::make_unique<CorrelatorGeneric>(d_constellation,
                                                         d_ldpc_rate == codings::ldpc::RATE_7_8 ? unsigned_to_bitvec<uint32_t>(0x1acffc1d)
                                                                                                : unsigned_to_bitvec<uint64_t>(0x034776c7272895b0),
                                                         d_ldpc_frame_size);

        logger->trace("LDPC Frame size {:d}, SIMD {:d}", d_ldpc_frame_size, d_ldpc_simd);

        // Parse internal sync marker if set
        if (d_internal_stream)
        {
            uint32_t asm_sync = 0x1acffc1d;
            if (parameters.count("internal_asm") > 0)
                asm_sync = std::stoul(parameters["internal_asm"].get<std::string>(), nullptr, 16);

            deframer = std::make_unique<deframing::BPSK_CCSDS_Deframer>(d_cadu_size, asm_sync);

            if (d_cadu_size % 8 != 0) // If this is not a perfect byte length match, pad the frames
            {
                deframer->CADU_PADDING = d_cadu_size % 8;
                logger->info("Frames will be padded!");
            }
        }

        soft_buffer = new int8_t[d_ldpc_frame_size];
        frames_in_ldpc_buffer = 0;
        ldpc_input_buffer = new int8_t[(d_ldpc_frame_size - d_ldpc_asm_size) * d_ldpc_simd];
        ldpc_output_buffer = new uint8_t[(d_ldpc_frame_size - d_ldpc_asm_size) * d_ldpc_simd];
        deframer_buffer = new uint8_t[d_ldpc_frame_size * 64];

        is_started = true;
    }

    std::vector<ModuleDataType> CCSDSLDPCDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> CCSDSLDPCDecoderModule::getOutputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    CCSDSLDPCDecoderModule::~CCSDSLDPCDecoderModule()
    {
        delete[] soft_buffer;
        delete[] deframer_buffer;
        delete[] ldpc_input_buffer;
        delete[] ldpc_output_buffer;
    }

    void CCSDSLDPCDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;

        std::string extension = is_ccsds ? ".cadu" : ".frm";

        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        if (output_data_type == DATA_FILE)
            data_out = std::ofstream(d_output_file_hint + extension, std::ios::binary);
        d_output_files.push_back(d_output_file_hint + extension);

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + extension);

        time_t lastTime = 0;

        phase_t phase;
        bool swap;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, d_ldpc_frame_size);
            else
                input_fifo->read((uint8_t *)soft_buffer, d_ldpc_frame_size);

            // if (d_iq_invert)
            // rotate_soft((int8_t *)soft_buffer, d_ldpc_frame_size, PHASE_0, true);

            int pos = correlator->correlate((int8_t *)soft_buffer, phase, swap, correlator_cor, d_ldpc_frame_size);

            correlator_locked = pos == 0; // Update locking state

            if (pos != 0 && pos < d_ldpc_frame_size) // Safety
            {
                memmove(soft_buffer, &soft_buffer[pos], d_ldpc_frame_size - pos);

                if (input_data_type == DATA_FILE)
                    data_in.read((char *)&soft_buffer[d_ldpc_frame_size - pos], pos);
                else
                    input_fifo->read((uint8_t *)&soft_buffer[d_ldpc_frame_size - pos], pos);
            }

            // Correct phase ambiguity
            if (d_constellation == dsp::OQPSK)
            {
                rotate_soft((int8_t *)soft_buffer, d_ldpc_frame_size, phase, false);

                if (swap)
                {
                    int8_t last_q_oqpsk = 0;
                    for (int i = (d_ldpc_frame_size / 2) - 1; i >= 0; i--)
                    {
                        int8_t back = soft_buffer[i * 2 + 1];
                        soft_buffer[i * 2 + 1] = last_q_oqpsk;
                        last_q_oqpsk = back;
                    }
                }
            }
            else
            {
                rotate_soft((int8_t *)soft_buffer, d_ldpc_frame_size, phase, swap);
            }

            // Derand
            if (d_derand)
                derand_ccsds_soft(&soft_buffer[d_ldpc_asm_size], d_ldpc_codeword_size - d_ldpc_asm_size);

            // LDPC Decoding
            memcpy(&ldpc_input_buffer[frames_in_ldpc_buffer * d_ldpc_codeword_size], &soft_buffer[d_ldpc_asm_size], d_ldpc_codeword_size);
            frames_in_ldpc_buffer++;

            if (frames_in_ldpc_buffer == d_ldpc_simd)
            {
#if 1 // For debug if necessary
                ldpc_corr = ldpc_dec->decode(ldpc_input_buffer, ldpc_output_buffer, d_ldpc_iterations);
#else
                for (int i = 0; i < d_ldpc_simd * d_ldpc_codeword_size; i++)
                    ldpc_output_buffer[i] = ldpc_input_buffer[i] > 0;
#endif

                if (d_internal_stream)
                {
                    for (int i = 0; i < d_ldpc_simd; i++)
                    {
                        // Deframe
                        int frames = deframer->work(&ldpc_output_buffer[i * d_ldpc_codeword_size], d_ldpc_data_size, deframer_buffer);
                        for (int i = 0; i < frames; i++)
                            data_out.write((char *)&deframer_buffer[i * d_cadu_bytes], d_cadu_bytes);
                    }
                }
                else
                {
                    // Repack
                    for (int i = 0; i < d_ldpc_simd * d_ldpc_codeword_size; i++)
                        deframer_buffer[i / 8] = deframer_buffer[i / 8] << 1 | ldpc_output_buffer[i];

                    for (int i = 0; i < d_ldpc_simd; i++)
                    {
                        // Write directly
                        if (d_ldpc_asm_size == 32)
                        {
                            const uint32_t sync = 0x1acffc1d;
                            if (output_data_type == DATA_FILE)
                                data_out.write((char *)&sync, 4);
                            else
                                output_fifo->write((uint8_t *)&sync, 4);
                        }
                        else if (d_ldpc_asm_size == 64)
                        {
                            const uint64_t sync = 0x034776c7272895b0;
                            if (output_data_type == DATA_FILE)
                                data_out.write((char *)&sync, 8);
                            else
                                output_fifo->write((uint8_t *)&sync, 8);
                        }

                        if (output_data_type == DATA_FILE)
                            data_out.write((char *)&deframer_buffer[i * (d_ldpc_codeword_size / 8)], (d_ldpc_frame_size - d_ldpc_asm_size) / 8);
                        else
                            output_fifo->write((uint8_t *)&deframer_buffer[i * (d_ldpc_codeword_size / 8)], (d_ldpc_frame_size - d_ldpc_asm_size) / 8);
                    }
                }

                frames_in_ldpc_buffer = 0;
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            // Update module stats
            if (d_internal_stream)
                module_stats["deframer_lock"] = deframer->getState() == deframer->STATE_SYNCED;
            module_stats["correlator_lock"] = correlator_locked;
            module_stats["correlator_corr"] = correlator_cor;
            module_stats["ldpc_corr"] = ldpc_corr;

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string lock_state = correlator_locked ? "SYNCED" : "NOSYNC";
                std::string deframer_state;
                if (d_internal_stream)
                    deframer_state = deframer->getState() == deframer->STATE_NOSYNC ? "NOSYNC" : (deframer->getState() == deframer->STATE_SYNCING ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Lock : " + lock_state + (d_internal_stream ? (", Deframer : " + deframer_state) : ""));
            }
        }

        if (output_data_type == DATA_FILE)
            data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void CCSDSLDPCDecoderModule::drawUI(bool window)
    {
        if (!is_started)
            return;

        ImGui::Begin("CCSDS LDPC Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        if (!streamingInput)
        {
            // Constellation
            if (d_constellation == dsp::BPSK)
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                         ImColor::HSV(0, 0, 0));

                for (int i = 0; i < 2048; i++)
                {
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i] / 127.0) * 130 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 14 * ui_scale) % int(200 * ui_scale)),
                                               2 * ui_scale,
                                               ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                }

                ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
            }
            else
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
            ImGui::Button("Correlator", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Corr  : ");
                ImGui::SameLine();
                ImGui::TextColored(correlator_locked ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(correlator_cor));

                std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                cor_history[200 - 1] = correlator_cor;

                if (d_ldpc_asm_size == 32)
                    ImGui::PlotLines("", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 15.0f, 35.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                else
                    ImGui::PlotLines("", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 25.0f, 70.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Button("LDPC", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Diff  : ");
                ImGui::SameLine();
                ImGui::TextColored(ldpc_corr > 10 ? IMCOLOR_SYNCING : IMCOLOR_SYNCED, UITO_C_STR(ldpc_corr));

                std::memmove(&ldpc_history[0], &ldpc_history[1], (200 - 1) * sizeof(float));
                ldpc_history[200 - 1] = ldpc_corr;

                ImGui::PlotLines("", ldpc_history, IM_ARRAYSIZE(ldpc_history), 0, "", 0.0f, d_ldpc_codeword_size / 20, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            if (d_internal_stream)
            {
                ImGui::Spacing();

                ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("State : ");

                    ImGui::SameLine();

                    if (deframer->getState() == deframer->STATE_NOSYNC)
                        ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                    else if (deframer->getState() == deframer->STATE_SYNCING)
                        ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                    else
                        ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");
                }
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string CCSDSLDPCDecoderModule::getID()
    {
        return "ccsds_ldpc_decoder";
    }

    std::vector<std::string> CCSDSLDPCDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> CCSDSLDPCDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<CCSDSLDPCDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace npp
