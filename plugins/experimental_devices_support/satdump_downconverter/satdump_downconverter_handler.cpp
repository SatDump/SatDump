#include "satdump_downconverter_handler.h"
#include "common/widgets/frequency_input.h"
#include "common/widgets/stepped_slider.h"
#include "imgui/imgui.h"
#include "libserial/serial.h"
#include "satdump_downconverter/dc.h"
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

namespace satdump
{
    namespace exp_devs
    {
        SatDumpDownconverHandler::SatDumpDownconverHandler()
        {
            handler_tree_icon = u8"\uf471";

            fetchTh = std::thread(
                [this]()
                {
                    while (fetchTh_r)
                    {
                        updateDC();
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                });

            ports = serial_cpp::list_ports();
        }

        SatDumpDownconverHandler::~SatDumpDownconverHandler()
        {
            fetchTh_r = false;
            if (fetchTh.joinable())
                fetchTh.join();
        }

        void SatDumpDownconverHandler::updateDC()
        {
            if (!dc)
                return;

            pll_freq = dc->getPLLFreq();
            pll_lock = dc->isPLLLocked();

            if (updateOnChange)
                pll_enabled = dc->getPLLEnabled();

            if (updateOnChange)
                attenuation = dc->getAttenuation();

            if (updateOnChange)
                ref_freq = dc->getRefFreq();
            if (updateOnChange)
                ref_ext = dc->isRefExternal();

            if (updateOnChange)
                preamp_enabled = dc->getPreampEnabled();

            mcu_temp = dc->getMCUTemp();
            rf_temp = dc->getRFTemp();
            vreg_temp = dc->getVREGTemp();
        }

        void SatDumpDownconverHandler::drawMenu()
        {
            if (ImGui::CollapsingHeader("Control", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (!dc)
                {
                    for (auto &p : ports)
                    {
                        if (ImGui::Button(p.port.c_str()))
                            dc = std::make_shared<SatDumpDownconverter>(p.port);
                    }
                }

                uint64_t freq = pll_freq;
                if (widgets::FrequencyInput("PLL Freq", &freq))
                    tq.push(
                        [this, freq]()
                        {
                            dc->setPLLFreq(freq);
                            updateOnChange = true;
                        });

                bool pen = pll_enabled;
                if (ImGui::Checkbox("PLL Enabled", &pen))
                    tq.push(
                        [this, pen]()
                        {
                            dc->setPLLEnabled(pen);
                            updateOnChange = true;
                        });

                float att = attenuation;
                if (widgets::SteppedSliderFloat("Attenuation", &att, 0, 31.75, 0.25))
                    tq.push(
                        [this, att]()
                        {
                            dc->setAttenuation(att);
                            updateOnChange = true;
                        });

                uint64_t rfreq = ref_freq;
                if (widgets::FrequencyInput("Ref Freq", &rfreq))
                    tq.push(
                        [this, rfreq]()
                        {
                            dc->setRefFreq(rfreq);
                            updateOnChange = true;
                        });

                bool rext = ref_ext;
                if (ImGui::Checkbox("Ref External", &rext))
                    tq.push(
                        [this, rext]()
                        {
                            dc->setRefExternal(rext);
                            updateOnChange = true;
                        });

                bool pea = preamp_enabled;
                if (ImGui::Checkbox("Preamp Enabled", &pea))
                    tq.push(
                        [this, pea]()
                        {
                            dc->setPreampEnabled(pea);
                            updateOnChange = true;
                        });
            }
        }

        void SatDumpDownconverHandler::drawMenuBar() {}

        void SatDumpDownconverHandler::drawContents(ImVec2 win_size)
        {
            ImGui::Text("PLL Freq : %f", pll_freq);
            ImGui::Text("PLL Lock : %d", pll_lock);
            ImGui::Text("PLL Enabled : %d", pll_enabled);

            ImGui::Spacing();

            ImGui::Text("Attenuation : %f", attenuation);

            ImGui::Spacing();

            ImGui::Text("Ref Freq : %f", ref_freq);
            ImGui::Text("Ref Ext : %d", ref_ext);

            ImGui::Spacing();

            ImGui::Text("Preamp Enabled : %d", preamp_enabled);

            ImGui::Spacing();

            ImGui::Text("MCU Temp : %f", mcu_temp);
            ImGui::Text("RF Temp : %f", rf_temp);
            ImGui::Text("VREG Temp : %f", vreg_temp);
        }
    } // namespace exp_devs
} // namespace satdump
