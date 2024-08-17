#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

#include "file_source.h"
#ifdef BUILD_ZIQ
#include "common/ziq.h"
#endif
#include "common/dsp/io/wav_writer.h"

#include "common/ziq2.h"

#include "logger.h"

namespace ndsp
{
    class FileSink : public ndsp::Block
    {
    private:
        void work();

        std::mutex rec_mutex;

        dsp::BasebandType d_sample_format;

        bool should_work = false;

        std::ofstream output_file;

        uint64_t current_size_out = 0;
        uint64_t current_size_out_raw = 0;

        int8_t *buffer_s8;
        int16_t *buffer_s16;

        int bit_depth = 0;

#ifdef BUILD_ZIQ
        ziq::ziq_cfg ziqcfg;
        std::shared_ptr<ziq::ziq_writer> ziqWriter;
#endif

        float *mag_buffer = nullptr;

        std::unique_ptr<dsp::WavWriter> wav_writer;

    public:
        // Nothing

    public:
        FileSink();
        ~FileSink();

        void set_params(nlohmann::json p = {});
        void start();

        void set_output_sample_type(dsp::BasebandType sample_format)
        {
            d_sample_format = sample_format;
        }

        std::string start_recording(std::string path_without_ext, uint64_t samplerate, int depth = 0, bool override_filename = false) // Depth is only for compressed non-raw formats
        {
            rec_mutex.lock();

            bit_depth = depth;

            std::string finalt;
            if (d_sample_format == dsp::CF_32)
                finalt = path_without_ext + ".cf32";
            else if (d_sample_format == dsp::CS_16)
                finalt = path_without_ext + ".cs16";
            else if (d_sample_format == dsp::CS_8)
                finalt = path_without_ext + ".cs8";
            else if (d_sample_format == dsp::WAV_16)
                finalt = path_without_ext + ".wav";
#ifdef BUILD_ZIQ
            else if (d_sample_format == dsp::ZIQ)
                finalt = path_without_ext + ".ziq";
#endif
            else if (d_sample_format == dsp::ZIQ2)
                finalt = path_without_ext + ".ziq";

            if (override_filename)
                finalt = path_without_ext;

            current_size_out = 0;
            current_size_out_raw = 0;

            output_file = std::ofstream(finalt, std::ios::binary);

            if (d_sample_format == dsp::WAV_16)
            {
                wav_writer = std::make_unique<dsp::WavWriter>(output_file);
                wav_writer->write_header(samplerate, 2);
            }

#ifdef BUILD_ZIQ
            if (d_sample_format == dsp::ZIQ)
            {
                ziqcfg.is_compressed = true;
                ziqcfg.bits_per_sample = depth;
                ziqcfg.samplerate = samplerate;
                ziqcfg.annotation = "";

                ziqWriter = std::make_shared<ziq::ziq_writer>(ziqcfg, output_file);
            }
#endif
#ifdef BUILD_ZIQ2
            if (d_sample_format == dsp::ZIQ2)
            {
                int sz = ziq2::ziq2_write_file_hdr((uint8_t *)buffer_s8, samplerate);
                output_file.write((char *)buffer_s8, sz);

                if (mag_buffer == nullptr)
                    mag_buffer = dsp::create_volk_buffer<float>(1e6); // TODO FIX
            }
#endif

            if (!std::filesystem::exists(finalt))
                logger->error("We have created the output baseband file, but it does not exist! There may be a permission issue! File : " + finalt);

            should_work = true;
            rec_mutex.unlock();

            return finalt;
        }

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
            if (d_sample_format == dsp::WAV_16)
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