#pragma once

#include <vector>
#include <complex>

namespace dsp
{
    class MovingAverageCC
    {
    private:
        int d_length;
        std::complex<float> d_scale;
        std::vector<std::complex<float>> d_scales;
        int d_max_iter;
        const unsigned int d_vlen;
        std::vector<std::complex<float>> d_sum;

        int d_new_length;
        std::complex<float> d_new_scale;
        bool d_updated;

        std::vector<std::complex<float>> tmp_;
        int d_history;

    public:
        MovingAverageCC(int length, std::complex<float> scale, int max_iter = 4096, unsigned int vlen = 1);
        ~MovingAverageCC();

        int length() const { return d_new_length; }
        std::complex<float> scale() const { return d_new_scale; }
        unsigned int vlen() const { return d_vlen; }

        void set_length_and_scale(int length, std::complex<float> scale);
        void set_length(int length);
        void set_scale(std::complex<float> scale);

        int work(std::complex<float> *in, int length, std::complex<float> *out);
    };

    class MovingAverageFF
    {
    private:
        int d_length;
        float d_scale;
        std::vector<float> d_scales;
        int d_max_iter;
        const unsigned int d_vlen;
        std::vector<float> d_sum;

        int d_new_length;
        float d_new_scale;
        bool d_updated;

        std::vector<float> tmp_;
        int d_history;

    public:
        MovingAverageFF(int length, float scale, int max_iter = 4096, unsigned int vlen = 1);
        ~MovingAverageFF();

        int length() const { return d_new_length; }
        float scale() const { return d_new_scale; }
        unsigned int vlen() const { return d_vlen; }

        void set_length_and_scale(int length, float scale);
        void set_length(int length);
        void set_scale(float scale);

        int work(float *in, int length, float *out);
    };
} // namespace libdsp