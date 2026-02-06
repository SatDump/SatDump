#pragma once

#include "dsp/block.h"
#include <portaudio.h>

namespace satdump
{
    namespace ndsp
    {
        class PortAudioSourceBlock : public Block
        {
        public:
            double samplerate = 48e3;
            int d_buffer_size = 256;

            PaStream *stream;

            bool work();

        public:
            void start();
            void stop(bool stop_now = false);

        public:
            PortAudioSourceBlock();
            ~PortAudioSourceBlock();

            void init() {}

            bool is_async() { return true; }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "samplerate", "float");
                p["samplerate"]["disable"] = is_work_running();
                add_param_simple(p, "buffer_size", "int");
                p["buffer_size"]["disable"] = is_work_running();
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "samplerate")
                    return samplerate;
                else if (key == "buffer_size")
                    return d_buffer_size;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "samplerate")
                    samplerate = v;
                else if (key == "buffer_size")
                    d_buffer_size = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump