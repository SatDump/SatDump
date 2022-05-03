#pragma once

#include "block.h"
#include "file_source.h"

namespace dsp
{
    class FileSinkBlock : public Block<complex_t, float>
    {
    private:
        std::mutex rec_mutex;

        BasebandType d_sample_format;
        void work();

        bool should_work = false;

        std::ofstream output_file;

        size_t current_size_out = 0;

        int8_t *buffer_s8;
        int16_t *buffer_s16;

    public:
        FileSinkBlock(std::shared_ptr<dsp::stream<complex_t>> input);
        ~FileSinkBlock();

        void set_output_sample_type(BasebandType sample_format)
        {
            d_sample_format = sample_format;
        }

        std::string start_recording(std::string path_without_ext)
        {
            rec_mutex.lock();
            std::string finalt;
            if (d_sample_format == CF_32)
                finalt = path_without_ext + ".f32";
            else if (d_sample_format == IS_16)
                finalt = path_without_ext + ".s16";
            else if (d_sample_format == IS_8)
                finalt = path_without_ext + ".s8";

            current_size_out = 0;

            output_file = std::ofstream(finalt);

            should_work = true;
            rec_mutex.unlock();

            return finalt;
        }

        size_t get_written()
        {
            return current_size_out;
        }

        void stop_recording()
        {
            rec_mutex.lock();
            should_work = false;
            current_size_out = 0;
            output_file.close();
            rec_mutex.unlock();
        }
    };
}