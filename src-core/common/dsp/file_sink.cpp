#include "file_sink.h"

namespace dsp
{
    FileSinkBlock::FileSinkBlock(std::shared_ptr<dsp::stream<complex_t>> input)
        : Block(input)
    {
        buffer_s8 = create_volk_buffer<int8_t>(STREAM_BUFFER_SIZE * 2);
        buffer_s16 = create_volk_buffer<int16_t>(STREAM_BUFFER_SIZE * 2);

        mag_buffer = create_volk_buffer<float>(STREAM_BUFFER_SIZE);
    }

    FileSinkBlock::~FileSinkBlock()
    {
        volk_free(buffer_s8);
        volk_free(buffer_s16);
        volk_free(mag_buffer);
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
            else if (d_sample_format == ZIQ2)
            {
                uint32_t max_index = 0;
                volk_32fc_magnitude_32f(mag_buffer, (lv_32fc_t *)input_stream->readBuf, nsamples);
                volk_32f_index_max_32u(&max_index, mag_buffer, nsamples);

                int bit_depth = 8;

                float scale = 0;

                if (bit_depth == 8)
                    scale = 127.0f / mag_buffer[max_index];
                else if (bit_depth == 16)
                    scale = 65535.0f / mag_buffer[max_index];

                buffer_s8[0] = 0x1a;
                buffer_s8[1] = 0xcf;
                buffer_s8[2] = 0xfc;
                buffer_s8[3] = 0x1d;
                *((uint8_t *)&buffer_s8[4]) = 1;
                *((uint8_t *)&buffer_s8[5]) = bit_depth;
                *((uint32_t *)&buffer_s8[6]) = nsamples;
                *((float *)&buffer_s8[10]) = scale;

                int final_size = 14;

                if (bit_depth == 8)
                {
                    volk_32f_s32f_convert_8i((int8_t *)&buffer_s8[14], (float *)input_stream->readBuf, scale, nsamples * 2);
                    final_size += nsamples * sizeof(int8_t) * 2;
                }
                else if (bit_depth == 16)
                {
                    volk_32f_s32f_convert_16i((int16_t *)&buffer_s8[14], (float *)input_stream->readBuf, scale, nsamples * 2);
                    final_size += nsamples * sizeof(int16_t) * 2;
                }

                output_file.write((char *)buffer_s8, final_size);
                current_size_out += final_size;
            }

            input_stream->flush();

            output_file.flush();
        }

        rec_mutex.unlock();
    }
}