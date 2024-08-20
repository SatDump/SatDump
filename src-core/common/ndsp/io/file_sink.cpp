#include "file_sink.h"
#include "common/dsp/block.h"

namespace ndsp
{
    FileSink::FileSink()
        : ndsp::Block("file_sink", {{sizeof(complex_t)}}, {})
    {
        buffer_s8 = dsp::create_volk_buffer<int8_t>(1e6 * 2);   // TODO FIX
        buffer_s16 = dsp::create_volk_buffer<int16_t>(1e6 * 2); // TODO FIX
    }

    FileSink::~FileSink()
    {
        volk_free(buffer_s8);
        volk_free(buffer_s16);
        if (mag_buffer != nullptr)
            volk_free(mag_buffer);
    }

    void FileSink::start()
    {
        set_params();
        //        ndsp::buf::init_nafifo_stdbuf<complex_t>(outputs[0], 2, ((ndsp::buf::StdBuf<complex_t> *)inputs[0]->write_buf())->max);
        ndsp::Block::start();
    }

    void FileSink::set_params(nlohmann::json p)
    {
    }

    void FileSink::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<complex_t> *)inputs[0]->read_buf();

            int nsamples = rbuf->cnt;

            if (nsamples <= 0 || !should_work)
            {
                inputs[0]->flush();
                return;
            }

            rec_mutex.lock();
            if (should_work)
            {
                if (d_sample_format == dsp::CF_32)
                {
                    output_file.write((char *)rbuf->dat, nsamples * sizeof(complex_t));
                    current_size_out += nsamples * sizeof(complex_t);
                }
                else if (d_sample_format == dsp::CS_16 || d_sample_format == dsp::WAV_16)
                {
                    volk_32f_s32f_convert_16i(buffer_s16, (float *)rbuf->dat, 65535, nsamples * 2);
                    output_file.write((char *)buffer_s16, nsamples * sizeof(int16_t) * 2);
                    current_size_out += nsamples * sizeof(int16_t) * 2;
                }
                else if (d_sample_format == dsp::CS_8)
                {
                    volk_32f_s32f_convert_8i(buffer_s8, (float *)rbuf->dat, 127, nsamples * 2);
                    output_file.write((char *)buffer_s8, nsamples * sizeof(int8_t) * 2);
                    current_size_out += nsamples * sizeof(int8_t) * 2;
                }
#ifdef BUILD_ZIQ
                else if (d_sample_format == dsp::ZIQ)
                {
                    current_size_out += ziqWriter->write(rbuf->dat, nsamples);
                    current_size_out_raw += (ziqcfg.bits_per_sample / 4) * nsamples;
                }
#endif
#ifdef BUILD_ZIQ2
                else if (d_sample_format == dsp::ZIQ2)
                {
                    int sz = ziq2::ziq2_write_iq_pkt((uint8_t *)buffer_s8, rbuf->dat, mag_buffer, nsamples, d_sample_format.ziq_depth);
                    output_file.write((char *)buffer_s8, sz);
                    current_size_out += sz;
                }
#endif

                inputs[0]->flush();

                output_file.flush();
            }

            rec_mutex.unlock();
        }
    }
}
