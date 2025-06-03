#include "module_ccsds_simple_psk_decoder.h"
#include "common/codings/differential/nrzm.h"
#include "common/codings/differential/qpsk_diff.h"
#include "common/codings/randomization.h"
#include "common/codings/soft_reader.h"
#include "core/exception.h"
#include "imgui/imgui.h"
#include "logger.h"

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace satdump
{
    namespace pipeline
    {
        namespace ccsds
        {
            CCSDSSimplePSKDecoderModule::CCSDSSimplePSKDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
                : ProcessingModule(input_file, output_file_hint, parameters), is_ccsds(parameters.count("ccsds") > 0 ? parameters["ccsds"].get<bool>() : true),

                  d_constellation_str(parameters["constellation"].get<std::string>()),

                  d_cadu_size(parameters["cadu_size"].get<int>()), d_cadu_bytes(ceil(d_cadu_size / 8.0)), // If we can't use complete bytes, add one and padding
                  d_buffer_size(d_cadu_size),

                  d_qpsk_swapiq(parameters.count("qpsk_swap_iq") > 0 ? parameters["qpsk_swap_iq"].get<bool>() : false),
                  d_qpsk_swapdiff(parameters.count("qpsk_swap_diff") > 0 ? parameters["qpsk_swap_diff"].get<bool>() : true),
                  d_oqpsk_delay(parameters.count("oqpsk_delay") > 0 ? parameters["oqpsk_delay"].get<bool>() : false),
                  d_oqpsk_method_2(parameters.count("oqpsk_method2") > 0 ? parameters["oqpsk_method2"].get<bool>() : false),
                  d_oqpsk_method_3(parameters.count("oqpsk_method3") > 0 ? parameters["oqpsk_method3"].get<bool>() : false),

                  d_diff_decode(parameters.count("nrzm") > 0 ? parameters["nrzm"].get<bool>() : false),

                  d_derand(parameters.count("derandomize") > 0 ? parameters["derandomize"].get<bool>() : true),
                  d_derand_after_rs(parameters.count("derand_after_rs") > 0 ? parameters["derand_after_rs"].get<bool>() : false),
                  d_derand_from(parameters.count("derand_start") > 0 ? parameters["derand_start"].get<int>() : 4),

                  d_rs_interleaving_depth(parameters["rs_i"].get<int>()), d_rs_fill_bytes(parameters.count("rs_fill_bytes") > 0 ? parameters["rs_fill_bytes"].get<int>() : -1),
                  d_rs_dualbasis(parameters.count("rs_dualbasis") > 0 ? parameters["rs_dualbasis"].get<bool>() : true),
                  d_rs_type(parameters.count("rs_type") > 0 ? parameters["rs_type"].get<std::string>() : "none"),
                  d_rs_usecheck(parameters.count("rs_usecheck") > 0 ? parameters["rs_usecheck"].get<bool>() : false)
            {
                bits_out = new uint8_t[d_buffer_size * 2];
                soft_buffer = new int8_t[d_buffer_size];
                qpsk_diff_buffer = new uint8_t[d_cadu_size * 2];
                frame_buffer = new uint8_t[d_cadu_size * 2]; // Larger by safety

                // Get constellation
                if (d_constellation_str == "bpsk")
                    d_constellation = dsp::BPSK;
                else if (d_constellation_str == "qpsk")
                    d_constellation = dsp::QPSK;
                else
                    throw satdump_exception("CCSDS Simple PSK Decoder : invalid constellation type!");

                // Parse RS
                reedsolomon::RS_TYPE rstype = reedsolomon::RS223;
                if (d_rs_interleaving_depth != 0)
                {
                    if (d_rs_type == "rs223")
                        rstype = reedsolomon::RS223;
                    else if (d_rs_type == "rs239")
                        rstype = reedsolomon::RS239;
                    else
                        throw satdump_exception("CCSDS Simple PSK Decoder : invalid Reed-Solomon type!");
                }

                // Parse sync marker if set
                uint32_t asm_sync = 0x1acffc1d;
                if (parameters.count("asm") > 0)
                    asm_sync = std::stoul(parameters["asm"].get<std::string>(), nullptr, 16);

                deframer = std::make_shared<deframing::BPSK_CCSDS_Deframer>(d_cadu_size, asm_sync);
                deframer_qpsk = std::make_shared<deframing::BPSK_CCSDS_Deframer>(d_cadu_size, asm_sync); // For QPSK without NRZ-M which gets split into 2 BPSK deframers
                if (d_rs_interleaving_depth != 0)
                    reed_solomon = std::make_shared<reedsolomon::ReedSolomon>(rstype, d_rs_fill_bytes);

                if (d_cadu_size % 8 != 0) // If this is not a perfect byte length match, pad the frames
                {
                    int pad = d_cadu_size % 8;
                    deframer->CADU_PADDING = pad;
                    deframer_qpsk->CADU_PADDING = pad;
                    logger->info("Frames will be padded!");
                }

                if (d_constellation == dsp::QPSK && (d_oqpsk_method_2 || d_oqpsk_method_3))
                    soft_buffer2 = new int8_t[d_buffer_size];
            }

            std::vector<ModuleDataType> CCSDSSimplePSKDecoderModule::getInputTypes() { return {DATA_FILE, DATA_STREAM}; }

            std::vector<ModuleDataType> CCSDSSimplePSKDecoderModule::getOutputTypes() { return {DATA_FILE, DATA_STREAM}; }

            CCSDSSimplePSKDecoderModule::~CCSDSSimplePSKDecoderModule()
            {
                delete[] bits_out;
                delete[] soft_buffer;
                delete[] frame_buffer;
                delete[] qpsk_diff_buffer;
                if (d_constellation == dsp::QPSK && (d_oqpsk_method_2 || d_oqpsk_method_3))
                    delete[] soft_buffer2;
            }

            void CCSDSSimplePSKDecoderModule::process()
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
                d_output_file = d_output_file_hint + extension;

                logger->info("Using input symbols " + d_input_file);
                logger->info("Decoding to " + d_output_file_hint + extension);

                SoftSymbolReader soft_reader(data_in, input_fifo, input_data_type, d_parameters);

                diff::NRZMDiff diff;
                diff::QPSKDiff qpsk_diff;

                qpsk_diff.swap = d_qpsk_swapdiff;

                dsp::constellation_t qpsk_const(dsp::QPSK);

                int8_t last_oqpsk2 = 0;
                int8_t last_q_oqpsk = 0; // For delaying OQPSK

                while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
                {
                    int frames = 0;

                    // Read a buffer
                    soft_reader.readSoftSymbols(soft_buffer, d_buffer_size);

                    if (d_constellation == dsp::BPSK)
                    {
                        for (int i = 0; i < d_buffer_size; i++) // Convert BPSK to bits
                            bits_out[i] = soft_buffer[i] > 0;

                        if (d_diff_decode) // Diff decoding if required
                            diff.decode_bits(bits_out, d_buffer_size);
                    }
                    else if (d_constellation == dsp::QPSK)
                    {
                        // OQPSK Delay
                        if (d_oqpsk_delay)
                        {
                            for (int i = 0; i < d_buffer_size / 2; i++)
                            {
                                int8_t back = soft_buffer[i * 2 + 0];
                                soft_buffer[i * 2 + 0] = last_q_oqpsk;
                                last_q_oqpsk = back;
                            }
                        }

                        if (d_qpsk_swapiq) // Swap IQ if required
                            rotate_soft((int8_t *)soft_buffer, d_buffer_size, PHASE_0, true);

                        if (d_diff_decode) // Diff decoding if required
                        {
                            for (int i = 0; i < d_buffer_size / 2; i++) // Demod QPSK to bits
                                qpsk_diff_buffer[i] = qpsk_const.soft_demod(&soft_buffer[i * 2]);

                            // Perform differential decoding. For now only QPSK with diff coding is supported.
                            qpsk_diff.work(qpsk_diff_buffer, d_buffer_size / 2, bits_out);
                        }
                        else
                        {
                            if (!(d_oqpsk_method_2 || d_oqpsk_method_3)) // Normal QPSK case
                            {
                                // Run deframer on 0 deg const
                                for (int i = 0; i < d_buffer_size / 2; i++)
                                {
                                    uint8_t sym = qpsk_const.soft_demod(&soft_buffer[i * 2]);
                                    bits_out[i * 2 + 0] = sym >> 1;
                                    bits_out[i * 2 + 1] = sym & 1;
                                }

                                frames += deframer_qpsk->work(bits_out, d_buffer_size, &frame_buffer[frames * d_cadu_bytes]);

                                // Now shift by 90 degs and let the main one do its thing
                                rotate_soft((int8_t *)soft_buffer, d_buffer_size, PHASE_90, false);

                                for (int i = 0; i < d_buffer_size / 2; i++)
                                {
                                    uint8_t sym = qpsk_const.soft_demod(&soft_buffer[i * 2]);
                                    bits_out[i * 2 + 0] = sym >> 1;
                                    bits_out[i * 2 + 1] = sym & 1;
                                }
                            }
                            else if (!d_oqpsk_method_3) // Weird OQPSK case, where the demodulator can lock at an offset
                            {
                                memcpy(soft_buffer2, soft_buffer, d_buffer_size);

                                for (int i = 0; i < d_buffer_size / 2; i++)
                                {
                                    int8_t back = soft_buffer2[i * 2 + 0];
                                    soft_buffer2[i * 2 + 0] = last_oqpsk2;
                                    last_oqpsk2 = back;
                                }

                                // Run deframer on 0 deg const, shifted around
                                for (int i = 0; i < d_buffer_size / 2; i++)
                                {
                                    uint8_t sym = qpsk_const.soft_demod(&soft_buffer2[i * 2]);
                                    bits_out[i * 2 + 0] = sym >> 1;
                                    bits_out[i * 2 + 1] = sym & 1;
                                }

                                frames += deframer_qpsk->work(bits_out, d_buffer_size, &frame_buffer[frames * d_cadu_bytes]);

                                // Now shift by 90 degs and let the main one do its thing
                                rotate_soft((int8_t *)soft_buffer, d_buffer_size, PHASE_90, false);

                                for (int i = 0; i < d_buffer_size / 2; i++)
                                {
                                    uint8_t sym = qpsk_const.soft_demod(&soft_buffer[i * 2]);
                                    bits_out[i * 2 + 0] = sym >> 1;
                                    bits_out[i * 2 + 1] = sym & 1;
                                }
                            }
                            else // Weird OQPSK case, where the demodulator can lock at an offset
                            {
                                memcpy(soft_buffer2, soft_buffer, d_buffer_size);

                                for (int i = 0; i < d_buffer_size / 2; i++)
                                {
                                    int8_t back = soft_buffer2[i * 2 + 0];
                                    soft_buffer2[i * 2 + 0] = last_oqpsk2;
                                    last_oqpsk2 = back;
                                }

                                // Now shift by 90 degs and let the main one do its thing
                                rotate_soft((int8_t *)soft_buffer2, d_buffer_size, PHASE_90, false);

                                // Run deframer on 0 deg const, shifted around
                                for (int i = 0; i < d_buffer_size / 2; i++)
                                {
                                    uint8_t sym = qpsk_const.soft_demod(&soft_buffer2[i * 2]);
                                    bits_out[i * 2 + 0] = sym >> 1;
                                    bits_out[i * 2 + 1] = sym & 1;
                                }

                                frames += deframer_qpsk->work(bits_out, d_buffer_size, &frame_buffer[frames * d_cadu_bytes]);

                                for (int i = 0; i < d_buffer_size / 2; i++)
                                {
                                    uint8_t sym = qpsk_const.soft_demod(&soft_buffer[i * 2]);
                                    bits_out[i * 2 + 0] = sym >> 1;
                                    bits_out[i * 2 + 1] = sym & 1;
                                }
                            }
                        }
                    }

                    // Run deframer
                    frames += deframer->work(bits_out, d_buffer_size, &frame_buffer[frames * d_cadu_bytes]);

                    for (int i = 0; i < frames; i++)
                    {
                        uint8_t *cadu = &frame_buffer[i * d_cadu_bytes];

                        if (d_derand && !d_derand_after_rs) // Derand if required, before RS
                            derand_ccsds(&cadu[d_derand_from], d_cadu_bytes - d_derand_from);

                        if (d_rs_interleaving_depth != 0) // RS Correction
                            reed_solomon->decode_interlaved(&cadu[4], d_rs_dualbasis, d_rs_interleaving_depth, errors);

                        bool valid = true;
                        for (int i = 0; i < d_rs_interleaving_depth; i++)
                            if (errors[i] == -1)
                                valid = false;

                        if (d_derand && d_derand_after_rs) // Derand if required, after RS
                            derand_ccsds(&cadu[d_derand_from], d_cadu_bytes - d_derand_from);

                        if (!d_rs_usecheck || valid)
                        {
                            // Write it out
                            if (output_data_type == DATA_STREAM)
                                output_fifo->write((uint8_t *)cadu, d_cadu_bytes);
                            else
                                data_out.write((char *)cadu, d_cadu_bytes);
                        }
                    }

                    if (input_data_type == DATA_FILE)
                        progress = data_in.tellg();
                }

                if (output_data_type == DATA_FILE)
                    data_out.close();
                if (input_data_type == DATA_FILE)
                    data_in.close();
            }

            nlohmann::json CCSDSSimplePSKDecoderModule::getModuleStats()
            {
                nlohmann::json v;
                v["deframer_lock"] = deframer->getState() == deframer->STATE_SYNCED || deframer_qpsk->getState() == deframer_qpsk->STATE_SYNCED;
                if (d_rs_interleaving_depth != 0)
                    v["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3]) / 4;
                std::shared_ptr<deframing::BPSK_CCSDS_Deframer> def = deframer;
                if (d_constellation == dsp::QPSK && !d_diff_decode)       // Is we are working with non-NRZM QPSK, check what deframer to use
                    if (deframer_qpsk->getState() > deframer->getState()) // ...and use the one that is locked
                        def = deframer_qpsk;
                std::string deframer_state = def->getState() == def->STATE_NOSYNC ? "NOSYNC" : (def->getState() == def->STATE_SYNCING ? "SYNCING" : "SYNCED");
                v["deframer_state"] = deframer_state;
                return v;
            }

            void CCSDSSimplePSKDecoderModule::drawUI(bool window)
            {
                ImGui::Begin("CCSDS Simple PSK Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

                std::shared_ptr<deframing::BPSK_CCSDS_Deframer> def = deframer;
                if (d_constellation == dsp::QPSK && !d_diff_decode)       // Is we are working with non-NRZM QPSK, check what deframer to use
                    if (deframer_qpsk->getState() > deframer->getState()) // ...and use the one that is locked
                        def = deframer_qpsk;

                ImGui::BeginGroup();
                {
                    // Constellation
                    ImDrawList *draw_list = ImGui::GetWindowDrawList();
                    ImVec2 rect_min = ImGui::GetCursorScreenPos();
                    ImVec2 rect_max = {rect_min.x + 200 * ui_scale, rect_min.y + 200 * ui_scale};
                    draw_list->AddRectFilled(rect_min, rect_max, style::theme.widget_bg);
                    draw_list->PushClipRect(rect_min, rect_max);

                    if (d_constellation == dsp::BPSK)
                    {
                        for (int i = 0; i < 2048; i++)
                        {
                            draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i] / 127.0) * 130 * ui_scale) % int(200 * ui_scale),
                                                              ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 14 * ui_scale) % int(200 * ui_scale)),
                                                       2 * ui_scale, style::theme.constellation);
                        }
                    }
                    else
                    {
                        for (int i = 0; i < 2048; i++)
                        {
                            draw_list->AddCircleFilled(
                                ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                       ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
                                2 * ui_scale, style::theme.constellation);
                        }
                    }

                    draw_list->PopClipRect();
                    ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
                }
                ImGui::EndGroup();

                ImGui::SameLine();

                ImGui::BeginGroup();
                {
                    ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
                    {
                        ImGui::Text("State : ");

                        ImGui::SameLine();

                        if (def->getState() == def->STATE_NOSYNC)
                            ImGui::TextColored(style::theme.red, "NOSYNC");
                        else if (def->getState() == def->STATE_SYNCING)
                            ImGui::TextColored(style::theme.orange, "SYNCING");
                        else
                            ImGui::TextColored(style::theme.green, "SYNCED");
                    }

                    ImGui::Spacing();

                    if (d_rs_interleaving_depth != 0)
                    {
                        ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
                        {
                            ImGui::Text("RS    : ");
                            for (int i = 0; i < d_rs_interleaving_depth; i++)
                            {
                                ImGui::SameLine();

                                if (def->getState() == def->STATE_NOSYNC)
                                {
                                    ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%i ", i);
                                }
                                else
                                {
                                    if (errors[i] == -1)
                                        ImGui::TextColored(style::theme.red, "%i ", i);
                                    else if (errors[i] > 0)
                                        ImGui::TextColored(style::theme.orange, "%i ", i);
                                    else
                                        ImGui::TextColored(style::theme.green, "%i ", i);
                                }
                            }
                        }
                    }
                }
                ImGui::EndGroup();

                if (!d_is_streaming_input)
                    ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

                ImGui::End();
            }

            std::string CCSDSSimplePSKDecoderModule::getID() { return "ccsds_simple_psk_decoder"; }

            std::shared_ptr<ProcessingModule> CCSDSSimplePSKDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            {
                return std::make_shared<CCSDSSimplePSKDecoderModule>(input_file, output_file_hint, parameters);
            }
        } // namespace ccsds
    } // namespace pipeline
} // namespace satdump