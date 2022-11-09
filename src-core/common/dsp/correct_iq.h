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
    template <typename T>
    class CorrectIQBlock : public Block<T, T>
    {
    private:
        // Just remove a DC spike with a moving average
        const float alpha = 0.0001;
        const float beta = 1.0f - alpha;
        T acc = 0;

        void work();

    public:
        CorrectIQBlock(std::shared_ptr<dsp::stream<T>> input);
        ~CorrectIQBlock();
    };
}