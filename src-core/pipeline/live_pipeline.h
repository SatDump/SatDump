#pragma once

#include "libs/ctpl/ctpl_stl.h"
#include "pipeline.h"

namespace satdump
{
    namespace pipeline
    {
        class LivePipeline
        {
        private:
            Pipeline d_pipeline;
            nlohmann::json d_parameters;
            std::string d_output_dir;

        private:
            std::vector<std::future<void>> module_futs;

            void prepare_modules(std::vector<int> mods);
            void prepare_module(std::string id);

        public:
            LivePipeline(Pipeline pipeline, nlohmann::json parameters, std::string output_dir);
            ~LivePipeline();

            std::vector<std::shared_ptr<ProcessingModule>> modules;

            void start(std::shared_ptr<dsp::stream<complex_t>> stream, ctpl::thread_pool &tp, bool server = false); // Start everything locally, "as usual", or in "server" mode
            void start_client(ctpl::thread_pool &tp);                                                               // Start a "client" setup, with no local input stream
            void stop();                                                                                            // Stop everything. The LivePipeline is NOT re-usable

            std::string getOutputFile();

            nlohmann::json getModulesStats();

            void drawUIs();
        };
    } // namespace pipeline
} // namespace satdump