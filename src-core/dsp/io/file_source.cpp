#include "file_source.h"

namespace satdump
{
    namespace ndsp
    {
        FileSourceBlock::FileSourceBlock() : Block("file_source_cc", {}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        FileSourceBlock::~FileSourceBlock() {}

        bool FileSourceBlock::work()
        {
            if (!baseband_reader.is_eof() && !work_should_exit)
            {
                auto oblk = outputs[0].fifo->newBufferSamples(d_buffer_size, sizeof(complex_t));
                complex_t *obuf = oblk.getSamples<complex_t>();

                int read = baseband_reader.read_samples(obuf, d_buffer_size);

                if (d_iq_swap)
                    for (int i = 0; i < read; i++)
                        obuf[i] = complex_t(obuf[i].imag, obuf[i].real);

                oblk.size = read;
                outputs[0].fifo->wait_enqueue(oblk);

                d_progress = baseband_reader.progress;
            }
            else
            {
                d_eof = true;
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                return true;
            }

            return false;
        }
    } // namespace ndsp
} // namespace satdump