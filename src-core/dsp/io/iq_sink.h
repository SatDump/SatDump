#pragma once

#include "common/dsp/block.h"
#include "common/dsp/complex.h"
#include "dsp/block.h"
#include "iq_types.h"
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
            IQType format;
            bool autogen = false;
            double samplerate = 1e6;
            double frequency = 100e6;
            double timestamp = 0;

            FILE *file_stream = nullptr;
            void *buffer_convert = nullptr;

            bool work();

        public:
            void start();
            void stop(bool stop_now = false, bool force=false);

        public:
            IQSinkBlock();
            ~IQSinkBlock();

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "file", "string");
                p["file"]["disable"] = is_work_running();
                add_param_simple(p, "type", "string");
                p["type"]["disable"] = is_work_running();
                add_param_simple(p, "buffer_size", "int");
                p["buffer_size"]["disable"] = is_work_running();
                add_param_simple(p, "autogen", "bool");
                p["autogen"]["disable"] = is_work_running();
                add_param_simple(p, "samplerate", "float");
                p["samplerate"]["disable"] = is_work_running();
                add_param_simple(p, "frequency", "float");
                p["frequency"]["disable"] = is_work_running();
                add_param_simple(p, "timestamp", "float");
                p["timestamp"]["disable"] = is_work_running();
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "buffer_size")
                    return buffer_size;
                else if (key == "file")
                    return filepath;
                else if (key == "type")
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
                else if (key == "file")
                    filepath = v;
                else if (key == "type")
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