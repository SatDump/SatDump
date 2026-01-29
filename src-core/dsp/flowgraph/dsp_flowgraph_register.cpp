#include "dsp_flowgraph_register.h"
#include "core/plugin.h"

#include "dsp/agc/agc.h"
#include "dsp/clock_recovery/clock_recovery_mm_fast.h"
#include "dsp/conv/char_to_float.h"
#include "dsp/conv/complex_to_float.h"
#include "dsp/conv/complex_to_ifloat.h"
#include "dsp/conv/float_to_char.h"
#include "dsp/conv/float_to_complex.h"
#include "dsp/conv/ifloat_to_complex.h"
#include "dsp/conv/real_to_complex.h"
#include "dsp/conv/short_to_float.h"
#include "dsp/device/dev.h"
#include "dsp/fft/fft_pan.h"
#include "dsp/filter/rrc.h"
#include "dsp/flowgraph/dsp_flowgraph_handler.h"
#include "dsp/flowgraph/flowgraph.h"
#include "dsp/io/file_sink.h"
#include "dsp/io/file_source.h"
#include "dsp/io/iq_sink.h"
#include "dsp/io/iq_source.h"
#include "dsp/io/nng_iq_sink.h"
#include "dsp/io/waveform.h"
#include "dsp/path/splitter.h"
#include "dsp/pll/pll_carrier_tracking.h"
#include "dsp/resampling/rational_resampler.h"
#include "dsp/utils/add.h"
#include "dsp/utils/correct_iq.h"
#include "dsp/utils/multiply.h"
#include "dsp/utils/subtract.h"
#include "dsp/utils/throttle.h"

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
#include "dsp/utils/vco.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        class NodeTestIQSource : public ndsp::NodeInternal
        {
        public:
            NodeTestIQSource(const ndsp::Flowgraph *f) : ndsp::NodeInternal(f, std::make_shared<ndsp::IQSourceBlock>()) {}

            virtual bool render()
            {
                ndsp::NodeInternal::render();
                float prog = double(((ndsp::IQSourceBlock *)blk.get())->d_progress) / double(((ndsp::IQSourceBlock *)blk.get())->d_filesize);
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

        void registerNodesInFlowgraph(ndsp::Flowgraph &flowgraph)
        {
            registerNode<NodeTestIQSource>(flowgraph, "iq_source_cc", "IO/IQ Source");

            registerNodeSimple<ndsp::IQSinkBlock>(flowgraph, "IO/IQ Sink");

            registerNode<NodeTestFFT>(flowgraph, "fft_pan_cc", "FFT/FFT Pan");
            registerNode<NodeTestConst>(flowgraph, "const_disp_c", "View/Constellation Display");
            registerNode<NodeTestHisto>(flowgraph, "histo_disp_c", "View/Histogram Display");

            registerNodeSimple<ndsp::AGCBlock<complex_t>>(flowgraph, "AGC/Agc CC");
            registerNodeSimple<ndsp::AGCBlock<float>>(flowgraph, "AGC/Agc FF");

            registerNodeSimple<ndsp::MultiplyBlock<float>>(flowgraph, "Utils/Multiply FF");
            registerNodeSimple<ndsp::MultiplyBlock<complex_t>>(flowgraph, "Utils/Multiply CC");

            registerNodeSimple<ndsp::SubtractBlock<float>>(flowgraph, "Utils/Subtract FF");
            registerNodeSimple<ndsp::SubtractBlock<complex_t>>(flowgraph, "Utils/Subtract CC");

            registerNodeSimple<ndsp::AddBlock<float>>(flowgraph, "Utils/Add FF");
            registerNodeSimple<ndsp::AddBlock<complex_t>>(flowgraph, "Utils/Add CC");

            registerNodeSimple<ndsp::CostasBlock>(flowgraph, "PLL/Costas Loop");
            registerNodeSimple<ndsp::PLLCarrierTrackingBlock>(flowgraph, "PLL/PLL Carrier Tracking");

            registerNodeSimple<ndsp::MMClockRecoveryBlock<complex_t>>(flowgraph, "Timing/Clock Recovery MM CC");
            registerNodeSimple<ndsp::MMClockRecoveryBlock<float>>(flowgraph, "Timing/Clock Recovery MM FF");

            registerNodeSimple<ndsp::MMClockRecoveryFastBlock<complex_t>>(flowgraph, "Timing/Clock Recovery Fast MM CC");
            registerNodeSimple<ndsp::MMClockRecoveryFastBlock<float>>(flowgraph, "Timing/Clock Recovery Fast MM FF");

            registerNodeSimple<ndsp::RationalResamplerBlock<complex_t>>(flowgraph, "Resampling/Rational Resampler CC");
            registerNodeSimple<ndsp::RationalResamplerBlock<float>>(flowgraph, "Resampling/Rational Resampler FF");

            registerNodeSimple<ndsp::RRC_FIRBlock<complex_t>>(flowgraph, "Filter/RRC FIR CC");

            registerNodeSimple<ndsp::CyclostationaryAnalysis>(flowgraph, "Utils/Cyclostationary Analysis");

            registerNodeSimple<ndsp::DelayOneImagBlock>(flowgraph, "Utils/Delay One Imag");

            registerNodeSimple<ndsp::CorrectIQBlock<complex_t>>(flowgraph, "Utils/Correct IQ");

            registerNodeSimple<ndsp::FreqShiftBlock>(flowgraph, "Utils/Frequency Shift");

            registerNodeSimple<ndsp::QuadratureDemodBlock>(flowgraph, "Utils/Quadrature Demod");
            registerNodeSimple<ndsp::HilbertBlock>(flowgraph, "Utils/Hilbert Transform");
            registerNodeSimple<ndsp::VCOBlock>(flowgraph, "Utils/VCO");

            registerNodeSimple<ndsp::SplitterBlock<complex_t>>(flowgraph, "Utils/Splitter CC");
            registerNodeSimple<ndsp::SplitterBlock<float>>(flowgraph, "Utils/Splitter FF");

            registerNodeSimple<ndsp::ThrottleBlock<complex_t>>(flowgraph, "Utils/Throttle CC");
            registerNodeSimple<ndsp::ThrottleBlock<float>>(flowgraph, "Utils/Throttle FF");

            registerNodeSimple<ndsp::NNGIQSinkBlock>(flowgraph, "IO/NNG IQ Sink");

            registerNodeSimple<ndsp::WaveformBlock<float>>(flowgraph, "IO/Waveform F");

            registerNodeSimple<ndsp::FileSourceBlock<complex_t>>(flowgraph, "IO/File Source C");
            registerNodeSimple<ndsp::FileSourceBlock<float>>(flowgraph, "IO/File Source F");
            registerNodeSimple<ndsp::FileSourceBlock<int16_t>>(flowgraph, "IO/File Source S");
            registerNodeSimple<ndsp::FileSourceBlock<int8_t>>(flowgraph, "IO/File Source H");
            registerNodeSimple<ndsp::FileSourceBlock<uint8_t>>(flowgraph, "IO/File Source B");

            registerNodeSimple<ndsp::FileSinkBlock<complex_t>>(flowgraph, "IO/File Sink C");
            registerNodeSimple<ndsp::FileSinkBlock<float>>(flowgraph, "IO/File Sink F");
            registerNodeSimple<ndsp::FileSinkBlock<int16_t>>(flowgraph, "IO/File Sink S");
            registerNodeSimple<ndsp::FileSinkBlock<int8_t>>(flowgraph, "IO/File Sink H");
            registerNodeSimple<ndsp::FileSinkBlock<uint8_t>>(flowgraph, "IO/File Sink B");

            registerNodeSimple<ndsp::CharToFloatBlock>(flowgraph, "Conv/Char To Float");
            registerNodeSimple<ndsp::ShortToFloatBlock>(flowgraph, "Conv/Short To Float");

            registerNodeSimple<ndsp::FloatToCharBlock>(flowgraph, "Conv/Float To Char");

            registerNodeSimple<ndsp::IFloatToComplexBlock>(flowgraph, "Conv/IFloat To Complex");
            registerNodeSimple<ndsp::ComplexToIFloatBlock>(flowgraph, "Conv/Complex To IFloat");

            registerNodeSimple<ndsp::RealToComplexBlock>(flowgraph, "Conv/Real to Complex");

            registerNodeSimple<ndsp::ComplexToFloatBlock>(flowgraph, "Conv/Complex To Float");
            registerNodeSimple<ndsp::FloatToComplexBlock>(flowgraph, "Conv/Float To Complex");

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
    } // namespace ndsp
} // namespace satdump
