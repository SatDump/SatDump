#include "splitter.h"

namespace ndsp
{
    Splitter::Splitter()
        : ndsp::Block("splitter", {{sizeof(complex_t)}}, {{sizeof(complex_t)}})
    {
    }

    void Splitter::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<complex_t>(outputs[0], 2, ((ndsp::buf::StdBuf<complex_t> *)inputs[0]->write_buf())->max);
        for (auto &c : c_outputs)
            ndsp::buf::init_nafifo_stdbuf<complex_t>(c.second.output_stream, 2, ((ndsp::buf::StdBuf<complex_t> *)inputs[0]->write_buf())->max);
        ndsp::Block::start();
    }

    void Splitter::stop()
    {
        for (auto &c : c_outputs)
            c.second.output_stream->stop();
        ndsp::Block::stop();
    }

    void Splitter::set_params(nlohmann::json p)
    {
        //        if (p.contains("order"))
        //            d_order = p["order"];
    }

    // Normal Copy
    void Splitter::add_output(std::string id)
    {
        state_mutex.lock();
        if (c_outputs.count(id) == 0)
        {
            c_outputs.insert({id, {std::make_shared<NaFiFo>(), false}});

            if (is_running())
                ndsp::buf::init_nafifo_stdbuf<complex_t>(c_outputs[id].output_stream, 2, ((ndsp::buf::StdBuf<complex_t> *)inputs[0]->write_buf())->max);
        }
        state_mutex.unlock();
    }

    void Splitter::del_output(std::string id)
    {
        state_mutex.lock();
        if (c_outputs.count(id) > 0)
            c_outputs.erase(id);
        state_mutex.unlock();
    }

    std::shared_ptr<NaFiFo> Splitter::sget_output(std::string id)
    {
        if (c_outputs.count(id) > 0)
            return c_outputs[id].output_stream;
        else
            return nullptr;
    }

    void Splitter::set_enabled(std::string id, bool enable)
    {
        state_mutex.lock();
        if (c_outputs.count(id) > 0)
            c_outputs[id].enabled = enable;
        state_mutex.unlock();
    }

    void Splitter::reset_output(std::string id)
    {
        state_mutex.lock();
        if (c_outputs.count(id) > 0)
        {
            c_outputs[id].output_stream = std::make_shared<NaFiFo>();
            c_outputs[id].enabled = false;

            if (is_running())
                ndsp::buf::init_nafifo_stdbuf<complex_t>(c_outputs[id].output_stream, 2, ((ndsp::buf::StdBuf<complex_t> *)inputs[0]->write_buf())->max);
        }
        state_mutex.unlock();
    }

    // Main
    void Splitter::set_main_enabled(bool enable)
    {
        state_mutex.lock();
        enable_main = enable;
        state_mutex.unlock();
    }

    void Splitter::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<complex_t> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<complex_t> *)outputs[0]->write_buf();

            state_mutex.lock();

            // Main
            if (enable_main)
                memcpy(wbuf->dat, rbuf->dat, rbuf->cnt * sizeof(complex_t));

            // Copy outputs
            for (auto &o : c_outputs)
                if (o.second.enabled)
                    memcpy(((ndsp::buf::StdBuf<complex_t> *)o.second.output_stream->write_buf())->dat, rbuf->dat, rbuf->cnt * sizeof(complex_t));

            inputs[0]->flush();

            if (enable_main)
            {
                wbuf->cnt = rbuf->cnt;
                outputs[0]->write();
            }

            for (auto &o : c_outputs)
            {
                if (o.second.enabled)
                {
                    ((ndsp::buf::StdBuf<complex_t> *)o.second.output_stream->write_buf())->cnt = rbuf->cnt;
                    o.second.output_stream->write();
                }
            }

            state_mutex.unlock();
        }
    }
}
