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

#include <functional>

template <typename T>
struct StandardBuffer
{
    uint64_t max;
    uint64_t cnt;
    T *dat;
};

template <typename T>
std::shared_ptr<NaFiFo> create_nafifo_standardbuffer(int size)
{
    auto alloc = [size]() -> void *
    {
        StandardBuffer<T> *ptr = new StandardBuffer<T>;
        ptr->max = size;
        ptr->cnt = 0;
        ptr->dat = new T[size];
        return ptr;
    };

    auto dealloc = [](void *p) -> void
    {
        StandardBuffer<T> *ptr = (StandardBuffer<T> *)p;
        delete[] ptr->dat;
        delete ptr;
    };

    auto ptr = std::make_shared<NaFiFo>();

    ptr->init(100, sizeof(T), alloc, dealloc);

    return ptr;
}

class TestBlock : public ndsp::Block
{
private:
    void work()
    {
        if (!inputs[0]->read())
        {
            StandardBuffer<float> *rbuf = (StandardBuffer<float> *)inputs[0]->read_buf();
            StandardBuffer<float> *wbuf = (StandardBuffer<float> *)outputs[0]->write_buf();

            for (int i = 0; i < rbuf->cnt; i++)
                wbuf->dat[i] = rbuf->dat[i] * 100;
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
        outputs[0] = create_nafifo_standardbuffer<float>(((StandardBuffer<float> *)inputs[0]->read_buf())->max * 2);
        ndsp::Block::start();
    }
};

int main(int argc, char *argv[])
{
    initLogger();

    std::shared_ptr<NaFiFo> fifo1 = create_nafifo_standardbuffer<float>(8192);

    TestBlock testBlock1;

    testBlock1.connect_input(0, fifo1);
    testBlock1.start();

    bool should_run = true;

    std::thread thread_tx([fifo1, &should_run]()
                          {
        while(should_run) {
            ((StandardBuffer<float>*) fifo1->read_buf())->dat[0] = 1;
             ((StandardBuffer<float>*) fifo1->read_buf())->dat[1] = 2;
            ((StandardBuffer<float>*) fifo1->read_buf())->cnt = 2;
            fifo1->write();
        } });

    std::thread thread_rx([&testBlock1]()
                          {
                              while (!testBlock1.outputs[0]->read())
                              {
                                  StandardBuffer<float> *wbuf = (StandardBuffer<float> *)testBlock1.outputs[0]->read_buf();
                                  for (int i = 0; i < wbuf->cnt; i++)
                                      printf("%f, ", wbuf->dat[i]);
                                  printf("\n");
                                  testBlock1.outputs[0]->flush();
                              } });

    sleep(2);

    should_run = false;
    fifo1->stop();
    if (thread_tx.joinable())
        thread_tx.join();

    testBlock1.stop();

    if (thread_rx.joinable())
        thread_rx.join();
}