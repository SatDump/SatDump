#include "splitter.h"

namespace dsp
{
    SplitterBlock::SplitterBlock(std::shared_ptr<dsp::stream<complex_t>> input)
        : Block(input)
    {
    }

    void SplitterBlock::add_output(std::string id)
    {
        state_mutex.lock();
        if (outputs.count(id) == 0)
            outputs.insert({id, {std::make_shared<dsp::stream<complex_t>>(), false}});
        state_mutex.unlock();
    }

    void SplitterBlock::del_output(std::string id)
    {
        state_mutex.lock();
        if (outputs.count(id) > 0)
            outputs.erase(id);
        state_mutex.unlock();
    }

    std::shared_ptr<dsp::stream<complex_t>> SplitterBlock::get_output(std::string id)
    {
        if (outputs.count(id) > 0)
            return outputs[id].output_stream;
        else
            return nullptr;
    }

    void SplitterBlock::set_enabled(std::string id, bool enable)
    {
        state_mutex.lock();
        if (outputs.count(id) > 0)
            outputs[id].enabled = enable;
        state_mutex.unlock();
    }

    void SplitterBlock::reset_output(std::string id)
    {
        state_mutex.lock();
        if (outputs.count(id) > 0)
        {
            outputs[id].output_stream = std::make_shared<dsp::stream<complex_t>>();
            outputs[id].enabled = false;
        }
        state_mutex.unlock();
    }

    void SplitterBlock::set_main_enabled(bool enable)
    {
        state_mutex.lock();
        enable_main = enable;
        state_mutex.unlock();
    }

    void SplitterBlock::work()
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