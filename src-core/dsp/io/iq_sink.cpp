#include "iq_sink.h"
#include "common/dsp/buffer.h"
#include "common/dsp/io/baseband_type.h"
#include "core/config.h"
#include "core/exception.h"
#include <complex.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <volk/volk_malloc.h>

namespace satdump
{
    namespace ndsp
    {
        IQSinkBlock::IQSinkBlock() : Block("iq_sink_c", {{"in", DSP_SAMPLE_TYPE_CF32}}, {}) {}

        IQSinkBlock::~IQSinkBlock() {}

        void IQSinkBlock::start()
        {
            total_written_raw = 0;
            total_written_bytes_compressed = 0;

            std::string path = filepath;
            if (autogen)
            {
                path += "/" + prepareBasebandFileName(timestamp, samplerate, frequency) + "." + (std::string)format;
            }
            logger->info("Recording IQ to " + path);

            file_stream = fopen(path.c_str(), "wb");
            if (file_stream == NULL)
                throw satdump_exception("Couldn't open IQ file for writing at " + path);

            if (format == CS32)
                buffer_convert = dsp::create_volk_buffer<int32_t>(buffer_size * 2);
            else if (format == CS16)
                buffer_convert = dsp::create_volk_buffer<int16_t>(buffer_size * 2);
            else if (format == CS8)
                buffer_convert = dsp::create_volk_buffer<int8_t>(buffer_size * 2);

            Block::start();
        }

        void IQSinkBlock::stop(bool stop_now, bool force)
        {
            Block::stop(stop_now, force);

            if (file_stream != nullptr)
            {
                fclose(file_stream);
                file_stream = nullptr;

                if (buffer_convert != nullptr)
                {
                    volk_free(buffer_convert);
                    buffer_convert = nullptr;
                }
            }
        }

        bool IQSinkBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            int nsam = iblk.size;
            complex_t *ibuf = iblk.getSamples<complex_t>();

            // TODOREWORK move?
            void *write_ptr = nullptr;
            size_t write_sz = 0;

            if (format == CF32)
            {
                write_ptr = ibuf;
                write_sz = nsam * sizeof(complex_t);
            }
            else if (format == CS32)
            {
                volk_32f_s32f_convert_32i((int32_t *)buffer_convert, (float *)ibuf, 2147483648 /*-1, but float*/, nsam * 2);
                write_ptr = buffer_convert;
                write_sz = nsam * 2 * sizeof(int32_t);
            }
            else if (format == CS16)
            {
                volk_32f_s32f_convert_16i((int16_t *)buffer_convert, (float *)ibuf, 32767, nsam * 2);
                write_ptr = buffer_convert;
                write_sz = nsam * 2 * sizeof(int16_t);
            }
            else if (format == CS8)
            {
                volk_32f_s32f_convert_8i((int8_t *)buffer_convert, (float *)ibuf, 127, nsam * 2);
                write_ptr = buffer_convert;
                write_sz = nsam * 2 * sizeof(int8_t);
            }

            size_t written = fwrite(write_ptr, write_sz, 1, file_stream);
            if (written == 0)
            {
                printf("Error writing!\n"); // TODOREWORK Better info
            }

            total_written_raw += write_sz;

            inputs[0].fifo->free(iblk);

            return false;
        }

        // TODOREWORK?
        std::string IQSinkBlock::prepareBasebandFileName(double timeValue_precise, uint64_t samplerate, uint64_t frequency)
        {
            const time_t timevalue = timeValue_precise;
            std::tm *timeReadable = gmtime(&timevalue);
            std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                                    (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                                    (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                                    (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                                    (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
                                    (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

            if (satdump::satdump_cfg.getValueFromSatDumpUI<bool>("recorder_baseband_filename_millis_precision"))
            {
                std::ostringstream ss;

                double ms_val = fmod(timeValue_precise, 1.0) * 1e3;
                ss << "-" << std::fixed << std::setprecision(0) << std::setw(3) << std::setfill('0') << ms_val;
                timestamp += ss.str();
            }

            return timestamp + "_" + std::to_string(samplerate) + "SPS_" + std::to_string(frequency) + "Hz";
        }

    } // namespace ndsp
} // namespace satdump