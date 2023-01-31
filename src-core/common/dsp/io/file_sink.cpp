#include "file_sink.h"

namespace dsp
{
    FileSinkBlock::FileSinkBlock(std::shared_ptr<dsp::stream<complex_t>> input)
        : Block(input)
    {
        buffer_s8 = create_volk_buffer<int8_t>(STREAM_BUFFER_SIZE * 2);
        buffer_s16 = create_volk_buffer<int16_t>(STREAM_BUFFER_SIZE * 2);
    }

    FileSinkBlock::~FileSinkBlock()
    {
        volk_free(buffer_s8);
        volk_free(buffer_s16);
    }

    void FileSinkBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0 || !should_work)
        {
            input_stream->flush();
            return;
        }

        rec_mutex.lock();
        if (should_work)
        {
            if (d_sample_format == CF_32)
            {
                output_file.write((char *)input_stream->readBuf, nsamples * sizeof(complex_t));
                current_size_out += nsamples * sizeof(complex_t);
            }
            else if (d_sample_format == IS_16 || d_sample_format == WAV_16)
            {
                volk_32f_s32f_convert_16i(buffer_s16, (float *)input_stream->readBuf, 65535, nsamples * 2);
                output_file.write((char *)buffer_s16, nsamples * sizeof(int16_t) * 2);
                current_size_out += nsamples * sizeof(int16_t) * 2;
            }
            else if (d_sample_format == IS_8)
            {
                volk_32f_s32f_convert_8i(buffer_s8, (float *)input_stream->readBuf, 127, nsamples * 2);
                output_file.write((char *)buffer_s8, nsamples * sizeof(int8_t) * 2);
                current_size_out += nsamples * sizeof(int8_t) * 2;
            }
#ifdef BUILD_ZIQ
            else if (d_sample_format == ZIQ)
            {
                current_size_out += ziqWriter->write(input_stream->readBuf, nsamples);
                current_size_out_raw += (ziqcfg.bits_per_sample / 4) * nsamples;
            }
#endif

            input_stream->flush();

            output_file.flush();
        }

        rec_mutex.unlock();
    }
}