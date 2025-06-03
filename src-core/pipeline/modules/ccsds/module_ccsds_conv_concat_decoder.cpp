#include "module_ccsds_conv_concat_decoder.h"
#include "common/codings/differential/nrzm.h"
#include "common/codings/randomization.h"
#include "common/widgets/themed_widgets.h"
#include "core/exception.h"
#include "logger.h"
#include <cstdint>

namespace satdump
{
    namespace pipeline
    {
        namespace ccsds
        {
            CCSDSConvConcatDecoderModule::CCSDSConvConcatDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
                : base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), is_ccsds(parameters.count("ccsds") > 0 ? parameters["ccsds"].get<bool>() : true),

                  d_constellation_str(parameters["constellation"].get<std::string>()),

                  d_iq_invert(parameters.count("iq_invert") > 0 ? parameters["iq_invert"].get<bool>() : false), d_cadu_size(parameters["cadu_size"].get<int>()),
                  d_cadu_bytes(ceil(d_cadu_size / 8.0)), // If we can't use complete bytes, add one and padding
                  d_buffer_size(std::max<int>(d_cadu_size, 8192)),

                  d_viterbi_outsync_after(parameters["viterbi_outsync_after"].get<int>()), d_viterbi_ber_threasold(parameters["viterbi_ber_thresold"].get<float>()),

                  d_diff_decode(parameters.count("nrzm") > 0 ? parameters["nrzm"].get<bool>() : false),

                  d_derand(parameters.count("derandomize") > 0 ? parameters["derandomize"].get<bool>() : true),
                  d_derand_after_rs(parameters.count("derand_after_rs") > 0 ? parameters["derand_after_rs"].get<bool>() : false),
                  d_derand_from(parameters.count("derand_start") > 0 ? parameters["derand_start"].get<int>() : 4),

                  d_conv_type(parameters.count("conv_rate") > 0 ? parameters["conv_rate"].get<std::string>() : "1/2"),

                  d_rs_interleaving_depth(parameters["rs_i"].get<int>()), d_rs_fill_bytes(parameters.count("rs_fill_bytes") > 0 ? parameters["rs_fill_bytes"].get<int>() : -1),
                  d_rs_dualbasis(parameters.count("rs_dualbasis") > 0 ? parameters["rs_dualbasis"].get<bool>() : true),
                  d_rs_type(parameters.count("rs_type") > 0 ? parameters["rs_type"].get<std::string>() : "none"),
                  d_rs_usecheck(parameters.count("rs_usecheck") > 0 ? parameters["rs_usecheck"].get<bool>() : false)
            {
                viterbi_out = new uint8_t[d_buffer_size * 8];
                soft_buffer = new int8_t[d_buffer_size];
                frame_buffer = new uint8_t[d_buffer_size * 8]; // Larger by safety
                d_bpsk_90 = false;
                d_oqpsk_mode = false;

                // Get constellation
                if (d_constellation_str == "bpsk")
                {
                    d_constellation = dsp::BPSK;
                    d_bpsk_90 = false;
                }
                else if (d_constellation_str == "bpsk_90")
                {
                    d_constellation = dsp::BPSK;
                    d_bpsk_90 = true;
                }
                else if (d_constellation_str == "qpsk")
                    d_constellation = dsp::QPSK;
                else if (d_constellation_str == "oqpsk")
                {
                    d_constellation = dsp::QPSK;
                    d_oqpsk_mode = true;
                }
                else
                    throw satdump_exception("CCSDS Concatenated 1/2 Decoder : invalid constellation type!");

                std::vector<phase_t> d_phases;

                // Get phases for the viterbi decoder to check
                if (d_constellation == dsp::BPSK && !d_bpsk_90)
                    d_phases = {PHASE_0};
                else if (d_constellation == dsp::BPSK && d_bpsk_90)
                    d_phases = {PHASE_90};
                else if (d_constellation == dsp::QPSK)
                    d_phases = {PHASE_0, PHASE_90};

                // Parse RS
                reedsolomon::RS_TYPE rstype = reedsolomon::RS223;
                if (d_rs_interleaving_depth != 0)
                {
                    if (d_rs_type == "rs223")
                        rstype = reedsolomon::RS223;
                    else if (d_rs_type == "rs239")
                        rstype = reedsolomon::RS239;
                    else
                        throw satdump_exception("CCSDS Concatenated 1/2 Decoder : invalid Reed-Solomon type!");
                }

                // Parse sync marker if set
                uint32_t asm_sync = 0x1acffc1d;
                if (parameters.count("asm") > 0)
                    asm_sync = std::stoul(parameters["asm"].get<std::string>(), nullptr, 16);

                if (d_conv_type == "1/2")
                    viterbi_coderate = PUNCRATE_1_2;
                else if (d_conv_type == "2/3")
                    viterbi_coderate = PUNCRATE_2_3;
                else if (d_conv_type == "3/4")
                    viterbi_coderate = PUNCRATE_3_4;
                else if (d_conv_type == "5/6")
                    viterbi_coderate = PUNCRATE_5_6;
                else if (d_conv_type == "7/8")
                    viterbi_coderate = PUNCRATE_7_8;

                if (viterbi_coderate == PUNCRATE_1_2)
                    viterbi = std::make_shared<viterbi::Viterbi1_2>(d_viterbi_ber_threasold, d_viterbi_outsync_after, d_buffer_size, d_phases, d_oqpsk_mode);
                else if (viterbi_coderate == PUNCRATE_2_3)
                    viterbip = std::make_shared<viterbi::Viterbi_Depunc>(std::make_shared<viterbi::puncturing::Depunc23>(), d_viterbi_ber_threasold, d_viterbi_outsync_after, d_buffer_size, d_phases,
                                                                         d_oqpsk_mode);
                else if (viterbi_coderate == PUNCRATE_3_4)
                    viterbip = std::make_shared<viterbi::Viterbi_Depunc>(std::make_shared<viterbi::puncturing::Depunc34>(), d_viterbi_ber_threasold, d_viterbi_outsync_after, d_buffer_size, d_phases,
                                                                         d_oqpsk_mode);
                else if (viterbi_coderate == PUNCRATE_5_6)
                    viterbip = std::make_shared<viterbi::Viterbi_Depunc>(std::make_shared<viterbi::puncturing::Depunc56>(), d_viterbi_ber_threasold, d_viterbi_outsync_after, d_buffer_size, d_phases,
                                                                         d_oqpsk_mode);
                else if (viterbi_coderate == PUNCRATE_7_8)
                    viterbip = std::make_shared<viterbi::Viterbi_Depunc>(std::make_shared<viterbi::puncturing::Depunc78>(), d_viterbi_ber_threasold, d_viterbi_outsync_after, d_buffer_size, d_phases,
                                                                         d_oqpsk_mode);

                deframer = std::make_shared<deframing::BPSK_CCSDS_Deframer>(d_cadu_size, asm_sync);
                if (d_rs_interleaving_depth != 0)
                    reed_solomon = std::make_shared<reedsolomon::ReedSolomon>(rstype, d_rs_fill_bytes);

                if (d_cadu_size % 8 != 0) // If this is not a perfect byte length match, pad the frames
                {
                    deframer->CADU_PADDING = d_cadu_size % 8;
                    logger->info("Frames will be padded!");
                }

                fsfsm_file_ext = is_ccsds ? ".cadu" : ".frm";
            }

            CCSDSConvConcatDecoderModule::~CCSDSConvConcatDecoderModule()
            {
                delete[] viterbi_out;
                delete[] soft_buffer;
                delete[] frame_buffer;
            }

            void CCSDSConvConcatDecoderModule::process()
            {
                diff::NRZMDiff diff;

                while (should_run())
                {
                    // Read a buffer
                    read_data((uint8_t *)soft_buffer, d_buffer_size);

                    if (d_bpsk_90 || d_iq_invert) // Symbols are swapped for some Q/BPSK sats
                        rotate_soft((int8_t *)soft_buffer, d_buffer_size, PHASE_0, true);

                    // Perform Viterbi decoding
                    int vitout = 0;
                    if (viterbi_coderate == PUNCRATE_1_2)
                    {
                        vitout = viterbi->work((int8_t *)soft_buffer, d_buffer_size, viterbi_out);
                        viterbi_ber = viterbi->ber();
                        viterbi_lock = viterbi->getState();
                    }
                    else
                    {
                        vitout = viterbip->work((int8_t *)soft_buffer, d_buffer_size, viterbi_out);
                        viterbi_ber = viterbip->ber();
                        viterbi_lock = viterbip->getState();
                    }

                    if (d_diff_decode) // Diff decoding if required
                        diff.decode_bits(viterbi_out, vitout);

                    // Run deframer
                    int frames = deframer->work(viterbi_out, vitout, frame_buffer);

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
                            write_data(cadu, d_cadu_bytes);
                        }
                    }
                }

                cleanup();
            }

            nlohmann::json CCSDSConvConcatDecoderModule::getModuleStats()
            {
                auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
                v["deframer_lock"] = deframer->getState() == deframer->STATE_SYNCED;
                v["viterbi_ber"] = viterbi_ber;
                v["viterbi_lock"] = viterbi_lock;
                if (d_rs_interleaving_depth != 0)
                    v["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3]) / 4;
                std::string viterbi_state = viterbi_lock == 0 ? "NOSYNC" : "SYNCED";
                std::string deframer_state = deframer->getState() == deframer->STATE_NOSYNC ? "NOSYNC" : (deframer->getState() == deframer->STATE_SYNCING ? "SYNCING" : "SYNCED");
                v["viterbi_state"] = viterbi_state;
                v["deframer_state"] = deframer_state;
                return v;
            }

            void CCSDSConvConcatDecoderModule::drawUI(bool window)
            {
                if (viterbi_coderate == PUNCRATE_1_2)
                    ImGui::Begin("CCSDS r=1/2 Concatenated Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);
                else if (viterbi_coderate == PUNCRATE_2_3)
                    ImGui::Begin("CCSDS r=2/3 Concatenated Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);
                else if (viterbi_coderate == PUNCRATE_3_4)
                    ImGui::Begin("CCSDS r=3/4 Concatenated Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);
                else if (viterbi_coderate == PUNCRATE_5_6)
                    ImGui::Begin("CCSDS r=5/6 Concatenated Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);
                if (viterbi_coderate == PUNCRATE_7_8)
                    ImGui::Begin("CCSDS r=7/8 Concatenated Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);
                float &ber = viterbi_ber;

                ImGui::BeginGroup();
                if (!d_is_streaming_input)
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
                    ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
                    {
                        ImGui::Text("State : ");

                        ImGui::SameLine();

                        if (viterbi_lock == 0)
                            ImGui::TextColored(style::theme.red, "NOSYNC");
                        else
                            ImGui::TextColored(style::theme.green, "SYNCED");

                        ImGui::Text("BER   : ");
                        ImGui::SameLine();
                        ImGui::TextColored(viterbi_lock == 0 ? style::theme.red : style::theme.green, UITO_C_STR(ber));

                        std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                        ber_history[200 - 1] = ber;

                        widgets::ThemedPlotLines(style::theme.plot_bg.Value, "", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                    }

                    ImGui::Spacing();

                    ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
                    {
                        ImGui::Text("State : ");

                        ImGui::SameLine();

                        if (viterbi_lock == 0)
                        {
                            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "NOSYNC");
                        }
                        else
                        {
                            if (deframer->getState() == deframer->STATE_NOSYNC)
                                ImGui::TextColored(style::theme.red, "NOSYNC");
                            else if (deframer->getState() == deframer->STATE_SYNCING)
                                ImGui::TextColored(style::theme.orange, "SYNCING");
                            else
                                ImGui::TextColored(style::theme.green, "SYNCED");
                        }
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

                                if (viterbi_lock == 0 || deframer->getState() == deframer->STATE_NOSYNC)
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

                drawProgressBar();

                ImGui::End();
            }

            std::string CCSDSConvConcatDecoderModule::getID() { return "ccsds_conv_concat_decoder"; }

            std::shared_ptr<ProcessingModule> CCSDSConvConcatDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            {
                return std::make_shared<CCSDSConvConcatDecoderModule>(input_file, output_file_hint, parameters);
            }
        } // namespace ccsds
    } // namespace pipeline
} // namespace satdump