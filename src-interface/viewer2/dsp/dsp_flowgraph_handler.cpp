#include "dsp_flowgraph_handler.h"
#include "imgui/imnodes/imnodes.h"
#include "logger.h"

#include "dsp/io/file_source.h"
#include "dsp/agc/agc.h"
#include "dsp/fft/fft_pan.h"

#include "common/widgets/fft_plot.h"

#include "dsp/filter/fir.h"
#include "common/dsp/filter/firdes.h"

#include "dsp/pll/costas.h"
#include "dsp/clock_recovery/clock_recovery_mm.h"
#include "dsp/const/const_disp.h"

#include "nlohmann/json_utils.h"

//#include "dsp/device/airspy/airspy_dev.h"

namespace satdump
{
    namespace viewer
    {
        class NodeTestFileSource : public ndsp::NodeInternal
        {

        public:
            NodeTestFileSource(const ndsp::Flowgraph *f)
                : ndsp::NodeInternal(f, std::make_shared<ndsp::FileSourceBlock>())
            {
            }

            virtual void render()
            {
                ndsp::NodeInternal::render();
                float prog = double(((ndsp::FileSourceBlock *)blk.get())->d_progress) / double(((ndsp::FileSourceBlock *)blk.get())->d_filesize);
                ImGui::ProgressBar(prog, {100, 20});
            }
        };

        class NodeTestConst : public ndsp::NodeInternal
        {
        public:
            NodeTestConst(const ndsp::Flowgraph *f)
                : ndsp::NodeInternal(f, std::make_shared<ndsp::ConstellationDisplayBlock>())
            {
            }

            virtual void render()
            {
                ndsp::NodeInternal::render();
                ((ndsp::ConstellationDisplayBlock *)blk.get())->constel.draw();
            }
        };

        class NodeTestFFT : public ndsp::NodeInternal
        {
        private:
            widgets::FFTPlot fft;

        public:
            NodeTestFFT(const ndsp::Flowgraph *f)
                : ndsp::NodeInternal(f, std::make_shared<ndsp::FFTPanBlock>()),
                  fft(((ndsp::FFTPanBlock *)blk.get())->output_fft_buff, 8192, -150, 150)
            {
                ((ndsp::FFTPanBlock *)blk.get())->set_fft_settings(8192, 6e6);
            }

            virtual void render()
            {
                ndsp::NodeInternal::render();
                fft.draw({500, 500});
            }
        };

        class NodeTestRRC : public ndsp::NodeInternal
        {
        private:
            double gain = 1.0;
            double samplerate = 6e6;
            double symbolrate = 2.33e6;
            double alpha = 0.5;
            int ntaps = 31;

        public:
            NodeTestRRC(const ndsp::Flowgraph *f, bool complex)
                : ndsp::NodeInternal(f, complex
                                            ? (std::shared_ptr<ndsp::Block>)std::make_shared<ndsp::FIRBlock<complex_t>>()
                                            : (std::shared_ptr<ndsp::Block>)std::make_shared<ndsp::FIRBlock<float>>())
            {
                j["gain"] = gain;
                j["samplerate"] = samplerate;
                j["symbolrate"] = symbolrate;
                j["alpha"] = alpha;
                j["ntaps"] = ntaps;
                j.erase("taps");
                setupJSONParams();
            }

            /* virtual void render()
             {
                 ImGui::SetNextItemWidth(120);
                 ImGui::InputDouble("Gain", &gain);
                 ImGui::SetNextItemWidth(120);
                 ImGui::InputDouble("Samplerate", &samplerate);
                 ImGui::SetNextItemWidth(120);
                 ImGui::InputDouble("Symbolrate", &symbolrate);
                 ImGui::SetNextItemWidth(120);
                 ImGui::InputDouble("Alpha", &alpha);
                 ImGui::SetNextItemWidth(120);
                 ImGui::InputInt("Ntaps", &ntaps);
             } */

            /*virtual nlohmann::json getP()
            {
                nlohmann::json v;
                v["gain"] = gain;
                v["samplerate"] = samplerate;
                v["symbolrate"] = symbolrate;
                v["alpha"] = alpha;
                v["ntaps"] = ntaps;
                return v;
            }*/

            /*virtual void setP(nlohmann::json p)
            {
                gain = getValueOrDefault(v["gain"], gain);
                samplerate = getValueOrDefault(v["samplerate"], samplerate);
                symbolrate = getValueOrDefault(v["symbolrate"], symbolrate);
                alpha = getValueOrDefault(v["alpha"], alpha);
                ntaps = getValueOrDefault(v["ntaps"], ntaps);
            }*/

            virtual void applyP()
            {
                getFinalJSON();
                nlohmann::json bv;
                bv["taps"] = dsp::firdes::root_raised_cosine(j["gain"], j["samplerate"], j["symbolrate"], j["alpha"], j["ntaps"]);
                blk->set_cfg(bv);
            }
        };

        DSPFlowGraphHandler::DSPFlowGraphHandler()
        {
            // TODOREWORK
            if (ImNodes::GetCurrentContext() == nullptr)
                ImNodes::CreateContext();

            handler_tree_icon = "\uf92f";

            flowgraph.node_internal_registry.insert({"file_source_cc", {"IO/File Source", [](const ndsp::Flowgraph *f)
                                                                        { return std::make_shared<NodeTestFileSource>(f); }}});
            flowgraph.node_internal_registry.insert({"agc_cc", {"Agc/Agc CC", [](const ndsp::Flowgraph *f)
                                                                { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::AGCBlock<complex_t>>()); }}});
            flowgraph.node_internal_registry.insert({"fft_pan_cc", {"FFT/FFT Pan", [](const ndsp::Flowgraph *f)
                                                                    { return std::make_shared<NodeTestFFT>(f); }}});

            flowgraph.node_internal_registry.insert({"fir_rrc_cc", {"Filter/FIR RRC Filter CC", [](const ndsp::Flowgraph *f)
                                                                    { return std::make_shared<NodeTestRRC>(f, true); }}});
            flowgraph.node_internal_registry.insert({"fir_rrc_ff", {"Filter/FIR RRC Filter FF", [](const ndsp::Flowgraph *f)
                                                                    { return std::make_shared<NodeTestRRC>(f, false); }}});

            flowgraph.node_internal_registry.insert({"costas_cc", {"PLL/Costas Loop", [](const ndsp::Flowgraph *f)
                                                                   { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::CostasBlock>()); }}});

            flowgraph.node_internal_registry.insert({"const_disp_cc", {"View/Constellation Display", [](const ndsp::Flowgraph *f)
                                                                       { return std::make_shared<NodeTestConst>(f); }}});

            flowgraph.node_internal_registry.insert({"mm_clock_recovery_cc", {"PLL/Clock Recovery MM CC", [=](const ndsp::Flowgraph *f)
                                                                              { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::MMClockRecoveryBlock<complex_t>>()); }}});

         //   flowgraph.node_internal_registry.insert({"airspy_dev_cc", {"Airspy Dev", [=](const ndsp::Flowgraph *f)
         //                                                              { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::AirspyDevBlock>()); }}});
        }

        DSPFlowGraphHandler::~DSPFlowGraphHandler()
        {
            if (flow_thread.joinable())
                flow_thread.join();
        }

        void DSPFlowGraphHandler::drawMenu()
        {
            bool running = flowgraph.is_running;

            if (ImGui::CollapsingHeader("Flowgraph"))
            {
                if (running)
                {
                    if (ImGui::Button("Stop"))
                        flowgraph.stop();
                }
                else
                {
                    if (ImGui::Button("Start"))
                    {
                        if (flow_thread.joinable())
                            flow_thread.join();
                        auto fun = [&]()
                        {
                            flowgraph.run();
                        };
                        flow_thread = std::thread(fun);
                    }
                }

                if (running)
                    style::beginDisabled();

                /*auto pos = ImGui::GetCursorPos();
                ImGui::Button("##test", {50, 50});
                pos.x += 10;
                ImGui::GetWindowDrawList()
                    ->AddTriangleFilled({pos.x + 10, pos.y + 43},
                                        {pos.x + 10, pos.y + 77},
                                        {pos.x + 40, pos.y + 60},
                                        ImColor(255, 255, 255, 255));*/

                //            needs_to_update |= TODO; // TODOREWORK move in top drawMenu?

                if (ImGui::Button("To JSON"))
                {
                    std::string json = flowgraph.getJSON().dump(4);
                    ImGui::SetClipboardText(json.c_str());
                }

                if (ImGui::Button("From JSON"))
                {
                    nlohmann::json v = nlohmann::json::parse(ImGui::GetClipboardText());
                    flowgraph.setJSON(v);
                }

                if (running)
                    style::endDisabled();
            }

            if (ImGui::CollapsingHeader("Variables"))
            {
                if (running)
                    style::beginDisabled();

                flowgraph.var_manager_test.drawVariables();

                if (running)
                    style::endDisabled();
            }
        }

        void DSPFlowGraphHandler::drawContents(ImVec2 win_size)
        {
            flowgraph.render();
        }
    }
}