#include "clock_recovery_mm.h"
#include "logger.h"

namespace dsp
{
    CCMMClockRecoveryBlock::CCMMClockRecoveryBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit) : Block(input), d_rec(omega, omegaGain, mu, muGain, omegaLimit)
    {
    }

    void CCMMClockRecoveryBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
            return;
        int d_out = 0;
        try
        {
            d_out = d_rec.work(input_stream->readBuf, nsamples, output_stream->writeBuf);
        }
        catch (std::exception &e)
        {
            logger->error(e.what());
        }
        input_stream->flush();
        output_stream->swap(d_out);
    }

    FFMMClockRecoveryBlock::FFMMClockRecoveryBlock(std::shared_ptr<dsp::stream<float>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit) : Block(input), d_rec(omega, omegaGain, mu, muGain, omegaLimit)
    {
    }

    void FFMMClockRecoveryBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
            return;
        int d_out = 0;
        try
        {
            d_out = d_rec.work(input_stream->readBuf, nsamples, output_stream->writeBuf);
        }
        catch (std::runtime_error &e)
        {
            logger->error(e.what());
        }
        input_stream->flush();
        output_stream->swap(d_out);
    }
}