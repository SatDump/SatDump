#include "cyclo_test.h"
#include "common/widgets/stepped_slider.h"
#include "common/dsp/filter/firdes.h"

namespace satdump
{
    namespace viewer
    {
        CycloHelperHandler::CycloHelperHandler()
        {
            file_source = std::make_shared<ndsp::FileSourceBlock>();
            splitter = std::make_shared<ndsp::SplitterBlock<complex_t>>();
            fft = std::make_shared<ndsp::FFTPanBlock>();
            agc = std::make_shared<ndsp::AGCBlock<complex_t>>();
            rrc = std::make_shared<ndsp::FIRBlock<complex_t>>();
            splitter2 = std::make_shared<ndsp::SplitterBlock<complex_t>>();
            fft2 = std::make_shared<ndsp::FFTPanBlock>();
            costas = std::make_shared<ndsp::CostasBlock>();
            recovery = std::make_shared<ndsp::MMClockRecoveryBlock<complex_t>>();
            constell = std::make_shared<ndsp::ConstellationDisplayBlock>();

            splitter->set_cfg("noutputs", 2);
            splitter2->set_cfg("noutputs", 2);

            splitter->link(file_source.get(), 0, 0, 4);
            fft->link(splitter.get(), 0, 0, 4);

            agc->link(splitter.get(), 1, 0, 4);
            rrc->link(agc.get(), 0, 0, 4);
            splitter2->link(rrc.get(), 0, 0, 4);
            fft2->link(splitter2.get(), 0, 0, 4);
            costas->link(splitter2.get(), 1, 0, 4);
            recovery->link(costas.get(), 0, 0, 4);
            constell->link(recovery.get(), 0, 0, 4);

            file_source->set_cfg("file", "/nvme_data/MetOpB.wav");
            file_source->set_cfg("type", "cs16");
            fft->set_fft_settings(8192, 6e6, 30);
            fft_plot = std::make_shared<widgets::FFTPlot>(fft->output_fft_buff, 8192, -200, 0);

            fft2->set_fft_settings(8192, 6e6, 30);
            fft_plot2 = std::make_shared<widgets::FFTPlot>(fft2->output_fft_buff, 8192, -200, 0);

            //  mod_type = MOD_QPSK;
            applyParams();

            opt_displayer.add_options(agc->get_cfg_list());
        }

        CycloHelperHandler::~CycloHelperHandler()
        {
        }

        void CycloHelperHandler::applyParams()
        {
            uint64_t samprate = samplerate.get();
            uint64_t symrate = symbolrate.get();

            if (mod_type == MOD_BPSK)
                costas->set_cfg("order", 2);
            else if (mod_type == MOD_QPSK)
                costas->set_cfg("order", 4);

            rrc->set_cfg("taps", dsp::firdes::root_raised_cosine(1.0, samprate, symrate, 0.5, 31));

            recovery->set_cfg("omega", double(samprate) / double(symrate));
        }

        void CycloHelperHandler::drawMenu()
        {
            if (ImGui::CollapsingHeader("Demodulator", ImGuiTreeNodeFlags_DefaultOpen))
            {
                samplerate.draw();
                symbolrate.draw();

                if (ImGui::RadioButton("BPSK", mod_type == MOD_BPSK))
                    mod_type = MOD_BPSK;
                ImGui::SameLine();
                if (ImGui::RadioButton("QPSK", mod_type == MOD_QPSK))
                    mod_type = MOD_QPSK;

                if (ImGui::Button("Set"))
                    applyParams();

                if (ImGui::Button("Start"))
                {
                    file_source->start();
                    splitter->start();
                    fft->start();
                    agc->start();
                    rrc->start();
                    splitter2->start();
                    fft2->start();
                    costas->start();
                    recovery->start();
                    constell->start();
                }

                if (ImGui::Button("Stop"))
                {
                    file_source->stop(true);

                    file_source->stop();
                    splitter->stop();
                    fft->stop();
                    agc->stop();
                    rrc->stop();
                    splitter2->stop();
                    fft2->stop();
                    costas->stop();
                    recovery->stop();
                    constell->stop();
                }

                ImGui::Separator();
                opt_displayer.draw();
            }
        }

        void CycloHelperHandler::drawContents(ImVec2 win_size)
        {
            fft_plot->draw({400, 400});
            ImGui::SameLine();
            fft_plot2->draw({400, 400});

            constell->constel.draw();
        }
    }
}