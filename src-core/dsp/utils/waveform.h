#pragma once

#include "common/dsp/utils/random.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class WaveformBlock : public Block
        {
        public:
            std::string p_waveform = "cosine";
            float p_samprate = 48000;
            float p_freq = 10000;
            float p_amp = 1.0;
            float p_phase = 0.0;
            int p_buffer_size = 8192;

            bool needs_reinit = false;

        private:
            std::string d_waveform;
            float d_samprate;
            float d_freq;
            float d_amp;
            float d_phase;
            int d_buffer_size;

            T tmp_val;

            dsp::Random d_rng;

            bool work();

        public:
            WaveformBlock();
            ~WaveformBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "waveform", "string", "Waveform");
                add_param_simple(p, "samprate", "float", "Samplerate");
                add_param_simple(p, "freq", "float", "Frequency");
                add_param_simple(p, "amp", "float", "Amplitude");
                add_param_simple(p, "phase", "float", "Phase");
                add_param_simple(p, "bufs", "int", "Buffer Size");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "waveform")
                    return p_waveform;
                else if (key == "samprate")
                    return p_samprate;
                else if (key == "freq")
                    return p_freq;
                else if (key == "amp")
                    return p_amp;
                else if (key == "phase")
                    return p_phase;
                else if (key == "bufs")
                    return p_buffer_size;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "waveform")
                {
                    p_waveform = v;
                    needs_reinit = true;
                }
                else if (key == "samprate")
                {
                    p_samprate = v;
                    needs_reinit = true;
                }
                else if (key == "freq")
                {
                    p_freq = v;
                    needs_reinit = true;
                }
                else if (key == "amp")
                {
                    p_amp = v;
                    needs_reinit = true;
                }
                else if (key == "phase")
                {
                    p_phase = v;
                    needs_reinit = true;
                }
                else if (key == "bufs")
                {
                    p_buffer_size = v;
                }
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
