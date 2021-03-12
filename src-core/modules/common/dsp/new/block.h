#pragma once

#include "modules/buffer.h"
#include <thread>

namespace dsp
{
    template <typename IN_T, typename OUT_T>
    class Block
    {
    protected:
        std::thread d_thread;
        bool should_run;
        virtual void work() = 0;
        void run()
        {
            while (should_run)
                work();
        }

    public:
        dsp::stream<IN_T> &input_stream;
        dsp::stream<OUT_T> output_stream;

    public:
        Block(dsp::stream<IN_T> &input) : input_stream(input)
        {
        }
        ~Block()
        {
        }
        void start()
        {
            should_run = true;
            d_thread = std::thread(&Block::run, this);
        }
        void stop()
        {
            should_run = false;

            input_stream.stopReader();
            output_stream.stopWriter();

            if (d_thread.joinable())
                d_thread.join();
        }
    };
}