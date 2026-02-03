#include "samplerate_meter.h"
#include "dsp/block_helpers.h"
#include "utils/time.h"
#include <cstring>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        SamplerateMeterBlock<T>::SamplerateMeterBlock()
            : BlockSimple<T, T>("samplerate_meter_" + getShortTypeName<T>() + getShortTypeName<T>(), {{"in", getTypeSampleType<T>()}}, {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        SamplerateMeterBlock<T>::~SamplerateMeterBlock()
        {
        }

        template <typename T>
        uint32_t SamplerateMeterBlock<T>::process(T *input, uint32_t nsamples, T *output)
        {
            double ctime = getTime();

            memcpy(output, input, nsamples * sizeof(T));

            samplecount += nsamples;
            double time_delta = ctime - last_timestamp;

            if (time_delta > 1.0)
            {
                if (last_timestamp != 0)
                    measured_samplerate = samplecount / time_delta;

                samplecount = 0;
                last_timestamp = ctime;
            }

            return nsamples;
        }

        template class SamplerateMeterBlock<float>;
        template class SamplerateMeterBlock<complex_t>;
    } // namespace ndsp
} // namespace satdump
