#include "block.h"
#include "core/exception.h"

namespace ndsp
{
    Block::Block(std::string id, std::vector<BlockInOutCfg> incfg, std::vector<BlockInOutCfg> oucfg)
        : d_id(id), d_incfg(incfg), d_oucfg(oucfg)
    {
        inputs.resize(d_incfg.size());
        outputs.resize(d_oucfg.size());

        for (int i = 0; i < d_incfg.size(); i++)
            outputs[i] = std::make_shared<NaFiFo>();
    }

    Block::~Block()
    {
    }

    void Block::connect_input(int in_n, std::shared_ptr<NaFiFo> &out)
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
        d_work_run = true;
        d_work_th = std::thread(&Block::loop, this);
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