#include "rational_resampler.h"
#include <numeric>
#include "firdes.h"

namespace dsp
{
    CCRationalResamplerBlock::CCRationalResamplerBlock(std::shared_ptr<dsp::stream<complex_t>> input, unsigned interpolation, unsigned decimation, std::vector<float> custom_taps)
        : Block(input),
          d_interpolation(interpolation),
          d_decimation(decimation),
          d_ctr(0)
    {
        // Get alignement parameters
        int align = volk_get_alignment();

        // Buffer
        in_buffer = 0;
        int size = 2 * STREAM_BUFFER_SIZE;
        buffer = (complex_t *)volk_malloc(size * sizeof(complex_t), align);
        std::fill(buffer, &buffer[size], 0);

        // Start by reducing the interp and decim by their GCD
        int gcd = std::gcd(interpolation, decimation);
        d_interpolation /= gcd;
        d_decimation /= gcd;

        // Generate taps
        std::vector<float> rtaps = custom_taps.size() > 0 ? custom_taps : firdes::design_resampler_filter_float(d_interpolation, d_decimation, 0.4); // 0.4 = Fractional BW

        // Filter number & tap number
        nfilt = d_interpolation;
        ntaps = rtaps.size() / nfilt;

        // If Ntaps is slightly over 1, add 1 tap
        if (fmod(double(rtaps.size()) / double(nfilt), 1.0) > 0.0)
            ntaps++;

        // Init tap buffers
        taps = (float **)volk_malloc(nfilt * sizeof(float *), align);
        for (int i = 0; i < nfilt; i++)
        {
            this->taps[i] = (float *)volk_malloc(ntaps * sizeof(float), align);
            memset(this->taps[i], 0, ntaps);
        }

        // Setup taps
        for (int i = 0; i < (int)rtaps.size(); i++)
            taps[i % nfilt][(ntaps - 1) - (i / nfilt)] = rtaps[i];
    }

    CCRationalResamplerBlock::~CCRationalResamplerBlock()
    {
        for (int i = 0; i < nfilt; i++)
            volk_free(taps[i]);
        volk_free(taps);
        volk_free(buffer);
    }

    void CCRationalResamplerBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        memcpy(&buffer[ntaps], input_stream->readBuf, nsamples * sizeof(complex_t));
        in_buffer = ntaps + nsamples;
        input_stream->flush();

        int outc = 0;
        for (int i = 0; i < in_buffer - ntaps;)
        {
            volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&output_stream->writeBuf[outc++], (lv_32fc_t *)&buffer[i], taps[d_ctr], ntaps);
            d_ctr += this->d_decimation;
            while (d_ctr >= d_interpolation)
            {
                d_ctr -= d_interpolation;
                i++;
            }
        }

        memmove(&buffer[0], &buffer[in_buffer - ntaps], ntaps * sizeof(complex_t));

        output_stream->swap(outc);
    }
}
