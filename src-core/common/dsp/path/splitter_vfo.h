#pragma once

#include "common/dsp/block.h"
#include <map>
#include <string>
#include "common/dsp/utils/freq_shift.h"

namespace dsp
{
    class VFOSplitterBlock : public Block<complex_t, complex_t>
    {
    private:
        std::mutex state_mutex;
        void work();

        struct VfoCFG
        {
            std::shared_ptr<dsp::stream<complex_t>> output_stream;
            bool enabled;
            std::shared_ptr<FreqShiftBlock> freq_shiter;
        };

        std::map<std::string, VfoCFG> outputs;

        bool enable_main = true;

    public:
        void add_vfo(std::string id, double samplerate, double freq);
        void del_vfo(std::string id);
        void reset_vfo(std::string id);
        std::shared_ptr<dsp::stream<complex_t>> get_vfo_output(std::string id);
        void set_vfo_enabled(std::string id, bool enable);
        void set_main_enabled(bool enable);

    public:
        VFOSplitterBlock(std::shared_ptr<dsp::stream<complex_t>> input);

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