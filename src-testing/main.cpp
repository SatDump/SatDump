/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"

#include "common/dsp/complex.h"
#include "common/ndsp/block.h"
#include <unistd.h>

class TestBlock : public ndsp::Block
{
private:
    void work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<float> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<float> *)outputs[0]->write_buf();

            for (int i = 0; i < rbuf->cnt; i++)
                wbuf->dat[i] = rbuf->dat[i] * 10;
            wbuf->cnt = rbuf->cnt;

            outputs[0]->write();
            inputs[0]->flush();
        }
    }

public:
    TestBlock()
        : ndsp::Block("test_block", {{sizeof(float)}}, {{sizeof(float)}})
    {
    }

    void start()
    {
        ndsp::buf::init_nafifo_stdbuf<float>(outputs[0], 2, ((ndsp::buf::StdBuf<float> *)inputs[0]->read_buf())->max * 2);
        ndsp::Block::start();
    }
};

int main(int argc, char *argv[])
{
    initLogger();

    std::shared_ptr<NaFiFo> fifo1 = std::make_shared<NaFiFo>();
    ndsp::buf::init_nafifo_stdbuf<float>(fifo1, 2, 8192);

    TestBlock testBlock1, testBlock2;

    testBlock1.connect_input(0, fifo1);
    testBlock1.start();

    testBlock2.connect_input(0, testBlock1.outputs[0]);
    testBlock2.start();

    bool should_run = true;

    std::thread thread_tx([fifo1, &should_run]()
                          {
        while(should_run) {
            ((ndsp::buf::StdBuf<float>*) fifo1->read_buf())->dat[0] = 1;
             ((ndsp::buf::StdBuf<float>*) fifo1->read_buf())->dat[1] = 2;
            ((ndsp::buf::StdBuf<float>*) fifo1->read_buf())->cnt = 2;
            fifo1->write();
        } });

    std::thread thread_rx([&testBlock2]()
                          {
                              while (!testBlock2.outputs[0]->read())
                              {
                                 auto *wbuf = (ndsp::buf::StdBuf<float> *)testBlock2.outputs[0]->read_buf();
                                  for (int i = 0; i < wbuf->cnt; i++)
                                      printf("%f, ", wbuf->dat[i]);
                                  printf("\n");
                                  testBlock2.outputs[0]->flush();
                              } });

    sleep(2);

    should_run = false;
    fifo1->stop();
    if (thread_tx.joinable())
        thread_tx.join();

    logger->info("Stopping blocks!");

    testBlock1.stop();

    logger->info("Stopping block 2!");

    testBlock2.stop();

    logger->info("Blocks stopped!");

    if (thread_rx.joinable())
        thread_rx.join();
}