#pragma once

#include "common/dsp/block.h"
#include "s2_defs.h"

/*
PLHeader synchronization, for DVB-S2 clock-recovered symbols
*/
namespace dvbs2
{
    class S2PLSyncBlock : public dsp::Block<complex_t, complex_t>
    {
    private:
        // Header definitions
        dsp::RingBuffer<complex_t> ring_buffer;
        void work();

        std::thread d_thread2;
        bool should_run2;
        void run2()
        {
            while (should_run2)
                work2();
        }
        void work2();

        s2_sof sof;
        s2_plscodes pls;

        // Utils
        complex_t correlate_sof_diff(complex_t *diffs);
        complex_t correlate_plscode_diff(complex_t *diffs);
        // Return conj(u)*v
        complex_t conjprod(const complex_t &u, const complex_t &v)
        {
            return complex_t(u.real * v.real + u.imag * v.imag,
                             u.real * v.imag - u.imag * v.real);
        }

        // Variables and such
        complex_t *correlation_buffer;

    public:
        int slot_number;
        int raw_frame_size;

        int current_position = -1;
        float thresold = 0.6;
        // float freq = 0;

    public:
        S2PLSyncBlock(std::shared_ptr<dsp::stream<complex_t>> input, int slot_number, bool pilots);
        ~S2PLSyncBlock();

        void start()
        {
            Block::start();
            should_run2 = true;
            d_thread2 = std::thread(&S2PLSyncBlock::run2, this);
        }
        void stop()
        {
            Block::stop();
            should_run2 = false;
            ring_buffer.stopReader();
            ring_buffer.stopWriter();

            if (d_thread2.joinable())
                d_thread2.join();
        }
    };
}