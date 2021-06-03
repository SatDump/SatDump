#include "fir_filter.h"

namespace dsp
{
    FIRFilterCCF::FIRFilterCCF(int decimation, const std::vector<float> &taps)
    {
        d_fir = new fir_filter_ccf(decimation, taps);
        d_updated = false;
        decimation_m = decimation;
    }

    FIRFilterCCF::~FIRFilterCCF()
    {
        delete d_fir;
    }

    void FIRFilterCCF::set_taps(const std::vector<float> &taps)
    {
        d_fir->set_taps(taps);
        d_updated = true;
    }

    std::vector<float> FIRFilterCCF::taps() const
    {
        return d_fir->taps();
    }

    int FIRFilterCCF::work(std::complex<float> *in, int length, std::complex<float> *out)
    {
        tmp_.insert(tmp_.end(), in, &in[length]);

        if (tmp_.size() <= d_fir->ntaps())
            return 0;

        int process_size = tmp_.size() - d_fir->ntaps();

        if (decimation_m == 1)
            d_fir->filterN(out, tmp_.data(), process_size);
        else
            d_fir->filterNdec(out, tmp_.data(), process_size, decimation_m);

        tmp_.erase(tmp_.begin(), tmp_.end() - d_fir->ntaps());

        return process_size / decimation_m;
    }

    FIRFilterFFF::FIRFilterFFF(int decimation, const std::vector<float> &taps)
    {
        d_fir = new fir_filter_fff(decimation, taps);
        d_updated = false;
        decimation_m = decimation;
    }

    FIRFilterFFF::~FIRFilterFFF()
    {
        delete d_fir;
    }

    void FIRFilterFFF::set_taps(const std::vector<float> &taps)
    {
        d_fir->set_taps(taps);
        d_updated = true;
    }

    std::vector<float> FIRFilterFFF::taps() const
    {
        return d_fir->taps();
    }

    int FIRFilterFFF::work(float *in, int length, float *out)
    {
        tmp_.insert(tmp_.end(), in, &in[length]);

        if (tmp_.size() <= d_fir->ntaps())
            return 0;

        int process_size = tmp_.size() - d_fir->ntaps();

        if (decimation_m == 1)
            d_fir->filterN(out, tmp_.data(), process_size);
        else
            d_fir->filterNdec(out, tmp_.data(), process_size, decimation_m);

        tmp_.erase(tmp_.begin(), tmp_.end() - d_fir->ntaps());

        return process_size / decimation_m;
    }
} // namespace libdsp