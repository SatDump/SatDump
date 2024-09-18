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

        bool enable_main = true;

    private:
        struct OutputCFG
        {
            std::shared_ptr<dsp::stream<complex_t>> output_stream;
            bool enabled;
        };

        std::map<std::string, OutputCFG> outputs;

    private:
        struct VfoCFG
        {
            std::shared_ptr<dsp::stream<complex_t>> output_stream;
            bool enabled;
            float freq = 0;
            complex_t phase_delta = 0;
            complex_t phase = complex_t(1, 0);
        };

        std::map<std::string, VfoCFG> vfo_outputs;

    public:
        // Copy outputs
        void add_output(std::string id);
        void del_output(std::string id);
        void reset_output(std::string id);
        std::shared_ptr<dsp::stream<complex_t>> get_output(std::string id);
        void set_enabled(std::string id, bool enable);

        // VFO outputs
        void add_vfo(std::string id, double samplerate, double freq);
        void del_vfo(std::string id);
        void reset_vfo(std::string id);
        std::shared_ptr<dsp::stream<complex_t>> get_vfo_output(std::string id);
        void set_vfo_enabled(std::string id, bool enable);

        // Main output
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