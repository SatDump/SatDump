#include "stx_demod_hier.h"

#include "common/dsp/filter/firdes.h"

namespace orbcomm
{
    STXDemod::STXDemod(std::shared_ptr<dsp::stream<complex_t>> input, std::function<void(uint8_t *, int)> callback, double samplerate, bool resamp)
        : HierBlock(input)
    {
        const double symrate = 4800;
        const double target_samp = 48e3;

        if (resamp)
            res = std::make_shared<dsp::SmartResamplerBlock<complex_t>>(input, (uint64_t)target_samp, (uint64_t)samplerate);

        // AGC
        agc = std::make_shared<dsp::AGCBlock<complex_t>>(resamp ? res->output_stream : input, 1e-2, 1.0f, 1.0f, 65536);

        // Quadrature demod
        qua = std::make_shared<dsp::QuadratureDemodBlock>(agc->output_stream, 1);

        // DC Blocking
        iqc = std::make_shared<dsp::CorrectIQBlock<float>>(qua->output_stream);

        // Filter
        rrc = std::make_shared<dsp::FIRBlock<float>>(iqc->output_stream, dsp::firdes::root_raised_cosine(1.0, target_samp, symrate, 0.4, 31));

        // Recovery
        float d_clock_gain_omega = 0.25 * 0.175 * 0.175;
        float d_clock_mu = 0.5f;
        float d_clock_gain_mu = 0.175;
        float d_clock_omega_relative_limit = 0.005f;
        rec = std::make_shared<dsp::MMClockRecoveryBlock<float>>(rrc->output_stream, target_samp / symrate, d_clock_gain_omega, d_clock_mu, d_clock_gain_mu, d_clock_omega_relative_limit);

        // Deframer
        def = std::make_shared<STXDeframerBlock>(rec->output_stream);
        def->callback = callback;
    }

    void STXDemod::start()
    {
        if (res)
            res->start();
        agc->start();
        qua->start();
        iqc->start();
        rrc->start();
        rec->start();
        def->start();
    }

    void STXDemod::stop()
    {
        if (res)
            res->stop();
        agc->stop();
        qua->stop();
        iqc->stop();
        rrc->stop();
        rec->stop();
        def->stop();
        def->output_stream->stopReader();
        def->output_stream->stopWriter();
    }
}