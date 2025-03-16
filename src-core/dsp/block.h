#pragma once

/**
 * @file block.h
 */

#include "base/dsp_buffer.h"
#include "base/readerwritercircularbuffer.h"
#include "core/exception.h"
#include "nlohmann/json.hpp"
#include <vector>
#include <thread>
#include <mutex>

namespace satdump
{
    namespace ndsp // TODOREWORK back to normal DSP!
    {
        typedef moodycamel::BlockingReaderWriterCircularBuffer<DSPBuffer> DspBufferFifo;

        /**
         * @brief Block IO types.
         *
         * This is helpful to enforce & check type compatibility
         * whenever that may be needed
         */
        enum BlockIOType
        {
            DSP_SAMPLE_TYPE_COMPLEX,
            DSP_SAMPLE_TYPE_FLOAT,
        };

        /**
         * @brief Block IO helper class
         *
         * @param name of the input/output. Mostly meant for when blocks
         * are rendered in a flowchart
         * @param type of the data/samples expected on this IO
         * @param fifo the actual fifo to connect to other blocks
         */
        struct BlockIO
        {
            std::string name;
            BlockIOType type;
            std::shared_ptr<DspBufferFifo> fifo = nullptr;
        };

        /**
         * @brief Base Block class.
         *
         * This has several goals :
         *
         * - First this simply provides an easy-to-use base to implement most
         * DSP blocks, with inputs/outputs passing pointers around, as well
         * as an internal thread to run the actual loop,
         *
         * - Secondly, unlike the previous system this class' interface alone
         * is meant to be enough to perform ALL block functions, from
         * configurations to starting/stopping and so on, as well as entirely
         * abstracting types used in IO streams.
         *
         * See each function's documentation for more information!
         *
         * @param d_id the block's ID
         * @param inputs the block's inputs
         * @param outputs the block's outputs
         */
        class Block
        {
        public:
            const std::string d_id;
            std::vector<BlockIO> inputs;
            std::vector<BlockIO> outputs;

        private:
            bool blk_should_run;
            std::mutex blk_th_mtx;
            std::thread blk_th;
            void run()
            {
                while (blk_should_run)
                    if (work())
                        break;
            }

        protected:
            /**
             * @brief Used to signal to a block it should exit in order
             * to stop(). Usually only needed in sources. TODOREWORK
             */
            bool work_should_exit;

            /**
             * @brief The actual looping work function meant to handle
             * all the DSP (well, in most blocks)
             *
             * @return true if block's thread should exit, usually upon
             * receiving a terminator in the input stream
             */
            virtual bool work() = 0;

        public:
            /**
             * @brief Generic constructor, to be overloaded.AGCBlock
             * @param id of the block, meant to be a short identifier (eg, agc_cc)
             * @param in Inputs configurations, optional
             * @param out Output configurations, optional
             */
            Block(std::string id, std::vector<BlockIO> in = {}, std::vector<BlockIO> out = {})
                : d_id(id), inputs(in), outputs(out)
            {
                blk_should_run = false;
                work_should_exit = false;
            }

            ~Block()
            {
                if (blk_should_run || blk_th.joinable())
                    throw satdump_exception("Block wasn't properly stopped before destructor was called!");
            }

        public:
            /**
             * @brief Get parameters of the block as JSON
             */
            virtual nlohmann::json getParams() = 0;

            /**
             * @brief Set parameters of the block from JSON,
             * including potentially IO configurations for blocks
             * that may have variable output sizes. However,
             * you likely should implemenet that in a separate
             * function as well (eg, addVFO or such) for it to be
             * easy to be done in C++ directly, and using said
             * function here.
             * Optionally, this can also be made to be functional
             * while the block is running!
             */
            virtual void setParams(nlohmann::json v) = 0;

        protected:
            template <typename T>
            void setValFromJSONIfExists(T &v, nlohmann::json p)
            {
                if (!p.is_null())
                    v = p;
            }

        public:
            /**
             * @brief Applies current parameters to the block.
             * This is called automatically once in start(), but
             * may also be called manually after parameters have
             * been changed to apply them while the block is
             * running (given the block supports doing that, which
             * is optional!)
             * /!\ Do note you should NOT make any changing to the
             * block's IO in this functions! IO config should be done
             * in setParams()!
             */
            virtual void init() {} //= 0; TODOREWORK

            /**
             * @brief Starts this block's internal thread and loop.
             */
            virtual void start()
            {
                if (blk_should_run || blk_th.joinable())
                    throw satdump_exception("Block wasn't properly stopped before start() was called again!");

                init();
                work_should_exit = false;
                blk_should_run = true;
                blk_th = std::thread(&Block::run, this);
#ifndef _WIN32
                pthread_setname_np(blk_th.native_handle(), d_id.c_str());
#endif
            }

            /**
             * @brief Stops the block, or rather tells the internal
             * loop it should exit & joins the thread to wait.
             * TODOREWORK, potentially allow sending the terminator
             * as well to force-quit.
             */
            virtual void stop(bool stop_now = false)
            { // TODOREWORK allow sending terminator in this function?
                if (stop_now)
                    work_should_exit = true;

                blk_th_mtx.lock();
                if (blk_th.joinable())
                    blk_th.join();
                blk_th_mtx.unlock();
                blk_should_run = false;
            }
        };
    }
}