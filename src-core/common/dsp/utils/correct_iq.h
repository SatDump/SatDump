#pragma once

#include "common/dsp/block.h"
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
    template <typename T>
    class CorrectIQBlock : public Block<T, T>
    {
    private:
        // Just remove a DC spike with a moving average
        float alpha = 0.0001;
        float beta = 0;
        T acc = 0;

        void work();

    public:
        CorrectIQBlock(std::shared_ptr<dsp::stream<T>> input, float beta = 0.0001);
        ~CorrectIQBlock();
    };
}