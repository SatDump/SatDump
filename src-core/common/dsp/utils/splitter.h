#pragma once

#include "common/dsp/block.h"

namespace dsp
{
    class SplitterBlock : public Block<complex_t, complex_t>
    {
    private:
        std::mutex state_mutex;
        bool enable_second = false;
        bool enable_third = false;
        void work();

    public:
        std::shared_ptr<dsp::stream<complex_t>> output_stream_2;
        std::shared_ptr<dsp::stream<complex_t>> output_stream_3;

    public:
        SplitterBlock(std::shared_ptr<dsp::stream<complex_t>> input);

        void set_output_2nd(bool value)
        {
            state_mutex.lock();
            enable_second = value;
            state_mutex.unlock();
        }

        void set_output_3rd(bool value)
        {
            state_mutex.lock();
            enable_third = value;
            state_mutex.unlock();
        }

        void stop_tmp()
        {
            should_run = false;

            if (d_got_input && input_stream.use_count())
                input_stream->stopReader();
            // output_stream->stopWriter();

            if (d_thread.joinable())
                d_thread.join();
        }
    };
}