#pragma once

#include "common/dsp/block.h"

#include "common/dsp/clock_recovery/clock_recovery_mm.h"
#include "common/dsp/demod/quadrature_demod.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/resamp/smart_resampler.h"
#include "common/dsp/utils/agc.h"
#include "common/dsp/utils/correct_iq.h"

#include "common/dsp/utils/snr_estimator.h"
#include "stx_deframer.h"
#include "utils/binary.h"
#include <functional>

namespace orbcomm
{
#define ORBCOMM_STX_FRM_SIZE 4800

    class STXDemod : public dsp::HierBlock<complex_t, uint8_t>
    {
    private:
        class STXDeframerBlock : public dsp::Block<float, uint8_t>
        {
        private:
            STXDeframer stx_deframer;
            M2M4SNREstimator snr_estimator;
            uint8_t *bits_buf;
            uint8_t *frames;

        private:
            void work()
            {
                int dat_size = input_stream->read();

                if (dat_size <= 0)
                {
                    input_stream->flush();
                    return;
                }

                // Estimate SNR
                snr_estimator.update((complex_t *)input_stream->readBuf, dat_size / 2);
                snr = snr_estimator.snr();

                // Def Status
                deframer_state = stx_deframer.getState();

                for (int i = 0; i < dat_size; i++)
                    bits_buf[i] = input_stream->readBuf[i] > 0;

                input_stream->flush();

                int framen = stx_deframer.work(bits_buf, dat_size, frames);

                for (int i = 0; i < framen; i++)
                    for (int y = 0; y < (ORBCOMM_STX_FRM_SIZE / 8); y++)
                        frames[i * (ORBCOMM_STX_FRM_SIZE / 8) + y] = satdump::reverseBits(frames[i * (ORBCOMM_STX_FRM_SIZE / 8) + y]);

                if (framen > 0)
                    callback(frames, framen);
            }

        public:
            STXDeframerBlock(std::shared_ptr<dsp::stream<float>> input) : dsp::Block<float, uint8_t>(input), stx_deframer(ORBCOMM_STX_FRM_SIZE)
            {
                bits_buf = new uint8_t[ORBCOMM_STX_FRM_SIZE * 50 * 8];
                frames = new uint8_t[ORBCOMM_STX_FRM_SIZE * 50];
            }

            ~STXDeframerBlock()
            {
                delete[] bits_buf;
                delete[] frames;
            }

            std::function<void(uint8_t *, int)> callback;
            int deframer_state = 0;
            float snr = 0;
        };

    private:
        std::shared_ptr<dsp::SmartResamplerBlock<complex_t>> res;
        std::shared_ptr<dsp::AGCBlock<complex_t>> agc;
        std::shared_ptr<dsp::QuadratureDemodBlock> qua;
        std::shared_ptr<dsp::CorrectIQBlock<float>> iqc;
        std::shared_ptr<dsp::FIRBlock<float>> rrc;
        std::shared_ptr<dsp::MMClockRecoveryBlock<float>> rec;
        std::shared_ptr<STXDeframerBlock> def;

    public:
        STXDemod(std::shared_ptr<dsp::stream<complex_t>> input, std::function<void(uint8_t *, int)> callback, double samplerate, bool resamp = false);
        void start();
        void stop();

        float getSNR()
        {
            if (def)
                return def->snr;
            else
                return 0;
        }

        int getDefState()
        {
            if (def)
                return def->deframer_state;
            else
                return 0;
        }
    };
} // namespace orbcomm