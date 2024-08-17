#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

/*
Simple Correct IQ block (removes a DC spike).

This is the simplest implementation I thought of.
A DC component will be a constant offset in the raw samples,
so correcting for it is as simple as getting the average value
(which in a normal situation should be close to 0), and substract it.
*/
namespace ndsp
{
    template <typename T>
    class CorrectIQ : public ndsp::Block
    {
    private:
        void work();

        // Just remove a DC spike with a moving average
        float alpha = 0.0001;
        float beta = 0;
        T acc = 0;

    public:
        float d_alpha = 0.0001;

    public:
        CorrectIQ();
        ~CorrectIQ();

        void set_params(nlohmann::json p = {});
        void start();
    };
}