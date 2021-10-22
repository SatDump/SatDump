#pragma once

#include "block.h"
#include <vector>

/*
Moving average, averaging the last x samples.
Adapted from GNU Radio's block
*/
namespace dsp
{
    class CCMovingAverageBlock : public Block<complex_t, complex_t>
    {
    private:
        int d_length;
        complex_t d_scale;
        int d_max_iter;
        const unsigned int d_vlen;
        std::vector<complex_t> d_scales;
        std::vector<complex_t> d_sum;

        complex_t *buffer;
        int in_buffer;

        void work();

    public:
        CCMovingAverageBlock(std::shared_ptr<dsp::stream<complex_t>> input, int length, complex_t scale, int max_iter = 4096, unsigned int vlen = 1);
        ~CCMovingAverageBlock();
    };

    class FFMovingAverageBlock : public Block<float, float>
    {
    private:
        int d_length;
        float d_scale;
        int d_max_iter;
        const unsigned int d_vlen;
        std::vector<float> d_scales;
        std::vector<float> d_sum;

        float *buffer;
        int in_buffer;

        void work();

    public:
        FFMovingAverageBlock(std::shared_ptr<dsp::stream<float>> input, int length, float scale, int max_iter = 4096, unsigned int vlen = 1);
        ~FFMovingAverageBlock();
    };
}