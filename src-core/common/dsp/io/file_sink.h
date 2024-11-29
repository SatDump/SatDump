#pragma once

#include "common/dsp/block.h"
#include "file_source.h"
#ifdef BUILD_ZIQ
#include "common/ziq.h"
#endif
#include "wav_writer.h"

#include "common/ziq2.h"

#include "logger.h"

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

        uint64_t current_size_out = 0;
        uint64_t current_size_out_raw = 0;

        int8_t *buffer_s8 = nullptr;
        int16_t *buffer_s16 = nullptr;
        int32_t *buffer_s32 = nullptr;

#ifdef BUILD_ZIQ
        ziq::ziq_cfg ziqcfg;
        std::shared_ptr<ziq::ziq_writer> ziqWriter;
#endif

        float *mag_buffer = nullptr;

        std::unique_ptr<WavWriter> wav_writer;

    public:
        FileSinkBlock(std::shared_ptr<dsp::stream<complex_t>> input);
        ~FileSinkBlock();

        void set_output_sample_type(BasebandType sample_format);
        std::string start_recording(std::string path_without_ext, uint64_t samplerate, bool override_filename = false);

        uint64_t get_written()
        {
            return current_size_out;
        }

        uint64_t get_written_raw()
        {
            return current_size_out_raw;
        }

        void stop_recording()
        {
            if (d_sample_format == WAV_16)
                wav_writer->finish_header(get_written());

            rec_mutex.lock();
            should_work = false;
            current_size_out = 0;
            current_size_out_raw = 0;
            output_file.close();
            rec_mutex.unlock();
        }
    };
}