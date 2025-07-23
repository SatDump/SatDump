#pragma once

/**
 * @file block.h
 */

#include "base/dsp_buffer.h"
#include "base/readerwritercircularbuffer.h"
#include "core/exception.h"
#include "nlohmann/json.hpp"
#include <mutex>
#include <thread>
#include <vector>

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
            DSP_SAMPLE_TYPE_CF32,
            DSP_SAMPLE_TYPE_F32,
            DSP_SAMPLE_TYPE_S8,
        };

        /**
         * @brief Block IO helper class
         *
         * @param name of the input/output. Mostly meant for when blocks
         * are rendered in a flowchart
         * @param type of the data/samples expected on this IO
         * @param fifo the actual fifo to connect to other blocks
         * @param blkdata optional data the blocks can chose to hold in IQ
         */
        struct BlockIO
        {
            std::string name;
            BlockIOType type;
            std::shared_ptr<DspBufferFifo> fifo = nullptr;
            std::shared_ptr<void> blkdata = nullptr;

            // TODOREWORK DOCUMENT
            uint64_t samplerate = 0;
            double frequency = 0;
        };

        // TODOREWORK cleanup!!!!
        inline void add_param_simple(nlohmann::ordered_json &p, std::string id, std::string type, std::string name = "")
        {
            p[id]["type"] = type;
            if (name != "")
                p[id]["name"] = name;
        }

        inline void add_param_range(nlohmann::ordered_json &p, std::string id, std::string type, nlohmann::ordered_json min, nlohmann::ordered_json max, nlohmann::ordered_json step,
                                    std::string name = "")
        {
            p[id]["type"] = type;
            if (name != "")
                p[id]["name"] = name;
            p[id]["range"] = {min, max, step};
        }

        inline void add_param_list(nlohmann::ordered_json &p, std::string id, std::string type, nlohmann::json list, std::string name = "")
        {
            p[id]["type"] = type;
            if (name != "")
                p[id]["name"] = name;
            p[id]["list"] = list;
        }

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

        protected:
            std::vector<BlockIO> inputs;
            std::vector<BlockIO> outputs;

        public:
            /**
             * @brief Get the block's input configurations and
             * streams. You should not modify them.
             * @return The block's input BlockIOs.
             */
            std::vector<BlockIO> get_inputs() { return inputs; }

            /**
             * @brief Get the block's output configurations and
             * streams. You should not modify them.
             * @return The block's output BlockIOs.
             */
            std::vector<BlockIO> get_outputs() { return outputs; }

            /**
             * @brief Link an input to an output stream of some sort.
             * This also checks the type of the BlockIO to ensure
             * (some level of) compatibility.
             * @param f input BlockIO to link to an input
             * @param i index of the input (probably quite often 0)
             */
            void set_input(BlockIO f, int i)
            {
                if (i >= (int)inputs.size())
                    throw satdump_exception("Input index " + std::to_string(i) + " does not exist for " + d_id + "!");
                if (inputs[i].type != f.type)
                    throw satdump_exception("Input type " + std::to_string(inputs[i].type) + " not compatible with " + std::to_string(f.type) + " for " + d_id + "!");
                inputs[i].fifo = f.fifo;
            }

            /**
             * @brief Get one of the block's outputs, creating
             * the fifo it if nbuf != 0
             * @param i index of the output
             * @param nbuf of buffers to setup in the output
             * @return BlockIO struct of the output
             */
            BlockIO get_output(int i, int nbuf)
            {
                if (i >= (int)outputs.size())
                    throw satdump_exception("Ouput index " + std::to_string(i) + " does not exist for " + d_id + "!");
                if (nbuf > 0)
                    outputs[i].fifo = std::make_shared<DspBufferFifo>(nbuf);
                return outputs[i];
            }

            /**
             * @brief Link a block's output to another input,
             * more or less just a warper around set_input and
             * set_output
             * @param ptr block to get the output from
             * @param output_index index of the output to use,
             * from ptr
             * @param input_index index of the input to asign no
             * (this block, NOT ptr)
             * @param nbuf of buffers to setup in the output (see
             * set_input for more info)
             * @return BlockIO struct of the output
             */
            void link(Block *ptr, int output_index, int input_index, int nbuf) { set_input(ptr->get_output(output_index, nbuf), input_index); }

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
             * to stop(). Usually only needed in sources.
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

            // TODOREWORK
            bool is_work_running() { return blk_should_run; }

        public:
            /**
             * @brief Generic constructor, to be overloaded.
             * @param id of the block, meant to be a short identifier (eg, agc_cc)
             * @param in Inputs configurations, optional
             * @param out Output configurations, optional
             */
            Block(std::string id, std::vector<BlockIO> in = {}, std::vector<BlockIO> out = {}) : d_id(id), inputs(in), outputs(out)
            {
                blk_should_run = false;
                work_should_exit = false;
            }

            virtual ~Block()
            {
                if (blk_should_run || blk_th.joinable())
                    throw satdump_exception("Block wasn't properly stopped before destructor was called!");
            }

        public:
            /**
             * @brief set_cfg status.
             * @param RES_OK Applied fine
             * @param RES_LISTUPD get_cfg_list must be pulled again
             * @param RES_IOUPD IO Configuration changed (must ALSO re-pull get_cfg_list!!!)
             * @param RES_ERR Config option not applied
             */
            enum cfg_res_t
            {
                RES_OK = 0,
                RES_LISTUPD = 1,
                RES_IOUPD = 2,
                RES_ERR = 3,
            };

            /**
             * @brief Get parameters *LIST* of the block's
             * parameters. This does not contain actual values,
             * only a description of what is available. This
             * will contains a name, optionally description,
             * its type and range if applicable, string options
             * and so on.
             * This must be re-pulled in several cases :
             * - If set_cfg returns RES_LISTUPD or higher
             * - If the block is started
             * - If the block is stopped
             *
             * @return parameters description
             */
            virtual nlohmann::ordered_json get_cfg_list() { return {}; }

            /**
             * @brief Get parameters of the block as JSON
             * @param key the parameter to retrieve
             * @return requested parameter value
             */
            virtual nlohmann::json get_cfg(std::string key) = 0;

            /**
             * @brief Get parameters of the block as JSON.
             * Unlike get_cfg(key), this returns every single
             * available parameter as declared by get_cfg_list().
             * @return All parameters values
             */
            nlohmann::json get_cfg()
            {
                nlohmann::json p;
                auto v = get_cfg_list();
                for (auto &v2 : v.items())
                    p[v2.key()] = get_cfg(v2.key());
                return p;
            }

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
             * @param key parameter to set
             * @param v value to set
             * @return error code or status.
             */
            virtual cfg_res_t set_cfg(std::string key, nlohmann::json v) = 0;

            /**
             * @brief Set parameters of the block from JSON.
             * Essentially the same as set_cfg(key, v), except
             * this will set any number of them at once.
             * @param v values to set
             * @return highest error code or status
             */
            cfg_res_t set_cfg(nlohmann::json v)
            {
                cfg_res_t r = RES_OK;
                for (auto &i : v.items())
                {
                    cfg_res_t r2 = set_cfg(i.key(), i.value());
                    if (r2 > r)
                        r = r2;
                }
                return r;
            }

        protected:
            template <typename T>
            void setValFromJSONIfExists(T &v, nlohmann::json p)
            {
                if (!p.is_null())
                    v = p.get<T>();
            }

        protected:
            /**
             * @brief Applies current parameters to the block.
             * This is called automatically once in start(), but
             * may also be called manually by the block as needed.
             */
            virtual void init() {}

        public:
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
#ifndef __APPLE__
                std::string th_id = d_id;
                th_id.resize(15);
                pthread_setname_np(blk_th.native_handle(), th_id.c_str());
#endif
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
    } // namespace ndsp
} // namespace satdump