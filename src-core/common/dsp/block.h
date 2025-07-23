#pragma once

#include "common/dsp/buffer.h"
#include "complex.h"
#include "core/exception.h"
#include "logger.h"
#include <memory>
#include <thread>

#define BRANCHLESS_CLIP(x, clip) (0.5 * (std::abs(x + clip) - std::abs(x - clip)))
#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace dsp
{
    template <typename IN_T, typename OUT_T>
    class Block
    {
    protected:
        std::thread d_thread;
        bool should_run = false;
        virtual void work() = 0;
        bool d_got_input;
        void run()
        {
            while (should_run)
                work();
        }

    public:
        std::shared_ptr<dsp::stream<IN_T>> input_stream;
        std::shared_ptr<dsp::stream<OUT_T>> output_stream;

    public:
        Block(std::shared_ptr<dsp::stream<IN_T>> input) : input_stream(input)
        {
            d_got_input = true;
            output_stream = std::make_shared<dsp::stream<OUT_T>>();
        }
        ~Block()
        {
            if (should_run)
            {
                logger->critical("CRITICAL! BLOCK SHOULD BE STOPPED BEFORE CALLING DESTRUCTOR!");
                stop();
            }
        }
        virtual void start()
        {
            should_run = true;
            d_thread = std::thread(&Block::run, this);
        }
        virtual void stop()
        {
            should_run = false;

            if (d_got_input)
                if (input_stream)
                    input_stream->stopReader();
            if (output_stream)
                output_stream->stopWriter();

            if (d_thread.joinable())
                d_thread.join();
        }
    };

    template <typename IN_T, typename OUT_T>
    class HierBlock
    {
    public:
        std::shared_ptr<dsp::stream<IN_T>> input_stream;
        std::shared_ptr<dsp::stream<OUT_T>> output_stream;

    public:
        HierBlock(std::shared_ptr<dsp::stream<IN_T>> input) : input_stream(input) {}
        ~HierBlock() {}
        virtual void start() = 0;
        virtual void stop() = 0;
    };

    // This is here as many blocks require it
    float branchless_clip(float x, float clip);
    float branched_clip(float x, float clip);
    double hz_to_rad(double freq, double samplerate);
    double rad_to_hz(double rad, double samplerate);
} // namespace dsp