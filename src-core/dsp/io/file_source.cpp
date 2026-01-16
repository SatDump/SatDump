#include "file_source.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        FileSourceBlock<T>::FileSourceBlock()
            : Block("file_source_" + getShortTypeName<T>(), {}, //
                    {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        FileSourceBlock<T>::~FileSourceBlock()
        {
        }

        template <typename T>
        bool FileSourceBlock<T>::work()
        {
            if (!file_reader.eof() && !work_should_exit)
            {
                auto oblk = outputs[0].fifo->newBufferSamples(d_buffer_size, sizeof(T));
                T *obuf = oblk.template getSamples<T>();

                file_reader.read((char *)obuf, d_buffer_size);

                oblk.size = d_buffer_size;
                outputs[0].fifo->wait_enqueue(oblk);

                d_progress = file_reader.tellg();
            }
            else
            {
                d_eof = true;
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                return true;
            }

            return false;
        }

        template class FileSourceBlock<complex_t>;
        template class FileSourceBlock<float>;
        template class FileSourceBlock<int16_t>;
        template class FileSourceBlock<int8_t>;
        template class FileSourceBlock<uint8_t>;
    } // namespace ndsp
} // namespace satdump