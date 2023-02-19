#pragma once

#include "common/dsp/block.h"
#include <map>
#include <string>

namespace dsp
{
    class SplitterBlock : public Block<complex_t, complex_t>
    {
    private:
        std::mutex state_mutex;
        void work();

        struct OutputCFG
        {
            std::shared_ptr<dsp::stream<complex_t>> output_stream;
            bool enabled;
        };

        std::map<std::string, OutputCFG> outputs;

        bool enable_main = true;

    public:
        void add_output(std::string id);
        void del_output(std::string id);
        void reset_output(std::string id);
        std::shared_ptr<dsp::stream<complex_t>> get_output(std::string id);
        void set_enabled(std::string id, bool enable);
        void set_main_enabled(bool enable);

    public:
        SplitterBlock(std::shared_ptr<dsp::stream<complex_t>> input);

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