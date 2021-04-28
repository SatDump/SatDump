#include "rational_resampler.h"
#include "fir_gen.h"

namespace dsp
{
    /**
 * @brief Given the interpolation rate, decimation rate and a fractional bandwidth,
    design a set of taps.
 *
 * Uses default parameters to build a low pass filter using a Kaiser Window
 *
 * @param interpolation interpolation factor (integer > 0)
 * @param decimation decimation factor (integer > 0)
 * @param fractional_bw fractional bandwidth in (0, 0.5)  0.4 works well. (float)
 */
    std::vector<float> design_resampler_filter(const unsigned interpolation,
                                               const unsigned decimation,
                                               const float fractional_bw)
    {
        if (fractional_bw >= 0.5 || fractional_bw <= 0)
        {
            // throw std::range_error("Invalid fractional_bandwidth, must be in (0, 0.5)");
        }

        // These are default values used to generate the filter when no taps are known
        //  Pulled from rational_resampler.py
        float beta = 7.0;
        float halfband = 0.5;
        float rate = float(interpolation) / float(decimation);
        float trans_width, mid_transition_band;

        if (rate >= 1.0)
        {
            trans_width = halfband - fractional_bw;
            mid_transition_band = halfband - trans_width / 2.0;
        }
        else
        {
            trans_width = rate * (halfband - fractional_bw);
            mid_transition_band = rate * halfband - trans_width / 2.0;
        }

        return firgen::low_pass(interpolation,       /* gain */
                                interpolation,       /* Fs */
                                mid_transition_band, /* trans mid point */
                                trans_width,         /* transition width */
                                fft::window::WIN_KAISER,
                                beta); /* beta*/
    }

    RationalResamplerCCF::RationalResamplerCCF(unsigned interpolation, unsigned decimation) : d_history(1),
                                                                                              d_decimation(decimation),
                                                                                              d_ctr(0),
                                                                                              d_updated(false)
    {
        if (interpolation == 0)
            throw std::out_of_range(
                "rational_resampler_base_ccf: interpolation must be > 0");
        if (decimation == 0)
            throw std::out_of_range(
                "rational_resampler_base_ccf: decimation must be > 0");

        d_firs.reserve(interpolation);
        for (unsigned i = 0; i < interpolation; i++)
        {
            d_firs.emplace_back(1, std::vector<float>());
        }

        std::vector<float> taps = design_resampler_filter(interpolation, decimation, 0);
        set_taps(taps);
        install_taps(d_new_taps);
    }

    RationalResamplerCCF::RationalResamplerCCF(unsigned interpolation, unsigned decimation, const std::vector<float> &taps) : d_history(1),
                                                                                                                              d_decimation(decimation),
                                                                                                                              d_ctr(0),
                                                                                                                              d_updated(false)
    {
        if (interpolation == 0)
            throw std::out_of_range(
                "rational_resampler_base_ccf: interpolation must be > 0");
        if (decimation == 0)
            throw std::out_of_range(
                "rational_resampler_base_ccf: decimation must be > 0");

        d_firs.reserve(interpolation);
        for (unsigned i = 0; i < interpolation; i++)
        {
            d_firs.emplace_back(1, std::vector<float>());
        }

        set_taps(taps);
        install_taps(d_new_taps);
    }

    void RationalResamplerCCF::install_taps(const std::vector<float> &taps)
    {
        int nfilters = this->interpolation();
        int nt = taps.size() / nfilters;

        std::vector<std::vector<float>> xtaps(nfilters);

        for (int n = 0; n < nfilters; n++)
            xtaps[n].resize(nt);

        for (int i = 0; i < (int)taps.size(); i++)
            xtaps[i % nfilters][i / nfilters] = taps[i];

        for (int n = 0; n < nfilters; n++)
            d_firs[n].set_taps(xtaps[n]);

        d_history = nt;
        d_updated = false;
    }

    void RationalResamplerCCF::set_taps(const std::vector<float> &taps)
    {
        d_new_taps = taps;
        d_updated = true;

        // round up length to a multiple of the interpolation factor
        int n = taps.size() % this->interpolation();
        if (n > 0)
        {
            n = this->interpolation() - n;
            while (n-- > 0)
            {
                d_new_taps.insert(d_new_taps.end(), 0);
            }
        }
    }

    std::vector<float> RationalResamplerCCF::taps() const
    {
        return d_new_taps;
    }

    int RationalResamplerCCF::work(std::complex<float> *in, int length, std::complex<float> *out)
    {
        tmp_.insert(tmp_.end(), in, &in[length]);

        unsigned int ctr = d_ctr;
        int count = 0;

        int i = 0;
        while (count < tmp_.size())
        {
            out[i++] = d_firs[ctr].filter(in);
            ctr += this->decimation();
            while (ctr >= this->interpolation())
            {
                ctr -= this->interpolation();
                in++;
                count++;
            }
        }

        d_ctr = ctr;

        tmp_.erase(tmp_.begin(), tmp_.end() - d_history);

        return i;
    }
}; // namespace libdsp