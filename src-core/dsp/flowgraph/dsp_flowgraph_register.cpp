#include "dsp_flowgraph_register.h"
#include "common/dsp_source_sink/format_notated.h"
#include "common/widgets/waterfall_plot.h"
#include "core/plugin.h"
#include <complex.h>
#include <cstdint>

#include "core/resources.h"
#include "dsp/agc/agc.h"
#include "dsp/agc/agc_fast.h"
#include "dsp/clock_recovery/clock_recovery_mm.h"
#include "dsp/clock_recovery/clock_recovery_mm_fast.h"
#include "dsp/conv/char_to_float.h"
#include "dsp/conv/complex_to_float.h"
#include "dsp/conv/complex_to_ifloat.h"
#include "dsp/conv/complex_to_imag.h"
#include "dsp/conv/complex_to_mag.h"
#include "dsp/conv/complex_to_mag_squared.h"
#include "dsp/conv/complex_to_real.h"
#include "dsp/conv/float_to_char.h"
#include "dsp/conv/float_to_complex.h"
#include "dsp/conv/ifloat_to_complex.h"
#include "dsp/conv/real_to_complex.h"
#include "dsp/conv/short_to_float.h"
#include "dsp/conv/uchar_to_float.h"
#include "dsp/ddc/ddc.h"
#include "dsp/digital/binary_slicer.h"
#include "dsp/digital/cadu_deframer.h"
#include "dsp/digital/cadu_derand.h"
#include "dsp/digital/differential_decoder.h"
#include "dsp/displays/const_disp.h"
#include "dsp/displays/hist_disp.h"
#include "dsp/fft/fft_pan.h"
#include "dsp/filter/fft.h"
#include "dsp/filter/fir.h"
#include "dsp/filter/rrc.h"
#include "dsp/flowgraph/flowgraph.h"
#include "dsp/flowgraph/node_int.h"
#include "dsp/hier/audio_demod.h"
#include "dsp/hier/psk_demod.h"
#include "dsp/io/file_sink.h"
#include "dsp/io/file_source.h"
#include "dsp/io/iq_sink.h"
#include "dsp/io/iq_source.h"
#include "dsp/io/nng_sink.h"
#include "dsp/io/waveform.h"
#include "dsp/path/selector.h"
#include "dsp/path/splitter.h"
#include "dsp/path/switch.h"
#include "dsp/pll/costas.h"
#include "dsp/pll/costas_fast.h"
#include "dsp/pll/pll_carrier_tracking.h"
#include "dsp/resampling/rational_resampler.h"
#include "dsp/utils/add.h"
#include "dsp/utils/blanker.h"
#include "dsp/utils/correct_iq.h"
#include "dsp/utils/cyclostationary_analysis.h"
#include "dsp/utils/delay_one_imag.h"
#include "dsp/utils/exponentiate.h"
#include "dsp/utils/freq_shift.h"
#include "dsp/utils/hilbert.h"
#include "dsp/utils/multiply.h"
#include "dsp/utils/psk_snr_estimator.h"
#include "dsp/utils/quadrature_demod.h"
#include "dsp/utils/samplerate_meter.h"
#include "dsp/utils/subtract.h"
#include "dsp/utils/throttle.h"
#include "dsp/utils/vco.h"

#include "common/widgets/fft_plot.h"
#include "imgui/imgui.h"

namespace satdump
{
    namespace ndsp
    {
        namespace flowgraph
        {
            class NodeTestIQSource : public NodeInternal
            {
            public:
                NodeTestIQSource(const Flowgraph *f) : NodeInternal(f, std::make_shared<ndsp::IQSourceBlock>()) {}

                virtual bool render()
                {
                    NodeInternal::render();
                    float prog = double(((ndsp::IQSourceBlock *)blk.get())->d_progress) / double(((ndsp::IQSourceBlock *)blk.get())->d_filesize);
                    ImGui::ProgressBar(prog, {100, 20});
                    return false;
                }
            };

            class NodeTestConst : public NodeInternal
            {
            public:
                NodeTestConst(const Flowgraph *f) : NodeInternal(f, std::make_shared<ndsp::ConstellationDisplayBlock>()) {}

                virtual bool render()
                {
                    NodeInternal::render();
                    ((ndsp::ConstellationDisplayBlock *)blk.get())->constel.draw();
                    return false;
                }
            };

            template <typename T>
            class NodeSamplerateMeter : public NodeInternal
            {
            public:
                NodeSamplerateMeter(const Flowgraph *f) : NodeInternal(f, std::make_shared<ndsp::SamplerateMeterBlock<T>>()) {}

                virtual bool render()
                {
                    NodeInternal::render();
                    ImGui::Text("%s", format_notated(((ndsp::SamplerateMeterBlock<T> *)blk.get())->measured_samplerate, "SPS", 4).c_str());
                    return false;
                }
            };

            class NodeTestFFT : public NodeInternal
            {
            private:
                widgets::FFTPlot fft;
                widgets::WaterfallPlot waterfall;
                int last_fft_size = 0;
                int wat_rate = 5;

            public:
                NodeTestFFT(const Flowgraph *f)
                    : NodeInternal(f, std::make_shared<ndsp::FFTPanBlock>()), fft(((ndsp::FFTPanBlock *)blk.get())->output_fft_buff, 8192, -150, 150), waterfall(131072, 400)
                {
                    waterfall.set_rate(30, wat_rate);
                    waterfall.set_palette(colormaps::loadMap(resources::getResourcePath("waterfall/classic.json")));

                    ((ndsp::FFTPanBlock *)blk.get())->on_fft = [this](float *b, int size)
                    {
                        if (size != last_fft_size)
                        {
                            fft.set_size(size);
                            waterfall.set_size(size);
                            last_fft_size = size;
                        }

                        waterfall.push_fft(b);
                    };
                }

                virtual bool render()
                {
                    fft.draw({500, 100});
                    waterfall.draw({500, 300});

                    bool v = NodeInternal::render();
                    ImGui::SetNextItemWidth(200);
                    ImGui::SliderFloat("FFT Min", &fft.scale_min, -200, 200);
                    ImGui::SetNextItemWidth(200);
                    ImGui::SliderFloat("FFT Max", &fft.scale_max, -200, 200);

                    updateW();

                    if (v)
                    {
                        int rate = blk->get_cfg("rate");
                        waterfall.set_rate(rate, wat_rate);
                    }

                    return v;
                }

                void updateW()
                {
                    if (fft.scale_max < fft.scale_min)
                    {
                        fft.scale_min = waterfall.scale_min;
                        fft.scale_max = waterfall.scale_max;
                    }
                    else if (fft.scale_min > fft.scale_max)
                    {
                        fft.scale_min = waterfall.scale_min;
                        fft.scale_max = waterfall.scale_max;
                    }
                    else
                    {
                        waterfall.scale_min = fft.scale_min;
                        waterfall.scale_max = fft.scale_max;
                    }
                }

                nlohmann::json getP()
                {
                    auto v = NodeInternal::getP();
                    v["fft_min"] = fft.scale_min;
                    v["fft_max"] = fft.scale_max;
                    return v;
                }

                void setP(nlohmann::json p)
                {
                    NodeInternal::setP(p);
                    if (p.contains("fft_min"))
                        fft.scale_min = p["fft_min"];
                    if (p.contains("fft_max"))
                        fft.scale_max = p["fft_max"];
                    updateW();

                    int rate = blk->get_cfg("rate");
                    waterfall.set_rate(rate, wat_rate);
                }
            };

            class NodeTestHisto : public NodeInternal
            {
            public:
                NodeTestHisto(const Flowgraph *f) : NodeInternal(f, std::make_shared<ndsp::HistogramDisplayBlock>()) {}

                virtual bool render()
                {
                    NodeInternal::render();
                    ((ndsp::HistogramDisplayBlock *)blk.get())->histo.draw();
                    return false;
                }
            };

            void registerNodesInFlowgraph(Flowgraph &flowgraph)
            {
                registerNode<NodeTestIQSource>(flowgraph, "IO/IQ Source");

                registerNode<NodeSamplerateMeter<complex_t>>(flowgraph, "Utils/Samplerate Meter CC");
                registerNode<NodeSamplerateMeter<float>>(flowgraph, "Utils/Samplerate Meter FF");

                registerNodeSimple<ndsp::IQSinkBlock>(flowgraph, "IO/IQ Sink");

                registerNode<NodeTestFFT>(flowgraph, "FFT/FFT Pan");
                registerNode<NodeTestConst>(flowgraph, "View/Constellation Display");
                registerNode<NodeTestHisto>(flowgraph, "View/Histogram Display");

                registerNodeSimple<ndsp::AGCBlock<complex_t>>(flowgraph, "AGC/Agc CC");
                registerNodeSimple<ndsp::AGCBlock<float>>(flowgraph, "AGC/Agc FF");

                registerNodeSimple<ndsp::AGCFastBlock<complex_t>>(flowgraph, "AGC/Agc Fast CC");
                registerNodeSimple<ndsp::AGCFastBlock<float>>(flowgraph, "AGC/Agc Fast FF");

                registerNodeSimple<ndsp::MultiplyBlock<float>>(flowgraph, "Utils/Multiply FF");
                registerNodeSimple<ndsp::MultiplyBlock<complex_t>>(flowgraph, "Utils/Multiply CC");

                registerNodeSimple<ndsp::SubtractBlock<float>>(flowgraph, "Utils/Subtract FF");
                registerNodeSimple<ndsp::SubtractBlock<complex_t>>(flowgraph, "Utils/Subtract CC");

                registerNodeSimple<ndsp::AddBlock<float>>(flowgraph, "Utils/Add FF");
                registerNodeSimple<ndsp::AddBlock<complex_t>>(flowgraph, "Utils/Add CC");

                registerNodeSimple<ndsp::CostasBlock>(flowgraph, "PLL/Costas Loop");
                registerNodeSimple<ndsp::CostasFastBlock>(flowgraph, "PLL/Costas Loop Fast");
                registerNodeSimple<ndsp::PLLCarrierTrackingBlock>(flowgraph, "PLL/PLL Carrier Tracking");

                registerNodeSimple<ndsp::MMClockRecoveryBlock<complex_t>>(flowgraph, "Timing/Clock Recovery MM CC");
                registerNodeSimple<ndsp::MMClockRecoveryBlock<float>>(flowgraph, "Timing/Clock Recovery MM FF");

                registerNodeSimple<ndsp::MMClockRecoveryFastBlock<complex_t>>(flowgraph, "Timing/Clock Recovery Fast MM CC");
                registerNodeSimple<ndsp::MMClockRecoveryFastBlock<float>>(flowgraph, "Timing/Clock Recovery Fast MM FF");

                registerNodeSimple<ndsp::RationalResamplerBlock<complex_t>>(flowgraph, "Resampling/Rational Resampler CC");
                registerNodeSimple<ndsp::RationalResamplerBlock<float>>(flowgraph, "Resampling/Rational Resampler FF");

                registerNodeSimple<ndsp::FIRBlock<complex_t>>(flowgraph, "Filter/FIR CCF");
                registerNodeSimple<ndsp::FIRBlock<float>>(flowgraph, "Filter/FIR FFF");
                registerNodeSimple<ndsp::FIRBlock<complex_t, complex_t>>(flowgraph, "Filter/FIR CCC");

                registerNodeSimple<ndsp::FFTFilterBlock<complex_t>>(flowgraph, "Filter/FFT CCF");
                // registerNodeSimple<ndsp::FIRBlock<float>>(flowgraph, "Filter/FFT FFF");
                registerNodeSimple<ndsp::FFTFilterBlock<complex_t, complex_t>>(flowgraph, "Filter/FFT CCC");

                registerNodeSimple<ndsp::RRC_Block<FIRBlock<complex_t>>>(flowgraph, "Filter/RRC FIR CC");
                registerNodeSimple<ndsp::RRC_Block<FFTFilterBlock<complex_t>>>(flowgraph, "Filter/RRC FFT CC");

                registerNodeSimple<ndsp::CyclostationaryAnalysis>(flowgraph, "Utils/Cyclostationary Analysis");

                registerNodeSimple<ndsp::DelayOneImagBlock>(flowgraph, "Utils/Delay One Imag");

                registerNodeSimple<ndsp::CorrectIQBlock<complex_t>>(flowgraph, "Utils/Correct IQ");

                registerNodeSimple<ndsp::FreqShiftBlock>(flowgraph, "Utils/Frequency Shift");

                registerNodeSimple<ndsp::QuadratureDemodBlock>(flowgraph, "Utils/Quadrature Demod");
                registerNodeSimple<ndsp::HilbertBlock>(flowgraph, "Utils/Hilbert Transform");
                registerNodeSimple<ndsp::VCOBlock>(flowgraph, "Utils/VCO");

                registerNodeSimple<ndsp::SplitterBlock<complex_t>>(flowgraph, "Utils/Splitter CC");
                registerNodeSimple<ndsp::SplitterBlock<float>>(flowgraph, "Utils/Splitter FF");
                registerNodeSimple<ndsp::DDC_Block>(flowgraph, "DDC/DDC");

                registerNodeSimple<ndsp::SwitchBlock<complex_t>>(flowgraph, "Utils/Switch CC");
                registerNodeSimple<ndsp::SwitchBlock<float>>(flowgraph, "Utils/Switch FF");

                registerNodeSimple<ndsp::SelectorBlock<complex_t>>(flowgraph, "Utils/Selector CC");
                registerNodeSimple<ndsp::SelectorBlock<float>>(flowgraph, "Utils/Selector FF");

                registerNodeSimple<ndsp::ThrottleBlock<complex_t>>(flowgraph, "Utils/Throttle CC");
                registerNodeSimple<ndsp::ThrottleBlock<float>>(flowgraph, "Utils/Throttle FF");

                registerNodeSimple<ndsp::BlankerBlock<complex_t>>(flowgraph, "Utils/Blanker CC");
                registerNodeSimple<ndsp::BlankerBlock<float>>(flowgraph, "Utils/Blanker FF");

                registerNodeSimple<ndsp::ExponentiateBlock>(flowgraph, "Utils/Exponentiate CC");

                registerNodeSimple<ndsp::NNGSinkBlock<complex_t>>(flowgraph, "IO/NNG Sink C");

                registerNodeSimple<ndsp::WaveformBlock<float>>(flowgraph, "IO/Waveform F");
                registerNodeSimple<ndsp::WaveformBlock<complex_t>>(flowgraph, "IO/Waveform C");

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

                registerNodeSimple<ndsp::UCharToFloatBlock>(flowgraph, "Conv/UChar To Float");
                registerNodeSimple<ndsp::CharToFloatBlock>(flowgraph, "Conv/Char To Float");
                registerNodeSimple<ndsp::ShortToFloatBlock>(flowgraph, "Conv/Short To Float");

                registerNodeSimple<ndsp::FloatToCharBlock>(flowgraph, "Conv/Float To Char");

                registerNodeSimple<ndsp::IFloatToComplexBlock>(flowgraph, "Conv/IFloat To Complex");
                registerNodeSimple<ndsp::ComplexToIFloatBlock>(flowgraph, "Conv/Complex To IFloat");

                registerNodeSimple<ndsp::RealToComplexBlock>(flowgraph, "Conv/Real To Complex");

                registerNodeSimple<ndsp::ComplexToFloatBlock>(flowgraph, "Conv/Complex To Float");
                registerNodeSimple<ndsp::FloatToComplexBlock>(flowgraph, "Conv/Float To Complex");
                registerNodeSimple<ndsp::ComplexToImagBlock>(flowgraph, "Conv/Complex To Imag");
                registerNodeSimple<ndsp::ComplexToRealBlock>(flowgraph, "Conv/Complex To Real");
                registerNodeSimple<ndsp::ComplexToMagBlock>(flowgraph, "Conv/Complex To Mag");
                registerNodeSimple<ndsp::ComplexToMagSquaredBlock>(flowgraph, "Conv/Complex To Mag²");

                registerNodeSimple<ndsp::PSKDemodHierBlock>(flowgraph, "Modem/PSK Demod");

                registerNodeSimple<ndsp::AudioDemodHierBlock>(flowgraph, "Modem/Audio Demod");

                registerNodeSimple<ndsp::PSKSnrEstimatorBlock>(flowgraph, "Utils/PSK SNR Estimator");

                registerNodeSimple<ndsp::BinarySlicerBlock>(flowgraph, "Digital/Binary Slicer");
                registerNodeSimple<ndsp::DifferentialDecoderBlock>(flowgraph, "Digital/Differential Decoder");
                registerNodeSimple<ndsp::CADUDeframerBlock>(flowgraph, "Digital/CADU Deframer");
                registerNodeSimple<ndsp::CADUDerandBlock>(flowgraph, "Digital/CADU Derand");

                eventBus->fire_event<RegisterNodesEvent>({flowgraph.node_internal_registry});

#if 0
            // Local devices!!
            std::vector<ndsp::DeviceInfo> found_devices = ndsp::getDeviceList();
            for (auto &dev : found_devices)
            {
                flowgraph.node_internal_registry.insert({"idk_dev_cc",
                                                         {"Local Devs/" + dev.name, [dev](const Flowgraph *f)
                                                          { return std::make_shared<NodeInternal>(f, ndsp::getDeviceInstanceFromInfo(dev, ndsp::DeviceBlock::MODE_NORMAL)); }}});
            }
#endif
            }
        } // namespace flowgraph
    } // namespace ndsp
} // namespace satdump