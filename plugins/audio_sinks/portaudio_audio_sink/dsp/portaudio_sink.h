#pragma once

#include "dsp/block.h"
#include <cstdint>
#include <cstdio>
#include <portaudio.h>

namespace satdump
{
    namespace ndsp
    {
        class PortAudioSinkBlock : public Block
        {
        private:
            double samplerate = 48e3;

            std::mutex audio_mtx;
            std::vector<int16_t> audio_buff;

            PaStream *stream;

            bool work();

        public:
            void start();
            void stop(bool stop_now = false);

        public:
            PortAudioSinkBlock();
            ~PortAudioSinkBlock();

            void init() {}

            static int audio_callback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "samplerate", "float");
                p["samplerate"]["disable"] = is_work_running();
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "samplerate")
                    return samplerate;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "samplerate")
                    samplerate = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump