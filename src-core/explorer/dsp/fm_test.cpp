#include "fm_test.h"
#include "core/exception.h"
#include "core/plugin.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include "dsp/device/options_displayer.h"
#include "imgui/imgui.h"
#include <memory>

namespace satdump
{
    namespace handlers
    {
        // TODOREWORK better block registration system!
        std::shared_ptr<ndsp::Block> getBlock(std::string id)
        {
            std::vector<std::shared_ptr<ndsp::Block>> v;
            eventBus->fire_event<ndsp::RequestBlockEvent>({id, v});
            if (v.size() == 0)
                throw satdump_exception("Couldn't get block " + id);
            return v[0];
        }

        FMTestHandler::FMTestHandler()
        {
            foundDevices = ndsp::getDeviceList(ndsp::DeviceBlock::MODE_RX_TX);

            // Setup TX blocks
            tx_audio_source = getBlock("portaudio_source_f");
            tx_resamp = std::make_shared<ndsp::RationalResamplerBlock<float>>();
            tx_vco = std::make_shared<ndsp::VCOBlock>();
            tx_blanker = std::make_shared<ndsp::BlankerBlock<complex_t>>();

            tx_audio_source->set_cfg("samplerate", 48e3);
            tx_resamp->set_cfg("decimation", 48e3);
            tx_resamp->set_cfg("interpolation", 1e6);
            tx_vco->set_cfg("k", 0.05);
            tx_vco->set_cfg("amp", 0.1);
            tx_blanker->set_cfg("blank", true);

            tx_resamp->link(tx_audio_source.get(), 0, 0, 4);
            tx_vco->link(tx_resamp.get(), 0, 0, 4);
            tx_blanker->link(tx_vco.get(), 0, 0, 4);

            // Setup RX blocks
            rx_agc = std::make_shared<ndsp::AGCBlock<complex_t>>();
            rx_resamp = std::make_shared<ndsp::RationalResamplerBlock<complex_t>>();
            rx_quad = std::make_shared<ndsp::QuadratureDemodBlock>();
            rx_audio_sink = getBlock("portaudio_sink_f");

            rx_resamp->set_cfg("decimation", 1e6);
            rx_resamp->set_cfg("interpolation", 48e3);
            rx_quad->set_cfg("gain", 0.012);
            rx_audio_sink->set_cfg("samplerate", 48e3);

            rx_resamp->link(rx_agc.get(), 0, 0, 4);
            rx_quad->link(rx_resamp.get(), 0, 0, 4);
            rx_audio_sink->link(rx_quad.get(), 0, 0, 4);
        }

        FMTestHandler::~FMTestHandler() {}

        void FMTestHandler::drawMenu()
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

                            dev = ndsp::getDeviceInstanceFromInfo(curDeviceI, ndsp::DeviceBlock::MODE_RX_TX);
                            dev->setStreamSamplerate(0, 0, 1e6);
                            dev->setStreamSamplerate(0, 1, 1e6);
                            options_displayer_dev = std::make_shared<ndsp::OptDisplayerWarper>(dev);
                            break;
                        }
                    }

                    ImGui::EndCombo();
                }
                if (deviceRunning)
                    style::endDisabled();

                if (options_displayer_dev)
                    options_displayer_dev->draw();

                ImGui::Separator();

                if (options_displayer_dev)
                {
                    if (!deviceRunning)
                    {
                        if (ImGui::Button("Start") && dev)
                        {
                            taskq.push(
                                [this]()
                                {
                                    rx_agc->link(dev.get(), 0, 0, 100); //        fftp->inputs[0] = dev->outputs[0];
                                    dev->link(tx_blanker.get(), 0, 0, 4);

                                    dev->start();

                                    tx_audio_source->start();
                                    tx_resamp->start();
                                    tx_vco->start();
                                    tx_blanker->start();

                                    rx_agc->start();
                                    rx_resamp->start();
                                    rx_quad->start();
                                    rx_audio_sink->start();

                                    options_displayer_dev->update();
                                });

                            deviceRunning = true;
                        }
                    }
                    else
                    {

                        if (ImGui::Button("Stop"))
                        {
                            taskq.push(
                                [this]()
                                {
                                    tx_audio_source->stop(true);
                                    tx_resamp->stop();
                                    tx_vco->stop();
                                    tx_blanker->stop();

                                    dev->stop(true);

                                    rx_agc->stop();
                                    rx_resamp->stop();
                                    rx_quad->stop();
                                    rx_audio_sink->stop();

                                    options_displayer_dev->update();
                                });

                            deviceRunning = false;
                        }
                    }
                }
            }

            // Handle PTT
            tx_blanker->set_cfg("blank", !ImGui::IsKeyDown(ImGuiKey_T));
        }

        void FMTestHandler::drawContents(ImVec2 win_size) {}
    } // namespace handlers
} // namespace satdump