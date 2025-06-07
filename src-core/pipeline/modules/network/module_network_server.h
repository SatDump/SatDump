#pragma once

#include "pipeline/module.h"
#include <complex>
#include <fstream>

namespace satdump
{
    namespace pipeline
    {
        namespace network
        {
            class NetworkServerModule : public ProcessingModule
            {
            protected:
                uint8_t *buffer;

                std::string mode = "default";

                std::ifstream data_in;
                std::atomic<uint64_t> filesize;
                std::atomic<uint64_t> progress;

                int pkt_size;
                std::string address;
                int port;

            public:
                NetworkServerModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
                ~NetworkServerModule();
                void process();
                void drawUI(bool window);
                std::vector<ModuleDataType> getInputTypes();
                std::vector<ModuleDataType> getOutputTypes();

            public:
                static std::string getID();
                virtual std::string getIDM() { return getID(); };
                static nlohmann::json getParams() { return {}; } // TODOREWORK
                static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            };
        } // namespace network
    } // namespace pipeline
} // namespace satdump