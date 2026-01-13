#include "module_stdc_decoder.h"
#include "common/utils.h"
#include "common/widgets/themed_widgets.h"
#include "logger.h"
#include <filesystem>
#include <fstream>

namespace inmarsat
{
    namespace stdc
    {
        STDCDecoderModule::STDCDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), viterbi(ENCODED_FRAME_SIZE_NOSYNC / 2, {109, 79})
        {
            buffer = new int8_t[ENCODED_FRAME_SIZE];
            buffer_shifter = new int8_t[ENCODED_FRAME_SIZE];
            buffer_synchronized = new int8_t[ENCODED_FRAME_SIZE];
            buffer_depermuted = new int8_t[ENCODED_FRAME_SIZE];
            buffer_vitdecoded = new uint8_t[ENCODED_FRAME_SIZE];

            fsfsm_file_ext = ".frm";
        }

        STDCDecoderModule::~STDCDecoderModule()
        {
            delete[] buffer;
            delete[] buffer_shifter;
            delete[] buffer_synchronized;
            delete[] buffer_depermuted;
            delete[] buffer_vitdecoded;
        }

        void STDCDecoderModule::process()
        {
            while (should_run())
            {
                // Read a buffer
                read_data((uint8_t *)buffer, ENCODED_FRAME_SIZE);

                gotFrame = false;

                for (int i = 0; i < ENCODED_FRAME_SIZE; i++)
                {
                    memmove(&buffer_shifter[0], &buffer_shifter[1], ENCODED_FRAME_SIZE - 1);
                    buffer_shifter[ENCODED_FRAME_SIZE - 1] = buffer[i];

                    bool inverted = false;
                    int best_match = compute_frame_match(buffer_shifter, inverted);
                    if (best_match > 120)
                    {
                        for (int b = 0; b < ENCODED_FRAME_SIZE; b++)
                        {
                            if (inverted)
                                buffer_synchronized[b] = ~buffer_shifter[b];
                            else
                                buffer_synchronized[b] = buffer_shifter[b];
                        }

                        cor = best_match;

                        depermute(buffer_synchronized, buffer_depermuted);
                        deinterleave(buffer_depermuted, buffer_synchronized);
                        viterbi.work(buffer_synchronized, buffer_vitdecoded);
                        descramble(buffer_vitdecoded);

                        frm_num = buffer_vitdecoded[2] << 8 | buffer_vitdecoded[3];
                        logger->trace("Got STD-C Frame Corr %d Inv %d Ber %f No %d", best_match, (int)inverted, viterbi.ber(), frm_num);

                        write_data((uint8_t *)buffer_vitdecoded, ENCODED_FRAME_SIZE_NOSYNC / 16);

                        gotFrame = true;
                    }
                }
            }

            cleanup();
        }

        nlohmann::json STDCDecoderModule::getModuleStats()
        {
            auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
            v["deframer_lock"] = gotFrame;
            v["viterbi_ber"] = viterbi.ber();
            v["last_count"] = frm_num;
            std::string lock_state = gotFrame ? "SYNCED" : "NOSYNC";
            v["lock_state"] = lock_state;
            return v;
        }

        void STDCDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Inmarsat STD-C Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            float ber = viterbi.ber();

            ImGui::BeginGroup();
            {
                ImGui::Button("Correlator", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("Corr  : ");
                    ImGui::SameLine();
                    ImGui::TextColored(gotFrame ? style::theme.green : style::theme.orange, UITO_C_STR(cor));

                    std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                    cor_history[200 - 1] = cor;

                    widgets::ThemedPlotLines(style::theme.plot_bg.Value, "##", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 60.0f, 128.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                }

                ImGui::Spacing();

                ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("BER   : ");
                    ImGui::SameLine();
                    ImGui::TextColored(ber < 0.22 ? style::theme.green : style::theme.red, UITO_C_STR(ber));

                    std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                    ber_history[200 - 1] = ber;

                    widgets::ThemedPlotLines(style::theme.plot_bg.Value, "##", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                }
            }
            ImGui::EndGroup();

            drawProgressBar();

            ImGui::End();
        }

        std::string STDCDecoderModule::getID() { return "inmarsat_stdc_decoder"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> STDCDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<STDCDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace stdc
} // namespace inmarsat