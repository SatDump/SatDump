#pragma once

#include "common/dsp_source_sink/format_notated.h"
#include "dsp/benchmark/bench.h"
#include "dsp/block.h"
#include "logger.h"
#include "utils/time.h"

namespace satdump
{
    namespace ndsp
    {
#define benchmarkLegacyBlock(T_IN, T_OUT, T_BLK, DURATION, BLOCKNAME, MULTIPICATOR, ...)                                                                                                               \
    {                                                                                                                                                                                                  \
        std::shared_ptr<dsp::stream<T_IN>> input_stream = std::make_shared<dsp::stream<T_IN>>();                                                                                                       \
                                                                                                                                                                                                       \
        T_BLK *block_to_test = new T_BLK(input_stream, __VA_ARGS__);                                                                                                                                   \
                                                                                                                                                                                                       \
        bool should_run = true;                                                                                                                                                                        \
        auto producer = [input_stream, &should_run]()                                                                                                                                                  \
        {                                                                                                                                                                                              \
            while (should_run)                                                                                                                                                                         \
            {                                                                                                                                                                                          \
                for (int i = 0; i < 8192 * 4; i++)                                                                                                                                                     \
                    input_stream->writeBuf[i] = T_IN(i / 1e5);                                                                                                                                         \
                input_stream->swap(8192 * 4);                                                                                                                                                          \
            }                                                                                                                                                                                          \
        };                                                                                                                                                                                             \
                                                                                                                                                                                                       \
        std::thread producer_th(producer);                                                                                                                                                             \
                                                                                                                                                                                                       \
        block_to_test->start();                                                                                                                                                                        \
                                                                                                                                                                                                       \
        double received = 0;                                                                                                                                                                           \
        double start_time = satdump::getTime();                                                                                                                                                        \
        double end_time = satdump::getTime();                                                                                                                                                          \
        while ((end_time - start_time) < DURATION)                                                                                                                                                     \
        {                                                                                                                                                                                              \
            int v = block_to_test->output_stream->read();                                                                                                                                              \
            received += v;                                                                                                                                                                             \
                                                                                                                                                                                                       \
            end_time = satdump::getTime();                                                                                                                                                             \
                                                                                                                                                                                                       \
            block_to_test->output_stream->flush();                                                                                                                                                     \
        }                                                                                                                                                                                              \
                                                                                                                                                                                                       \
        should_run = false;                                                                                                                                                                            \
        block_to_test->stop();                                                                                                                                                                         \
        input_stream->stopReader();                                                                                                                                                                    \
        input_stream->stopWriter();                                                                                                                                                                    \
                                                                                                                                                                                                       \
        if (producer_th.joinable())                                                                                                                                                                    \
            producer_th.join();                                                                                                                                                                        \
                                                                                                                                                                                                       \
        double throughput = received / (end_time - start_time);                                                                                                                                        \
        throughput *= MULTIPICATOR;                                                                                                                                                                    \
        logger->warn((std::string) "Performance (" + BLOCKNAME + ") %s", format_notated(throughput, "S/s").c_str());                                                                                   \
                                                                                                                                                                                                       \
        delete block_to_test;                                                                                                                                                                          \
                                                                                                                                                                                                       \
        res.push_back({(std::string)BLOCKNAME + " (Legacy) - " + format_notated(throughput, "S/s"), throughput});                                                                                      \
    }

        template <typename T>
        DSPBenchResult benchmarkNDSPBlock(std::shared_ptr<Block> ptr, int duration, std::string blkname, double ratio = 1)
        {
            BlockIO istr;
            istr.fifo = std::make_shared<DSPStream>(4); // TODOREWORK FIFONUM?
            istr.name = "o";
            istr.type = getTypeSampleType<T>();

            ptr->set_input(istr, 0);

            std::atomic<bool> should_run = true;
            auto producer = [&istr, &should_run]()
            {
                while (should_run)
                {
                    DSPBuffer nbuf = istr.fifo->newBufferSamples(8192 * 4, sizeof(T));
                    for (int i = 0; i < 8192 * 4; i++)
                        nbuf.getSamples<T>()[i] = T(i / 1e5);
                    nbuf.size = 8192 * 4;
                    istr.fifo->wait_enqueue(nbuf);
                }

                istr.fifo->wait_enqueue(istr.fifo->newBufferTerminator());
            };

            std::thread producer_th(producer);

            auto blk_out = ptr->get_output(0, 4 /*TODOREWORK*/);

            ptr->start();

            double received = 0;
            double start_time = getTime();
            double end_time = getTime();
            auto p2 = [&]()
            {
                while (true)
                {
                    DSPBuffer iblk = blk_out.fifo->wait_dequeue();

                    if (iblk.isTerminator())
                        break;

                    received += iblk.size;

                    end_time = satdump::getTime();

                    blk_out.fifo->free(iblk);
                }
            };
            std::thread consumer_th(p2);

            std::this_thread::sleep_for(std::chrono::seconds(duration));
            should_run = false;

            ptr->stop();
            if (producer_th.joinable())
                producer_th.join();
            if (consumer_th.joinable())
                consumer_th.join();

            double throughput = received / (end_time - start_time);
            throughput *= ratio;
            logger->warn((std::string) "Performance (" + blkname + ") %s", format_notated(throughput, "S/s").c_str());
            return {blkname + " - " + format_notated(throughput, "S/s"), throughput};
        }
    } // namespace ndsp
} // namespace satdump