#pragma once

/**
 * @file processing_handler.h
 */

#include "handler.h"
#include <thread>

namespace satdump
{
    namespace handlers
    {
        /**
         * @brief ProcessingHandler base class
         *
         * Some handlers implement a lot of processing already in order
         * to allow manipulating products, images, or more. Therefore,
         * it'd be stupid not to re-use this code for headless
         * processing as well! :-)
         *
         * This is a common interface Handlers can implement if required
         * that allows implementing processing in a way common to both
         * the UI and headless uses. This implements the multi-threaded
         * processing required in the UI as well.
         *
         * @param is_processing variable to be used in the UI to disable controls
         * while processing is ongoing
         */
        class ProcessingHandler
        {
        public:
            ProcessingHandler() {}
            ~ProcessingHandler()
            {
                if (async_thread.joinable())
                    async_thread.join();
            }

        protected:
            bool is_processing = false;

            /**
             * @brief Actual processing function to be implemented by the child class
             */
            virtual void do_process() = 0;

            std::thread async_thread;

        public:
            /**
             * @brief Performing processing. This is not multi-threaded, intended to call it externally
             */
            void process()
            {
                is_processing = true;
                do_process();
                is_processing = false;
            }

            /**
             * @brief Perform processing asynchronously, in a thread
             */
            void asyncProcess()
            {
                if (is_processing && async_thread.joinable())
                {
                    printf("ALREADY PROCESSING!!!!\n"); // TODOREWORK
                    return;
                }

                try
                {
                    async_thread.join();
                }
                catch (std::exception &e)
                {
                }

                is_processing = true;
                auto fun = [this]() { process(); };
                async_thread = std::thread(fun);
            }

            static std::string getID();
            static std::shared_ptr<Handler> getInstance();
        };
    } // namespace handlers
} // namespace satdump