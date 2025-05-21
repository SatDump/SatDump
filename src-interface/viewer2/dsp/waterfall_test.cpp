#include "waterfall_test.h"
#include "common/widgets/stepped_slider.h"

namespace satdump
{
    namespace handlers
    {
        WaterfallTestHandler::WaterfallTestHandler()
        {
            {
                int rv = 0;
                if (rv = nng_bus0_open(&socket); rv != 0)
                {
                    printf("pair open error\n");
                }

                if (rv = nng_dial(socket, "ws://192.168.0.102:80/ws", &dialer, 0); rv != 0)
                {
                    printf("server listen error\n");
                }
            }

            fft_buffer = new float[1000000];
            fft_plot = std::make_shared<widgets::FFTPlot>(fft_buffer, 65536, -150, 150, 10);
            fft_plot->frequency = 431.8e6;
            fft_plot->enable_freq_scale = true;
            waterfall_plot = std::make_shared<widgets::WaterfallPlot>(65536, 500);
            waterfall_plot->set_size(65536);
            waterfall_plot->set_rate(30, 20);

            auto fun = [this]()
            {
                while (1)
                {
                    size_t size = 65536 * sizeof(float);
                    uint8_t *ptr;
                    nng_recv(socket, (char *)&ptr, &size, NNG_FLAG_ALLOC);

                    float *min = (float *)&ptr[0];
                    float *max = (float *)&ptr[4];
                    uint8_t *dat = &ptr[8];

                    for (int i = 0; i < 65536; i++)
                    {
                        fft_buffer[i] = ((dat[i] / 255.0) * (*max - *min)) + *min;
                    }

                    waterfall_plot->push_fft(fft_buffer);
                }
            };

            listenTh = std::thread(fun);
        }

        WaterfallTestHandler::~WaterfallTestHandler() { delete[] fft_buffer; }

        void WaterfallTestHandler::drawMenu()
        {
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

        void WaterfallTestHandler::drawContents(ImVec2 win_size)
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