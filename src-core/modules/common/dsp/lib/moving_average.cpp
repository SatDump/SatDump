#include "moving_average.h"
#include <algorithm>
#include <numeric>

namespace dsp
{
    template <typename T>
    inline void volk_add(T *out, const T *add, unsigned int num)
    {
        for (unsigned int elem = 0; elem < num; elem++)
        {
            out[elem] += add[elem];
        }
    }

    template <typename T>
    inline void volk_sub(T *out, const T *sub, unsigned int num)
    {
        for (unsigned int elem = 0; elem < num; elem++)
        {
            out[elem] -= sub[elem];
        }
    }

    template <typename T>
    inline void volk_mul(T *out, const T *in, const T *scale, unsigned int num)
    {
        for (unsigned int elem = 0; elem < num; elem++)
        {
            out[elem] = in[elem] * scale[elem];
        }
    }

    MovingAverageCC::MovingAverageCC(int length, std::complex<float> scale, int max_iter, unsigned int vlen)
        : d_length(length),
          d_scale(scale),
          d_max_iter(max_iter),
          d_vlen(vlen),
          d_new_length(length),
          d_new_scale(scale),
          d_updated(false)
    {
        d_history = length;
        // we store this vector so that work() doesn't spend its time allocating and freeing
        // vector storage
        if (d_vlen > 1)
        {
            d_sum = std::vector<std::complex<float>>(d_vlen);
            d_scales = std::vector<std::complex<float>>(d_vlen, d_scale);
        }
    }

    MovingAverageCC::~MovingAverageCC()
    {
    }

    void MovingAverageCC::set_length_and_scale(int length, std::complex<float> scale)
    {
        d_new_length = length;
        d_new_scale = scale;
        d_updated = true;
    }

    void MovingAverageCC::set_length(int length)
    {
        d_new_length = length;
        d_updated = true;
    }

    void MovingAverageCC::set_scale(std::complex<float> scale)
    {
        d_new_scale = scale;
        d_updated = true;
    }

    int MovingAverageCC::work(std::complex<float> *in, int length, std::complex<float> *out)
    {
        tmp_.insert(tmp_.end(), in, &in[length]);

        const unsigned int num_iter = std::min((int)tmp_.size(), d_max_iter);
        if (d_vlen == 1)
        {
            std::complex<float> sum = std::accumulate(&tmp_[0], &tmp_[d_length - 1], std::complex<float>{});

            for (unsigned int i = 0; i < num_iter; i++)
            {
                sum += tmp_[i + d_length - 1];
                out[i] = sum * d_scale;
                sum -= tmp_[i];
            }
        }
        else
        { // d_vlen > 1
            // gets automatically optimized well
            std::copy(&tmp_[0], &tmp_[d_vlen], std::begin(d_sum));

            for (int i = 1; i < d_length - 1; i++)
            {
                volk_add(d_sum.data(), &tmp_[i * d_vlen], d_vlen);
            }

            for (unsigned int i = 0; i < num_iter; i++)
            {
                volk_add(d_sum.data(), &tmp_[(i + d_length - 1) * d_vlen], d_vlen);
                volk_mul(&out[i * d_vlen], d_sum.data(), d_scales.data(), d_vlen);
                volk_sub(d_sum.data(), &tmp_[i * d_vlen], d_vlen);
            }
        }

        tmp_.erase(tmp_.begin(), tmp_.end() - d_history);

        return num_iter;
    }

    MovingAverageFF::MovingAverageFF(int length, float scale, int max_iter, unsigned int vlen)
        : d_length(length),
          d_scale(scale),
          d_max_iter(max_iter),
          d_vlen(vlen),
          d_new_length(length),
          d_new_scale(scale),
          d_updated(false)
    {
        d_history = length;
        // we store this vector so that work() doesn't spend its time allocating and freeing
        // vector storage
        if (d_vlen > 1)
        {
            d_sum = std::vector<float>(d_vlen);
            d_scales = std::vector<float>(d_vlen, d_scale);
        }
    }

    MovingAverageFF::~MovingAverageFF()
    {
    }

    void MovingAverageFF::set_length_and_scale(int length, float scale)
    {
        d_new_length = length;
        d_new_scale = scale;
        d_updated = true;
    }

    void MovingAverageFF::set_length(int length)
    {
        d_new_length = length;
        d_updated = true;
    }

    void MovingAverageFF::set_scale(float scale)
    {
        d_new_scale = scale;
        d_updated = true;
    }

    int MovingAverageFF::work(float *in, int length, float *out)
    {
        if (d_updated)
        {
            d_length = d_new_length;
            d_scale = d_new_scale;
            std::fill(std::begin(d_scales), std::end(d_scales), d_scale);
            d_history = d_length;
            d_updated = false;
        }

        tmp_.insert(tmp_.end(), in, &in[length]);

        const unsigned int num_iter = std::min((int)tmp_.size(), d_max_iter);
        if (d_vlen == 1)
        {
            float sum = std::accumulate(&tmp_[0], &tmp_[d_length - 1], float{});

            for (unsigned int i = 0; i < num_iter; i++)
            {
                sum += tmp_[i + d_length - 1];
                out[i] = sum * d_scale;
                sum -= tmp_[i];
            }
        }
        else
        { // d_vlen > 1
            // gets automatically optimized well
            std::copy(&tmp_[0], &tmp_[d_vlen], std::begin(d_sum));

            for (int i = 1; i < d_length - 1; i++)
            {
                volk_add(d_sum.data(), &tmp_[i * d_vlen], d_vlen);
            }

            for (unsigned int i = 0; i < num_iter; i++)
            {
                volk_add(d_sum.data(), &tmp_[(i + d_length - 1) * d_vlen], d_vlen);
                volk_mul(&out[i * d_vlen], d_sum.data(), d_scales.data(), d_vlen);
                volk_sub(d_sum.data(), &tmp_[i * d_vlen], d_vlen);
            }
        }

        tmp_.erase(tmp_.begin(), tmp_.end() - d_history);

        return num_iter;
    }
} // namespace libdsp