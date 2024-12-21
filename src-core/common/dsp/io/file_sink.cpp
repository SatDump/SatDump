#include "file_sink.h"

namespace dsp
{
    FileSinkBlock::FileSinkBlock(std::shared_ptr<dsp::stream<complex_t>> input)
        : Block(input)
    {
    }

    FileSinkBlock::~FileSinkBlock()
    {
        if(buffer_s8 != nullptr)
            volk_free(buffer_s8);
        if(buffer_s16 != nullptr)
            volk_free(buffer_s16);
        if (buffer_s32 != nullptr)
            volk_free(buffer_s32);
        if (mag_buffer != nullptr)
            volk_free(mag_buffer);
    }

    void FileSinkBlock::set_output_sample_type(BasebandType sample_format)
    {
        if (d_sample_format == sample_format)
            return;

        // Free old buffer
        if (d_sample_format == CS_8 && buffer_s8 != nullptr)
        {
            volk_free(buffer_s8);
            buffer_s8 = nullptr;
        }
        else if ((d_sample_format == CS_16 || d_sample_format == WAV_16) && buffer_s16 != nullptr)
        {
            volk_free(buffer_s16);
            buffer_s16 = nullptr;
        }
        else if (d_sample_format == CS_32 && buffer_s32 != nullptr)
        {
            volk_free(buffer_s32);
            buffer_s32 = nullptr;
        }
#ifdef BUILD_ZIQ2
        else if (d_sample_format == ZIQ2 && buffer_s8 != nullptr)
        {
            volk_free(buffer_s8);
            buffer_s8 = nullptr;
        }
#endif

        // Alloc new buffer
        if (sample_format == CS_8)
            buffer_s8 = create_volk_buffer<int8_t>(STREAM_BUFFER_SIZE * 2);
        else if (sample_format == CS_16 || sample_format == WAV_16)
            buffer_s16 = create_volk_buffer<int16_t>(STREAM_BUFFER_SIZE * 2);
        else if (sample_format == CS_32)
            buffer_s32 = create_volk_buffer<int32_t>(STREAM_BUFFER_SIZE * 2);
#ifdef BUILD_ZIQ2
        else if (sample_format == ZIQ2)
            buffer_s8 = create_volk_buffer<int8_t>(STREAM_BUFFER_SIZE * 2);
#endif

        d_sample_format = sample_format;
    }

    std::string FileSinkBlock::start_recording(std::string path_without_ext, uint64_t samplerate, bool override_filename)
    {
        rec_mutex.lock();

        std::string finalt;
        finalt = path_without_ext + "." + (std::string)d_sample_format;
        if (override_filename)
            finalt = path_without_ext;

        current_size_out = 0;
        current_size_out_raw = 0;

        output_file = std::ofstream(finalt, std::ios::binary);

        if (d_sample_format == WAV_16)
        {
            wav_writer = std::make_unique<WavWriter>(output_file);
            wav_writer->write_header(samplerate, 2);
        }

#ifdef BUILD_ZIQ
        if (d_sample_format == ZIQ)
        {
            ziqcfg.is_compressed = true;
            ziqcfg.bits_per_sample = d_sample_format.ziq_depth;
            ziqcfg.samplerate = samplerate;
            ziqcfg.annotation = "";

            ziqWriter = std::make_shared<ziq::ziq_writer>(ziqcfg, output_file);
        }
#endif
#ifdef BUILD_ZIQ2
        if (d_sample_format == ZIQ2)
        {
            int sz = ziq2::ziq2_write_file_hdr((uint8_t*)buffer_s8, samplerate);
            output_file.write((char*)buffer_s8, sz);

            if (mag_buffer == nullptr)
                mag_buffer = create_volk_buffer<float>(STREAM_BUFFER_SIZE);
        }
#endif

        if (!std::filesystem::exists(finalt))
            logger->error("We have created the output baseband file, but it does not exist! There may be a permission issue! File : " + finalt);

        should_work = true;
        rec_mutex.unlock();

        return finalt;
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
            else if (d_sample_format == CS_32)
            {
                volk_32f_s32f_convert_32i(buffer_s32, (float*)input_stream->readBuf, 2147483647, nsamples * 2);
                output_file.write((char*)buffer_s32, nsamples * sizeof(int32_t) * 2);
                current_size_out += nsamples * sizeof(int32_t) * 2;
            }
            else if (d_sample_format == CS_16 || d_sample_format == WAV_16)
            {
                volk_32f_s32f_convert_16i(buffer_s16, (float *)input_stream->readBuf, 32767, nsamples * 2);
                output_file.write((char *)buffer_s16, nsamples * sizeof(int16_t) * 2);
                current_size_out += nsamples * sizeof(int16_t) * 2;
            }
            else if (d_sample_format == CS_8)
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
#ifdef BUILD_ZIQ2
            else if (d_sample_format == ZIQ2)
            {
                int sz = ziq2::ziq2_write_iq_pkt((uint8_t *)buffer_s8, input_stream->readBuf, mag_buffer, nsamples, d_sample_format.ziq_depth);
                output_file.write((char *)buffer_s8, sz);
                current_size_out += sz;
            }
#endif

            input_stream->flush();

            output_file.flush();
        }

        rec_mutex.unlock();
    }
}