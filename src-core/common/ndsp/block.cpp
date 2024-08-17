#include "block.h"
#include "core/exception.h"

namespace ndsp
{
    Block::Block(std::string id, std::vector<BlockInOutCfg> incfg, std::vector<BlockInOutCfg> oucfg)
        : d_id(id), d_incfg(incfg), d_oucfg(oucfg)
    {
        inputs.resize(d_incfg.size());
        outputs.resize(d_oucfg.size());

        for (int i = 0; i < d_oucfg.size(); i++)
            outputs[i] = std::make_shared<NaFiFo>();

        d_work_run = false;
    }

    Block::~Block()
    {
    }

    std::shared_ptr<NaFiFo> Block::get_output(int ou_n)
    {
        if (ou_n >= outputs.size())
            throw satdump_exception("Input index invalid!");
        return outputs[ou_n];
    }

    void Block::set_input(int in_n, std::shared_ptr<NaFiFo> out)
    {
        if (in_n >= inputs.size())
            throw satdump_exception("Input index invalid!");
        if (!out)
            throw satdump_exception("Output stream is not valid!");

        inputs[in_n] = out;
    }

    void Block::loop()
    {
        while (d_work_run)
            work();
    }

    void Block::start()
    {
        set_params();

        d_work_run = true;
        d_work_th = std::thread(&Block::loop, this);
        // #ifndef _WIN32
        // pthread_setname_np(/*d_work_th.native_handle()*/ pthread_self(), d_id.c_str());
        // #endif
    }

    void Block::stop()
    {
        for (auto &o : outputs)
            o->stop();

        d_work_run = false;
        if (d_work_th.joinable())
            d_work_th.join();
    }
}