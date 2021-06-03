#pragma once

#include "common/dsp/buffer.h"
#include <thread>
#include <memory>

namespace dsp
{
    template <typename IN_T, typename OUT_T>
    class Block
    {
    protected:
        std::thread d_thread;
        bool should_run;
        virtual void work() = 0;
        bool d_got_input;
        void run()
        {
            while (should_run)
                work();
        }

    public:
        std::shared_ptr<dsp::stream<IN_T>> input_stream;
        std::shared_ptr<dsp::stream<OUT_T>> output_stream;

    public:
        Block(std::shared_ptr<dsp::stream<IN_T>> input) : input_stream(input)
        {
            d_got_input = true;
            output_stream = std::make_shared<dsp::stream<OUT_T>>();
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

            if (d_got_input)
                input_stream->stopReader();
            output_stream->stopWriter();

            if (d_thread.joinable())
                d_thread.join();
        }
    };
}