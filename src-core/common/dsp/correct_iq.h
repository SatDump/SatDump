#pragma once

#include "block.h"
#include <deque>

/*
Simple Correct IQ block (removes a DC spike).

This is the simplest implementation I thought of.
A DC component will be a constant offset in the raw samples,
so correcting for it is as simple as getting the average value
(which in a normal situation should be close to 0), and substract it.
*/
namespace dsp
{
    class CorrectIQBlock : public Block<complex_t, complex_t>
    {
    private:
        // Just remove a DC spike with a moving average
        const float alpha = 0.0001;
        const float beta = 1.0f - alpha;
        complex_t acc;

        void work();

    public:
        CorrectIQBlock(std::shared_ptr<dsp::stream<complex_t>> input);
        ~CorrectIQBlock();
    };
}