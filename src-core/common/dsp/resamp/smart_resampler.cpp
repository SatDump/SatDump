#include "smart_resampler.h"
#include "logger.h"
#include <cmath>

namespace dsp
{
    template <typename T>
    SmartResamplerBlock<T>::SmartResamplerBlock(std::shared_ptr<dsp::stream<T>> input, unsigned interpolation, unsigned decimation)
        : Block<T, T>(input),
          d_interpolation(interpolation),
          d_decimation(decimation)
    {
        if (d_decimation > d_interpolation)
        {
            use_decim = false;
            use_resamp = false;

            int best_power = floor(log2(decimation / interpolation));
            double rsamp_in = decimation;
            double fout = interpolation;

            if (best_power > 0)
            {
                int best_decim = std::min<int>(1 << best_power, PowerDecimatorBlock<T>::max_decim());
                rsamp_in = (double)decimation / (double)best_decim;
                pdecim = std::make_unique<PowerDecimatorBlock<T>>(nullptr, best_decim);
                use_decim = true;
                // logger->info("Using Decim ratio {:d}", best_decim);
            }

            // logger->info("SampRate {:f} {:f}", rsamp_in, fout);

            if (rsamp_in != fout)
            {
                double t; // Ensure it's all integer
                while (modf(rsamp_in, &t) != 0 || modf(fout, &t) != 0)
                {
                    rsamp_in *= 10;
                    fout *= 10;
                }

                rresamp = std::make_unique<RationalResamplerBlock<T>>(nullptr, fout, rsamp_in);
                use_resamp = true;
                // logger->info("Using Rsamp {:f} / {:f}", fout, rsamp_in);
            }
        }
        else if (d_decimation < d_interpolation)
        {
            use_decim = false;
            use_resamp = true;

            rresamp = std::make_unique<RationalResamplerBlock<T>>(nullptr, interpolation, decimation);

            // logger->info("Using Rsamp {:d} / {:d}", interpolation, decimation);
        }
        else
        {
            use_decim = false;
            use_resamp = false;
        }
    }

    template <typename T>
    SmartResamplerBlock<T>::~SmartResamplerBlock()
    {
    }

    template <typename T>
    int SmartResamplerBlock<T>::process(T *input, int nsamples, T *output)
    {
        if (use_decim && use_resamp)
        {
            nsamples = pdecim->process(input, nsamples, output);
            nsamples = rresamp->process(output, nsamples, output);
        }
        else if (use_resamp)
        {
            nsamples = rresamp->process(input, nsamples, output);
        }
        else if (use_decim)
        {
            nsamples = pdecim->process(input, nsamples, output);
        }
        else
        {
            memcpy(output, input, nsamples * sizeof(T));
        }

        return nsamples;
    }

    template <typename T>
    void SmartResamplerBlock<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        int outn = process(Block<T, T>::input_stream->readBuf, nsamples, Block<T, T>::output_stream->writeBuf);

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(outn);
    }

    template class SmartResamplerBlock<complex_t>;
    template class SmartResamplerBlock<float>;
}
