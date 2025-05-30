#include "juls_cyclo_test.h"
#include "common/dsp_source_sink/format_notated.h"
#include "common/widgets/stepped_slider.h"
#include "core/style.h"
#include "dsp/utils/correct_iq.h"
#include "imgui/imgui.h"

namespace satdump
{
    namespace handlers
    {
        JulsCycloHelperHandler::JulsCycloHelperHandler()
        {
            file_source = std::make_shared<ndsp::FileSourceBlock>();
            dc_block = std::make_shared<ndsp::CorrectIQBlock<complex_t>>();
            freq_shift = std::make_shared<ndsp::FreqShiftBlock>();
            splitter = std::make_shared<ndsp::SplitterBlock<complex_t>>();
            fft = std::make_shared<ndsp::FFTPanBlock>();
            agc = std::make_shared<ndsp::AGCBlock<complex_t>>();
            rrc = std::make_shared<ndsp::RRC_FIRBlock<complex_t>>();
            splitter2 = std::make_shared<ndsp::SplitterBlock<complex_t>>();
            fft2 = std::make_shared<ndsp::FFTPanBlock>();
            costas = std::make_shared<ndsp::CostasBlock>();
            recovery = std::make_shared<ndsp::MMClockRecoveryBlock<complex_t>>();
            constell = std::make_shared<ndsp::ConstellationDisplayBlock>();
            splitter3 = std::make_shared<ndsp::SplitterBlock<complex_t>>();
            delay_one_imag = std::make_shared<ndsp::DelayOneImagBlock>();
            constell2 = std::make_shared<ndsp::ConstellationDisplayBlock>();
            cyclo = std::make_shared<ndsp::CyclostationaryAnalysis>();
            realcomp = std::make_shared<ndsp::RealToComplexBlock>();

            splitter->set_cfg("noutputs", 2);
            splitter2->set_cfg("noutputs", 2);
            splitter3->set_cfg("noutputs", 2);
            
            dc_block->link(file_source.get(), 0, 0, 4);
            freq_shift->link(dc_block.get(), 0, 0, 4);
            splitter->link(freq_shift.get(), 0, 0, 4);
            fft->link(splitter.get(), 0, 0, 4);

            agc->link(splitter.get(), 1, 0, 4);
            rrc->link(agc.get(), 0, 0, 4);
            splitter2->link(rrc.get(), 0, 0, 4);
            cyclo->link(splitter2.get(), 0, 0, 4);
            realcomp->link(cyclo.get(), 0, 0, 4);
            fft2->link(realcomp.get(), 0, 0, 4);
            costas->link(splitter2.get(), 1, 0, 4);
            recovery->link(costas.get(), 0, 0, 4);
            splitter3->link(recovery.get(), 0, 0, 4);

            constell->link(splitter3.get(), 0, 0, 4);
            delay_one_imag->link(splitter3.get(), 1, 0, 4);
            constell2->link(delay_one_imag.get(), 0, 0, 4);

            file_source->set_cfg("file", "/home/data/SatDumpRecordings/meteosatellites_MetOpB.wav");
            file_source->set_cfg("type", "cs16");
            // fft->set_fft_settings(8192, 6e6, 30);
            fft_plot = std::make_shared<widgets::FFTPlot>(fft->output_fft_buff, 8192, -160, 30);

            fft2->avg_num = 50.0;
            // fft2->set_fft_settings(65536, 6e6, 30);
            fft_plot2 = std::make_shared<widgets::FFTPlot>(fft2->output_fft_buff, 65536, -160, 30);

            file_source_optdisp = std::make_unique<ndsp::OptDisplayerWarper>(file_source);
            freq_shift_optdisp = std::make_unique<ndsp::OptDisplayerWarper>(freq_shift);
            agc_optdisp = std::make_unique<ndsp::OptDisplayerWarper>(agc);
            rrc_optdisp = std::make_unique<ndsp::OptDisplayerWarper>(rrc);

            applyParams();
        }

        JulsCycloHelperHandler::~JulsCycloHelperHandler() {}

        void JulsCycloHelperHandler::applyParams()
        {
            uint64_t samprate = samplerate.get();
            uint64_t symrate = symbolrate.get();

            if (mod_type == MOD_BPSK)
                costas->set_cfg("order", 2);
            else if (mod_type == MOD_QPSK)
                costas->set_cfg("order", 4);
            else if (mod_type == MOD_8PSK)
                costas->set_cfg("order", 8);

            rrc->set_cfg("samplerate", samprate);
            rrc->set_cfg("symbolrate", symrate);

            freq_shift->set_cfg("samplerate", samprate);

            recovery->set_cfg("omega", double(samprate) / double(symrate));

            fft->set_fft_settings(8192, samprate, 30);
            fft2->set_fft_settings(65536, samprate, 30);

            file_source_optdisp->update();
            freq_shift_optdisp->update();
            agc_optdisp->update();
            rrc_optdisp->update();
        }

        void JulsCycloHelperHandler::drawMenu()
        {
            if (ImGui::CollapsingHeader("Demodulator", ImGuiTreeNodeFlags_DefaultOpen))
            {
                file_source_optdisp->draw();

                ImGui::Separator();

                bool upd = false;

                upd |= samplerate.draw();
                upd |= symbolrate.draw();

                if (ImGui::RadioButton("BPSK", mod_type == MOD_BPSK))
                {
                    mod_type = MOD_BPSK;
                    upd = true;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("QPSK", mod_type == MOD_QPSK))
                {
                    mod_type = MOD_QPSK;
                    upd = true;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("8PSK", mod_type == MOD_8PSK))
                {
                    mod_type = MOD_8PSK;
                    upd = true;
                }

                if (upd)
                    applyParams();

                ImGui::Separator();

                widgets::SteppedSliderFloat("FFT Max", &fft_plot->scale_max, -200, 200);
                widgets::SteppedSliderFloat("FFT Min", &fft_plot->scale_min, -200, 200);

                if (fft_plot->scale_max < fft_plot->scale_min)
                {
                    fft_plot->scale_min = fft_plot2->scale_min;
                    fft_plot->scale_max = fft_plot2->scale_max;
                }
                else if (fft_plot->scale_min > fft_plot->scale_max)
                {
                    fft_plot->scale_min = fft_plot2->scale_min;
                    fft_plot->scale_max = fft_plot2->scale_max;
                }
                else
                {
                    fft_plot2->scale_min = fft_plot->scale_min;
                    fft_plot2->scale_max = fft_plot->scale_max;
                }

                if (!started)
                {
                    if (ImGui::Button("Start"))
                    {
                        started = true;

                        file_source->start();
                        dc_block->start();
                        freq_shift->start();
                        splitter->start();
                        fft->start();
                        agc->start();
                        rrc->start();
                        splitter2->start();
                        cyclo->start();
                        realcomp->start();
                        fft2->start();
                        costas->start();
                        recovery->start();
                        splitter3->start();
                        constell->start();
                        constell2->start();
                        delay_one_imag->start();

                        file_source_optdisp->update();
                        freq_shift_optdisp->update();
                        agc_optdisp->update();
                        rrc_optdisp->update();
                    }
                }
                else
                {
                    if (ImGui::Button("Stop"))
                    {
                        started = false;

                        file_source->stop(true);

                        file_source->stop();
                        dc_block->stop();
                        freq_shift->stop();
                        splitter->stop();
                        fft->stop();
                        agc->stop();
                        rrc->stop();
                        splitter2->stop();
                        cyclo->stop();
                        realcomp->stop();
                        fft2->stop();
                        costas->stop();
                        recovery->stop();
                        splitter3->stop();
                        constell->stop();
                        constell2->stop();
                        delay_one_imag->stop();

                        file_source_optdisp->update();
                        freq_shift_optdisp->update();
                        agc_optdisp->update();
                        rrc_optdisp->update();
                    }
                }

                ImGui::Separator();
                freq_shift_optdisp->draw();

                ImGui::Separator();
                agc_optdisp->draw();

                ImGui::Separator();
                rrc_optdisp->draw();
            }
        }

        void JulsCycloHelperHandler::drawContents(ImVec2 win_size)
        {
            float right_width = win_size.x;
            float fft2_size = win_size.y;
            float fft2_ratio = 0.3;
            float left_width = ImGui::GetCursorPosX() - 9;

            ImGui::BeginChild("CycloFFT", {right_width, fft2_size}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            {
                float fft_height = fft2_size * (fft2_ratio);
                float fft2_height = fft2_size * (1 - fft2_ratio) + 15 * ui_scale;
                float wfft_widht = right_width - 9 * ui_scale;
                float wfft_widht2 = right_width - 9 * ui_scale;
                bool t = true;
#ifdef __ANDROID__
                int offset = 8;
#else
                int offset = 30;
#endif
                ImGui::SetNextWindowSizeConstraints(ImVec2((right_width + offset * ui_scale), 50), ImVec2((right_width + offset * ui_scale), fft2_size));
                ImGui::SetNextWindowSize(ImVec2((right_width + offset * ui_scale), fft2_ratio * fft2_size));
                ImGui::SetNextWindowPos(ImVec2(left_width, 25 * ui_scale));
                if (ImGui::Begin("#fft", &t,
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoScrollbar |
                                     ImGuiWindowFlags_NoScrollWithMouse))
                {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 9 * ui_scale);
                    fft_plot->draw({float(wfft_widht / 2), fft_height});

                    ImGui::SameLine();
                    // ImGui::Button("Constellation", {200 * ui_scale, 20 * ui_scale});
                    constell->constel.draw();
                    ImGui::SameLine();
                    // ImGui::Button("q(t) = [t - (n + 1/2)Ts]", {200 * ui_scale, 20 * ui_scale}); // q(t) = [t - (n + 1/2)Ts]
                    // ImGui::SameLine();
                    constell2->constel.draw();

                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                        fft2_ratio = ImGui::GetWindowHeight() / fft2_size;
                }
                ImGui::EndChild();
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15 * ui_scale);
                    fft_plot2->draw({wfft_widht2, fft2_height});
                    if (ImGui::IsWindowHovered())
                    {
                        ImVec2 mouse_pos = ImGui::GetMousePos();
                        float ratio = (mouse_pos.x - left_width - 16 * ui_scale) / (right_width - 9 * ui_scale) - 0.5;
                        ImGui::SetTooltip("%s", ((ratio >= 0 ? "" : "- ") + format_notated(abs(ratio) * samplerate.get(), "Hz\n") + format_notated(0 + ratio * samplerate.get(), "Hz")).c_str());
                    }
                }
            }
            ImGui::EndChild();
            // fft_plot->draw({400, 400});
            // ImGui::SameLine();
            // fft_plot2->draw({400, 400});
            //
            // constell->constel.draw();
            // ImGui::SameLine();
            // constell2->constel.draw();
        }
    } // namespace handlers
} // namespace satdump
