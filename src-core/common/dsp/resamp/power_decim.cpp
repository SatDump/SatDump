#include "power_decim.h"
#include <numeric>
#include "power_decim/plans.h"

namespace dsp
{
    template <typename T>
    PowerDecimatorBlock<T>::PowerDecimatorBlock(std::shared_ptr<dsp::stream<T>> input, unsigned decimation)
        : Block<T, T>(input),
          d_decimation(decimation)
    {
        if (d_decimation > 1)
        {
            int id = log2(d_decimation) - 1;

            if (id > power_decim::plans_len)
                throw std::runtime_error("Power Decimator Plan ID over 13!");
            if ((d_decimation & (d_decimation - 1)) != 0)
                throw std::runtime_error("Power Decimator Plan decimation is NOT a power of 2!");

            power_decim::plan plan = power_decim::plans[id];

            for (int i = 0; i < plan.stageCount; i++)
            {
                auto &s = plan.stages[i];
                std::vector<float> taps = std::vector<float>(s.taps, s.taps + s.tapcount);
                fir_stages.push_back(std::make_unique<DecimatingFIRBlock<T>>(nullptr, taps, s.decimation));
            }
        }
    }

    template <typename T>
    PowerDecimatorBlock<T>::~PowerDecimatorBlock()
    {
    }

    template <typename T>
    int PowerDecimatorBlock<T>::process(T *input, int nsamples, T *output)
    {
        if (d_decimation == 1)
        {
            memcpy(output, input, nsamples * sizeof(T));
            return nsamples;
        }

        T *curr_data = input;

        for (int i = 0; i < (int)fir_stages.size(); i++)
        {
            nsamples = fir_stages[i]->process(curr_data, nsamples, output);
            curr_data = output;
        }

        return nsamples;
    }

    template <typename T>
    void PowerDecimatorBlock<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        int outn = process(Block<T, T>::input_stream->readBuf, nsamples, Block<T, T>::output_stream->writeBuf);

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(outn);
    }

    template <typename T>
    int PowerDecimatorBlock<T>::max_decim()
    {
        return 1 << power_decim::plans_len;
    }

    template class PowerDecimatorBlock<complex_t>;
    template class PowerDecimatorBlock<float>;
}
