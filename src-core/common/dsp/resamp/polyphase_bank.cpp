#include "polyphase_bank.h"
#include <volk/volk.h>

namespace dsp
{
    void PolyphaseBank::init(std::vector<float> rtaps, int nfilt2)
    {
        this->nfilt = nfilt2;
        this->ntaps = (rtaps.size() + nfilt - 1) / nfilt;

        // Get alignement parameters
        int align = volk_get_alignment();

        // If Ntaps is slightly over 1, add 1 tap
        if (fmod(double(rtaps.size()) / double(nfilt), 1.0) > 0.0)
            ntaps++;

        // Init tap buffers
        taps = (float **)volk_malloc(nfilt * sizeof(float *), align);
        for (int i = 0; i < nfilt; i++)
        {
            this->taps[i] = (float *)volk_malloc(ntaps * sizeof(float), align);
            for (int y = 0; y < ntaps; y++)
                this->taps[i][y] = 0;
        }

        // Setup taps
        for (int i = 0; i < nfilt * ntaps; i++)
            taps[(nfilt - 1) - (i % nfilt)][i / nfilt] = (i < (int)rtaps.size()) ? rtaps[i] : 0;

        is_init = true;
    }

    PolyphaseBank::PolyphaseBank() {}

    PolyphaseBank::~PolyphaseBank()
    {
        if (is_init)
        {
            for (int i = 0; i < nfilt; i++)
                volk_free(taps[i]);
            volk_free(taps);
        }
    }
} // namespace dsp
