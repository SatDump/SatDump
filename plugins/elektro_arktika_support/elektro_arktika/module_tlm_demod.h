#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "common/dsp/agc.h"
#include "common/dsp/fir.h"
#include "common/dsp/costas_loop.h"
#include "common/dsp/clock_recovery_mm.h"
#include "common/dsp/file_source.h"
#include "common/dsp/correct_iq.h"
#include "common/dsp/rational_resampler.h"
#include "common/dsp/snr_estimator.h"
#include "common/dsp/pll_carrier_tracking.h"
#include "common/dsp/freq_shift.h"
#include "common/repack_bits_byte.h"
#include "tlm_deframer.h"
#include "common/widgets/constellation.h"
#include "common/widgets/snr_plot.h"
#include <volk/volk.h>

namespace elektro_arktika
{
    /*
    This may be useful for other signals laters, but for now it can live here
    */
    class QuadratureRecomposer : public dsp::Block<complex_t, complex_t>
    {
    private:
        complex_t phase_delta1, phase_delta2;
        complex_t phase1, phase2;
        complex_t *buffer1, *buffer2;
        complex_t pshift;

        complex_t lastSample;

        void work()
        {
            int nsamples = input_stream->read();
            if (nsamples <= 0)
            {
                input_stream->flush();
                return;
            }

            volk_32fc_s32fc_x2_rotator_32fc((lv_32fc_t *)buffer1, (lv_32fc_t *)input_stream->readBuf, phase_delta1, (lv_32fc_t *)&phase1, nsamples); // Shift upper sideband down
            volk_32fc_s32fc_x2_rotator_32fc((lv_32fc_t *)buffer2, (lv_32fc_t *)input_stream->readBuf, phase_delta2, (lv_32fc_t *)&phase2, nsamples); // Shift lower sideband up

            // Swap I and Q for the lower sideband
            float tmp;
            for (int i = 0; i < nsamples; i++)
            {
                tmp = buffer2[i].real;
                buffer2[i].real = buffer2[i].imag;
                buffer2[i].imag = tmp;
            }

            // Shift the lower sideband by 90 degs in phase
            volk_32fc_s32fc_multiply_32fc((lv_32fc_t *)buffer2, (lv_32fc_t *)buffer2, pshift, nsamples);

            // Add both
            volk_32f_x2_add_32f((float *)output_stream->writeBuf, (float *)buffer1, (float *)buffer2, nsamples * 2);

            input_stream->flush();
            output_stream->swap(nsamples);
        }

    public:
        QuadratureRecomposer(std::shared_ptr<dsp::stream<complex_t>> input, float samplerate, float symbolrate) : Block(input)
        {
            buffer1 = new complex_t[STREAM_BUFFER_SIZE];
            buffer2 = new complex_t[STREAM_BUFFER_SIZE];

            lastSample = 0;

            phase1 = complex_t(1, 0);
            phase2 = complex_t(1, 0);
            phase_delta1 = complex_t(cos(2.0f * M_PI * (-symbolrate / samplerate)), sin(2.0f * M_PI * (-symbolrate / samplerate)));
            phase_delta2 = complex_t(cos(2.0f * M_PI * (symbolrate / samplerate)), sin(2.0f * M_PI * (symbolrate / samplerate)));

            pshift = complex_t(cos(M_PI / 2.0), sin(M_PI / 2.0));
        }
        ~QuadratureRecomposer()
        {
            delete[] buffer1;
            delete[] buffer2;
        }
    };

    class TLMDemodModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<dsp::FileSourceBlock> file_source;
        std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> res;
        std::shared_ptr<dsp::AGCBlock> agc;
        std::shared_ptr<dsp::PLLCarrierTrackingBlock> cpl;
        std::shared_ptr<dsp::CorrectIQBlock> dcb;
        std::shared_ptr<QuadratureRecomposer> reco;
        std::shared_ptr<dsp::FIRBlock<complex_t>> rrc;
        std::shared_ptr<dsp::CostasLoopBlock> pll;
        std::shared_ptr<dsp::MMClockRecoveryBlock<complex_t>> rec;

        const int d_samplerate;
        /*const*/ int d_buffer_size;

        RepackBitsByte repacker;
        uint8_t *bits_buffer;
        float *real_buffer;
        uint8_t *repacked_buffer;

        const int MAX_SPS = 5; // Maximum sample per symbol the demodulator will accept before resampling the input
        bool resample = false;

        int8_t *sym_buffer;

        int8_t clamp(float x)
        {
            if (x < -128.0)
                return -127;
            if (x > 127.0)
                return 127;
            return x;
        }

        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        M2M4SNREstimator snr_estimator;
        float snr, peak_snr;

        CADUDeframer deframer;

        // UI Stuff
        widgets::ConstellationViewer constellation;
        widgets::SNRPlotViewer snr_plot;

    public:
        TLMDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~TLMDemodModule();
        void init();
        void stop();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}