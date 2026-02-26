#include "module_wrapper.h"
#include "common/dsp/block.h"
#include "common/dsp/buffer.h"
#include "common/dsp/window/window.h"
#include "dsp/block.h"
#include "pipeline/module.h"
#include <complex.h>
#include <cstdint>
#include <memory>

namespace satdump
{
    namespace ndsp
    {
        ModuleWrapperBlock::ModuleWrapperBlock() : Block("module_wrapper", {{"in", DSP_SAMPLE_TYPE_S8}}, {{"out", DSP_SAMPLE_TYPE_U8}}) {}

        ModuleWrapperBlock::~ModuleWrapperBlock() {}

        void ModuleWrapperBlock::start()
        {
            mod = pipeline::getModuleInstance(module_id, "", "/tmp/modulewrapper", mod_cfg);
            mod->input_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
            mod->output_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
            mod->setInputType(pipeline::DATA_STREAM);
            mod->setOutputType(pipeline::DATA_STREAM);
            mod->init();
            mod->input_active = true;

            modThread = std::thread([this]() { mod->process(); });

            work2shouldrun = true;
            work2Thread = std::thread(&ModuleWrapperBlock::work2, this);

            Block::start();
        }

        void ModuleWrapperBlock::stop(bool stop_now, bool force)
        {
            Block::stop(stop_now, force);

            mod->input_active = false;
            mod->stop();
            work2shouldrun = false;
            mod->output_fifo->stopReader();
            mod->output_fifo->stopWriter();
            mod->input_fifo->stopReader();
            mod->input_fifo->stopWriter();
            if (work2Thread.joinable())
                work2Thread.join();
            if (modThread.joinable())
                modThread.join();
        }

        bool ModuleWrapperBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                mod->input_active = false;
                mod->stop();
                work2shouldrun = false;
                mod->output_fifo->stopReader();
                mod->output_fifo->stopWriter();
                mod->input_fifo->stopReader();
                mod->input_fifo->stopWriter();
                if (work2Thread.joinable())
                    work2Thread.join();
                if (modThread.joinable())
                    modThread.join();

                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                inputs[0].fifo->free(iblk);
                return true;
            }

            // printf("SEND %d\n", iblk.size);
            mod->input_fifo->write(iblk.getSamples<uint8_t>(), iblk.size);

            inputs[0].fifo->free(iblk);

            return false;
        }

        void ModuleWrapperBlock::work2()
        {
            while (work2shouldrun)
            {
                DSPBuffer oblk = outputs[0].fifo->newBufferSamples(1024, sizeof(uint8_t));

                int got = mod->output_fifo->read(oblk.getSamples<uint8_t>(), oblk.max_size);

                if (got > 0)
                {
                    oblk.size = got;
                    outputs[0].fifo->wait_enqueue(oblk);
                }
                else
                {
                    outputs[0].fifo->free(oblk);
                }
            }
        }
    } // namespace ndsp
} // namespace satdump