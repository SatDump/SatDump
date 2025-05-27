#include "newrec.h"
#include "common/widgets/stepped_slider.h"
#include "core/style.h"
#include "dsp/block.h"
#include "dsp/device/dev.h"
#include "imgui/imgui.h"
#include "logger.h"

namespace satdump
{
    namespace handlers
    {
        NewRecHandler::NewRecHandler()
        {
            auto devs = foundDevices = ndsp::getDeviceList(ndsp::DeviceBlock::MODE_SINGLE_RX);

            // for (auto &d : devs)
            // {
            //     if (d.type == "rtlsdr")
            //     {
            //         dev = ndsp::getDeviceInstanceFromInfo(d, ndsp::DeviceBlock::MODE_SINGLE_RX);
            //         options_displayer_test.add_options(dev->get_cfg_list());
            //         options_displayer_test.set_values(dev->get_cfg());
            //         break;
            //     }
            // }

            fftp = std::make_shared<ndsp::FFTPanBlock>();
            fftp->avg_num = 1;

            fft_plot = std::make_shared<widgets::FFTPlot>(fftp->output_fft_buff, 65536, -150, 150, 10);
            fft_plot->frequency = 431.8e6;
            fft_plot->enable_freq_scale = true;

            waterfall_plot = std::make_shared<widgets::WaterfallPlot>(65536, 500);
            waterfall_plot->set_size(65536);
            waterfall_plot->set_rate(30, 20);

            fftp->on_fft = [this](float *p) { waterfall_plot->push_fft(p); };
        }

        NewRecHandler::~NewRecHandler() {}

        void NewRecHandler::drawMenu()
        {
            if (ImGui::CollapsingHeader("Device", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (deviceRunning)
                    style::beginDisabled();
                if (ImGui::BeginCombo("Device##devicebox", curDeviceI.name.c_str()))
                {
                    for (auto &d : foundDevices)
                    {
                        if (ImGui::Selectable(d.name.c_str(), d == curDeviceI))
                        {
                            curDeviceI = d;

                            dev = ndsp::getDeviceInstanceFromInfo(curDeviceI, ndsp::DeviceBlock::MODE_SINGLE_RX);
                            options_displayer_test.clear();
                            options_displayer_test.add_options(dev->get_cfg_list());
                            options_displayer_test.set_values(dev->get_cfg());
                            break;
                        }
                    }

                    ImGui::EndCombo();
                }
                if (deviceRunning)
                    style::endDisabled();

                nlohmann::json changed = options_displayer_test.draw();

                if (changed.size() > 0)
                {
                    logger->trace("\n%s\n", changed.dump(4).c_str());
                    auto r = dev->set_cfg(changed);
                    if (r >= ndsp::Block::RES_LISTUPD)
                    {
                        options_displayer_test.clear();
                        logger->trace("\n%s\n", dev->get_cfg_list().dump(4).c_str());
                        options_displayer_test.add_options(dev->get_cfg_list());
                        options_displayer_test.set_values(dev->get_cfg());
                    }

                    // dev->init();
                }
            }

            ImGui::Separator();

            if (!deviceRunning)
            {
                if (ImGui::Button("Start"))
                {
                    fftp->set_fft_settings(65536, dev->getStreamSamplerate(0, false), 30);
                    fftp->avg_num = 1;

                    fftp->link(dev.get(), 0, 0, 16); //        fftp->inputs[0] = dev->outputs[0];

                    dev->start();
                    fftp->start();

                    options_displayer_test.clear();
                    options_displayer_test.add_options(dev->get_cfg_list());
                    options_displayer_test.set_values(dev->get_cfg());

                    deviceRunning = true;
                }
            }
            else
            {

                if (ImGui::Button("Stop"))
                {
                    dev->stop(true);
                    fftp->stop();

                    options_displayer_test.clear();
                    options_displayer_test.add_options(dev->get_cfg_list());
                    options_displayer_test.set_values(dev->get_cfg());

                    deviceRunning = false;
                }
            }

            ImGui::Separator();

            widgets::SteppedSliderFloat("FFT Max", &fft_plot->scale_max, -160, 150);
            widgets::SteppedSliderFloat("FFT Min", &fft_plot->scale_min, -160, 150);

            if (fft_plot->scale_max < fft_plot->scale_min)
            {
                fft_plot->scale_min = waterfall_plot->scale_min;
                fft_plot->scale_max = waterfall_plot->scale_max;
            }
            else if (fft_plot->scale_min > fft_plot->scale_max)
            {
                fft_plot->scale_min = waterfall_plot->scale_min;
                fft_plot->scale_max = waterfall_plot->scale_max;
            }
            else
            {
                waterfall_plot->scale_min = fft_plot->scale_min;
                waterfall_plot->scale_max = fft_plot->scale_max;
            }
        }

        void NewRecHandler::drawContents(ImVec2 win_size)
        {
            float right_width = win_size.x;
            float wf_size = win_size.y;
            bool show_waterfall = true;
            float waterfall_ratio = 0.3;
            float left_width = ImGui::GetCursorPosX() - 9;

            ImGui::BeginChild("RecorderFFT", {right_width, wf_size}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            {
                float fft_height = wf_size * (show_waterfall ? waterfall_ratio : 1.0);
                float wf_height = wf_size * (1 - waterfall_ratio) + 15 * ui_scale;
                float wfft_widht = right_width - 9 * ui_scale;
                bool t = true;
#ifdef __ANDROID__
                int offset = 8;
#else
                int offset = 30;
#endif
                ImGui::SetNextWindowSizeConstraints(ImVec2((right_width + offset * ui_scale), 50), ImVec2((right_width + offset * ui_scale), wf_size));
                ImGui::SetNextWindowSize(ImVec2((right_width + offset * ui_scale), show_waterfall ? waterfall_ratio * wf_size : wf_size));
                ImGui::SetNextWindowPos(ImVec2(left_width, 25 * ui_scale));
                if (ImGui::Begin("#fft", &t,
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoScrollbar |
                                     ImGuiWindowFlags_NoScrollWithMouse))
                {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 9 * ui_scale);
                    fft_plot->draw({float(wfft_widht), fft_height});
                    if (show_waterfall && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                        waterfall_ratio = ImGui::GetWindowHeight() / wf_size;
                }
                ImGui::EndChild();
                if (show_waterfall)
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15 * ui_scale);
                    waterfall_plot->draw({wfft_widht, wf_height}, true);
                }
            }
            ImGui::EndChild();
        }
    } // namespace handlers
} // namespace satdump
