#pragma once

#include "common/audio/audio_sink.h"
#include "core/config.h"
#include "dsp/block.h"

#include <memory>
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {

        class AudioSinkBlock : public Block
        {
        private:
            bool needs_reinit = false;
            double samplerate = 6e6;
            // int p_buffer_size = 8192;

        public:
            bool play_audio;
            bool enable_audio = true;
            double audio_samplerate = 48e3;

            std::shared_ptr<audio::AudioSink> audio_sink;

        private:
            // std::mutex audio_mutex;
            // int buffer_size = 0;

            bool work();

        public:
            AudioSinkBlock();
            ~AudioSinkBlock();

            void init()
            {
                // buffer_size = p_buffer_size;

                play_audio = satdump::config::main_cfg["user_interface"]["play_audio"]["value"].get<bool>();
                audio_sink = audio::get_default_sink();
                audio_sink->set_samplerate(audio_samplerate);
                audio_sink->start();
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "enable_audio", "bool", "Play Audio");
                add_param_simple(p, "audio_samplerate", "float", "Audio Sample Rate");
                add_param_simple(p, "samplerate", "float", "Sample Rate");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "enable_audio")
                    return enable_audio;
                else if (key == "audio_samplerate")
                    return audio_samplerate;
                else if (key == "samplerate")
                    return samplerate;
                // else if (key == "buffer_size")
                //     return p_buffer_size;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "enable_audio")
                    enable_audio = v;
                else if (key == "audio_samplerate")
                {
                    audio_samplerate = v;
                    needs_reinit = true;
                }
                else if (key == "samplerate")
                {
                    samplerate = v;
                    needs_reinit = true;
                }
                // else if (key == "buffer_size")
                //     p_buffer_size = v;
                else
                    throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
