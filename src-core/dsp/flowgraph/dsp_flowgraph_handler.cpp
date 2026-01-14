#include "dsp_flowgraph_handler.h"
#include "core/backend.h"
#include "core/plugin.h"
#include "dsp/agc/agc.h"
#include "dsp/device/dev.h"
#include "dsp/fft/fft_pan.h"
#include "dsp/filter/rrc.h"
#include "dsp/io/file_source.h"
#include "dsp/io/nng_iq_sink.h"
#include "dsp/path/splitter.h"
#include "dsp/utils/throttle.h"
#include "imgui/imgui.h"
#include "imgui/imnodes/imnodes.h"
#include "logger.h"

#include "common/widgets/fft_plot.h"

#include "common/dsp/filter/firdes.h"
#include "dsp/filter/fir.h"

#include "dsp/clock_recovery/clock_recovery_mm.h"
#include "dsp/displays/const_disp.h"
#include "dsp/displays/hist_disp.h"
#include "dsp/pll/costas.h"

#include "dsp/utils/correct_iq.h"
#include "dsp/utils/cyclostationary_analysis.h"
#include "dsp/utils/delay_one_imag.h"
#include "dsp/utils/freq_shift.h"
#include "dsp/utils/hilbert.h"
#include "dsp/utils/quadrature_demod.h"
#include "dsp/utils/real_to_complex.h"

#include "nlohmann/json_utils.h"
#include <complex.h>
#include <fstream>
#include <memory>

// #include "dsp/device/airspy/airspy_dev.h"

namespace satdump
{
    namespace handlers
    {
        class NodeTestFileSource : public ndsp::NodeInternal
        {
        public:
            NodeTestFileSource(const ndsp::Flowgraph *f) : ndsp::NodeInternal(f, std::make_shared<ndsp::FileSourceBlock>()) {}

            virtual bool render()
            {
                ndsp::NodeInternal::render();
                float prog = double(((ndsp::FileSourceBlock *)blk.get())->d_progress) / double(((ndsp::FileSourceBlock *)blk.get())->d_filesize);
                ImGui::ProgressBar(prog, {100, 20});
                return false;
            }
        };

        class NodeTestConst : public ndsp::NodeInternal
        {
        public:
            NodeTestConst(const ndsp::Flowgraph *f) : ndsp::NodeInternal(f, std::make_shared<ndsp::ConstellationDisplayBlock>()) {}

            virtual bool render()
            {
                ndsp::NodeInternal::render();
                ((ndsp::ConstellationDisplayBlock *)blk.get())->constel.draw();
                return false;
            }
        };

        class NodeTestFFT : public ndsp::NodeInternal
        {
        private:
            widgets::FFTPlot fft;

        public:
            NodeTestFFT(const ndsp::Flowgraph *f) : ndsp::NodeInternal(f, std::make_shared<ndsp::FFTPanBlock>()), fft(((ndsp::FFTPanBlock *)blk.get())->output_fft_buff, 8192, -150, 150)
            {
                ((ndsp::FFTPanBlock *)blk.get())->set_fft_settings(8192, 6e6);
            }

            virtual bool render()
            {
                ndsp::NodeInternal::render();
                fft.draw({500, 500});
                return false;
            }
        };

        class NodeTestHisto : public ndsp::NodeInternal
        {
        public:
            NodeTestHisto(const ndsp::Flowgraph *f) : ndsp::NodeInternal(f, std::make_shared<ndsp::HistogramDisplayBlock>()) {}

            virtual bool render()
            {
                ndsp::NodeInternal::render();
                ((ndsp::HistogramDisplayBlock *)blk.get())->histo.draw();
                return false;
            }
        };

        DSPFlowGraphHandler::DSPFlowGraphHandler()
        {
            // TODOREWORK
            if (ImNodes::GetCurrentContext() == nullptr)
                ImNodes::CreateContext();

            handler_tree_icon = u8"\uf92f";

            flowgraph.node_internal_registry.insert({"file_source_cc", {"IO/File Source", [](const ndsp::Flowgraph *f) { return std::make_shared<NodeTestFileSource>(f); }}});
            flowgraph.node_internal_registry.insert(
                {"agc_cc", {"Agc/Agc CC", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::AGCBlock<complex_t>>()); }}});
            flowgraph.node_internal_registry.insert({"fft_pan_cc", {"FFT/FFT Pan", [](const ndsp::Flowgraph *f) { return std::make_shared<NodeTestFFT>(f); }}});

            //            flowgraph.node_internal_registry.insert({"fir_rrc_cc", {"Filter/FIR RRC Filter CC", [](const ndsp::Flowgraph *f) { return std::make_shared<NodeTestRRC>(f, true); }}});
            //            flowgraph.node_internal_registry.insert({"fir_rrc_ff", {"Filter/FIR RRC Filter FF", [](const ndsp::Flowgraph *f) { return std::make_shared<NodeTestRRC>(f, false); }}});

            flowgraph.node_internal_registry.insert(
                {"costas_cc", {"PLL/Costas Loop", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::CostasBlock>()); }}});

            flowgraph.node_internal_registry.insert({"const_disp_c", {"View/Constellation Display", [](const ndsp::Flowgraph *f) { return std::make_shared<NodeTestConst>(f); }}});

            flowgraph.node_internal_registry.insert({"histo_disp_c", {"View/Histogram Display", [](const ndsp::Flowgraph *f) { return std::make_shared<NodeTestHisto>(f); }}});

            flowgraph.node_internal_registry.insert(
                {"mm_clock_recovery_cc",
                 {"PLL/Clock Recovery MM CC", [=](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::MMClockRecoveryBlock<complex_t>>()); }}});

            flowgraph.node_internal_registry.insert(
                {"rrc_fir_cc", {"RRC FIR CC", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::RRC_FIRBlock<complex_t>>()); }}});

            flowgraph.node_internal_registry.insert(
                {"cyclostationary_analysis_cf",
                 {"Utils/Cyclostationary Analysis", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::CyclostationaryAnalysis>()); }}});

            flowgraph.node_internal_registry.insert(
                {"delay_one_imag_cc", {"Utils/Delay One Imag", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::DelayOneImagBlock>()); }}});

            flowgraph.node_internal_registry.insert(
                {"correct_iq_cc", {"Utils/Correct IQ", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::CorrectIQBlock<complex_t>>()); }}});

            flowgraph.node_internal_registry.insert(
                {"freq_shift_cc", {"Utils/Frequency Shift", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::FreqShiftBlock>()); }}});

            flowgraph.node_internal_registry.insert(
                {"real_to_complex_fc", {"Utils/Real to Complex", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::RealToComplexBlock>()); }}});

            flowgraph.node_internal_registry.insert(
                {"quadrature_demod_cf", {"Utils/Quadrature Demod", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::QuadratureDemodBlock>()); }}});

            flowgraph.node_internal_registry.insert(
                {"hilbert_fc", {"Utils/Hilbert Transform", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::HilbertBlock>()); }}});

            flowgraph.node_internal_registry.insert(
                {"splitter_cc", {"Utils/Splitter CC", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::SplitterBlock<complex_t>>()); }}});

            flowgraph.node_internal_registry.insert(
                {"throttle_cc", {"Utils/Throttle CC", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::ThrottleBlock<complex_t>>()); }}});

            flowgraph.node_internal_registry.insert(
                {"nng_iq_sink_cc", {"IQ/NNG IQ Sink", [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::NNGIQSinkBlock>()); }}});

            //   flowgraph.node_internal_registry.insert({"airspy_dev_cc", {"Airspy Dev", [=](const ndsp::Flowgraph *f)
            //                                                              { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<ndsp::AirspyDevBlock>()); }}});

            eventBus->fire_event<RegisterNodesEvent>({flowgraph.node_internal_registry});

#if 0
            // Local devices!!
            std::vector<ndsp::DeviceInfo> found_devices = ndsp::getDeviceList();
            for (auto &dev : found_devices)
            {
                flowgraph.node_internal_registry.insert({"idk_dev_cc",
                                                         {"Local Devs/" + dev.name, [dev](const ndsp::Flowgraph *f)
                                                          { return std::make_shared<ndsp::NodeInternal>(f, ndsp::getDeviceInstanceFromInfo(dev, ndsp::DeviceBlock::MODE_NORMAL)); }}});
            }
#endif
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
                        auto fun = [&]() { flowgraph.run(); };
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

                if (running)
                    style::endDisabled();
            }

            if (ImGui::CollapsingHeader("Variables"))
            {
                if (running)
                    style::beginDisabled();

                //                flowgraph.var_manager_test.drawVariables();

                if (running)
                    style::endDisabled();
            }
        }

        void DSPFlowGraphHandler::drawMenuBar()
        {
            if (ImGui::MenuItem("Save"))
            {
                auto fun = [this]()
                {
                    std::string save_at = backend::saveFileDialog({{"JSON Files", "*.json"}}, "", "flowgraph.json");

                    if (save_at == "")
                        return;

                    std::string json = flowgraph.getJSON().dump(4);
                    std::ofstream(save_at, std::ios::binary).write(json.c_str(), json.size());
                };
                tq.push(fun);
            }

            if (ImGui::MenuItem("Load"))
            {
                auto fun = [this]()
                {
                    std::string load_at = backend::selectFileDialog({{"JSON Files", "*.json"}}, "");

                    if (load_at == "")
                        return;

                    flowgraph.setJSON(loadJsonFile(load_at));
                };
                tq.push(fun);
            }

            if (ImGui::MenuItem("To Clipboard"))
            {
                std::string json = flowgraph.getJSON().dump(4);
                ImGui::SetClipboardText(json.c_str());
            }

            if (ImGui::MenuItem("From Clipboard"))
            {
                nlohmann::json v = nlohmann::json::parse(ImGui::GetClipboardText());
                flowgraph.setJSON(v);
            }
        }

        void DSPFlowGraphHandler::drawContents(ImVec2 win_size) { flowgraph.render(); }
    } // namespace handlers
} // namespace satdump
