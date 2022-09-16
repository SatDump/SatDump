#include "cc_encoder.h"
#include "cc_common.h"
#include <volk/volk.h>
#include <volk/volk_typedefs.h>
#include <cmath>

namespace viterbi
{
    CCEncoder::CCEncoder(int frame_size, int k, int rate, std::vector<int> polys, int start_state)
        : d_rate(rate),
          d_k(k),
          d_polys(polys),
          d_start_state(start_state)
    {
        if (static_cast<size_t>(d_rate) != d_polys.size())
            throw std::runtime_error("cc_encoder: Number of polynomials must be the same as the value of rate");

        if (d_rate < 2)
            throw std::runtime_error("cc_encoder: inverse rate r must be > 2");

        if (k < 2 || k > 31)
            throw std::runtime_error("cc_encoder: constraint length K must in be the range [2, 31]");

        if (d_start_state >= (1u << (d_k - 1)))
            throw std::runtime_error("cc_encoder: start state is invalid; must be in range [0, 2^(K-1)-1] where K is the constraint length");

        if (frame_size < 1)
            throw std::runtime_error("cc_encoder: frame_size must be > 0");

        partab_init();

        d_max_frame_size = frame_size;
        set_frame_size(frame_size);
    }

    CCEncoder::~CCEncoder()
    {
    }

    bool CCEncoder::set_frame_size(unsigned int frame_size)
    {
        bool ret = true;
        if (frame_size > d_max_frame_size)
        {
            frame_size = d_max_frame_size;
            ret = false;
        }

        d_frame_size = frame_size;

        d_output_size = d_rate * d_frame_size;

        return ret;
    }

    double CCEncoder::rate()
    {
        return static_cast<double>(d_rate);
    }

    int CCEncoder::parity(int x)
    {
        x ^= (x >> 16);
        x ^= (x >> 8);
        return parityb(x);
    }

    int CCEncoder::parityb(unsigned char x)
    {
        return Partab[x];
    }

    void CCEncoder::partab_init(void)
    {
        int i, cnt, ti;

        /* Initialize parity lookup table */
        for (i = 0; i < 256; i++)
        {
            cnt = 0;
            ti = i;
            while (ti)
            {
                if (ti & 1)
                    cnt++;
                ti >>= 1;
            }
            Partab[i] = cnt & 1;
        }
    }

    void CCEncoder::work(uint8_t *in, uint8_t *out)
    {
        unsigned int my_state = d_start_state;

        for (unsigned int i = 0; i < d_frame_size; ++i)
        {
            my_state = (my_state << 1) | (in[i] & 1);
            for (unsigned int j = 0; j < d_rate; ++j)
                out[i * d_rate + j] = (d_polys[j] < 0) ^ parity(my_state & abs(d_polys[j])) ? 1 : 0;
        }

        d_start_state = my_state;
    }

    void CCEncoder::work(uint8_t *in, uint8_t *out, int size)
    {
        unsigned int my_state = d_start_state;

        for (int i = 0; i < size; ++i)
        {
            my_state = (my_state << 1) | (in[i] & 1);
            for (unsigned int j = 0; j < d_rate; ++j)
                out[i * d_rate + j] = (d_polys[j] < 0) ^ parity(my_state & abs(d_polys[j])) ? 1 : 0;
        }

        d_start_state = my_state;
    }
}
