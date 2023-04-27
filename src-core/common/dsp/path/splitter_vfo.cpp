#include "splitter_vfo.h"

namespace dsp
{
    VFOSplitterBlock::VFOSplitterBlock(std::shared_ptr<dsp::stream<complex_t>> input)
        : Block(input)
    {
    }

    void VFOSplitterBlock::add_vfo(std::string id, double samplerate, double freq)
    {
        state_mutex.lock();
        if (outputs.count(id) == 0)
        {
            auto o = std::make_shared<dsp::stream<complex_t>>();
            outputs.insert({id, {o, false, std::make_shared<FreqShiftBlock>(o, samplerate, freq)}});
            outputs[id].freq_shiter->start();
        }
        state_mutex.unlock();
    }

    void VFOSplitterBlock::del_vfo(std::string id)
    {
        state_mutex.lock();
        if (outputs.count(id) > 0)
        {
            outputs[id].freq_shiter->stop();
            outputs.erase(id);
        }
        state_mutex.unlock();
    }

    std::shared_ptr<dsp::stream<complex_t>> VFOSplitterBlock::get_vfo_output(std::string id)
    {
        if (outputs.count(id) > 0)
            return outputs[id].freq_shiter->output_stream;
        else
            return nullptr;
    }

    void VFOSplitterBlock::set_vfo_enabled(std::string id, bool enable)
    {
        state_mutex.lock();
        if (outputs.count(id) > 0)
            outputs[id].enabled = enable;
        state_mutex.unlock();
    }

    void VFOSplitterBlock::reset_vfo(std::string id)
    {
        state_mutex.lock();
        if (outputs.count(id) > 0)
        {
            outputs[id].output_stream = std::make_shared<dsp::stream<complex_t>>();
            outputs[id].enabled = false;
        }
        state_mutex.unlock();
    }

    void VFOSplitterBlock::set_main_enabled(bool enable)
    {
        state_mutex.lock();
        enable_main = enable;
        state_mutex.unlock();
    }

    void VFOSplitterBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        state_mutex.lock();

        if (enable_main)
            memcpy(output_stream->writeBuf, input_stream->readBuf, nsamples * sizeof(complex_t));

        for (auto &o : outputs)
            if (o.second.enabled)
                memcpy(o.second.output_stream->writeBuf, input_stream->readBuf, nsamples * sizeof(complex_t));

        input_stream->flush();

        if (enable_main)
            output_stream->swap(nsamples);

        for (auto &o : outputs)
            if (o.second.enabled)
                o.second.output_stream->swap(nsamples);

        state_mutex.unlock();
    }
}