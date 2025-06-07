#pragma once

#include "common/dsp/block.h"
#include "common/dsp/complex.h"
#include "common/dsp/io/baseband_type.h"
#include "dsp/block.h"
#include <cstdint>
#include <cstdio>

namespace satdump
{
    namespace ndsp
    {
        class IQSinkBlock : public Block
        {
        public:
            size_t total_written_raw;
            size_t total_written_bytes_compressed;

        private:
            int buffer_size = 1024 * 1024; // TODOREWORK
            std::string filepath;
            dsp::BasebandType format;
            bool autogen = false;
            double samplerate = 1e6;
            double frequency = 100e6;
            double timestamp = 0;

            FILE *file_stream = nullptr;
            void *buffer_convert = nullptr;

            bool work();

        public:
            void start();
            void stop();

        public:
            IQSinkBlock();
            ~IQSinkBlock();

            void init() {}

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "buffer_size")
                    return filepath;
                else if (key == "path")
                    return filepath;
                else if (key == "format")
                    return format;
                else if (key == "autogen")
                    return autogen;
                else if (key == "samplerate")
                    return samplerate;
                else if (key == "frequency")
                    return frequency;
                else if (key == "timestamp")
                    return timestamp;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "buffer_size")
                    buffer_size = v;
                else if (key == "filepath")
                    filepath = v;
                else if (key == "format")
                    format = v.get<std::string>();
                else if (key == "samplerate")
                    samplerate = v;
                else if (key == "autogen")
                    autogen = v;
                else if (key == "frequency")
                    frequency = v;
                else if (key == "timestamp")
                    timestamp = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }

        public:
            static std::string prepareBasebandFileName(double timeValue_precise, uint64_t samplerate, uint64_t frequency);
        };
    } // namespace ndsp
} // namespace satdump