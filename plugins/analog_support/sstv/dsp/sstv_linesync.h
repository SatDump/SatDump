#pragma once

#include "dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        class SSTVLineSyncBlock : public Block
        {
        private:
            // Durations in seconds
            double samplerate = 48000;
            float line_time = 0.150;
            float sync_time = 0.0105;

            // Frequencies in Hz
            float sync_freq = 1200;
            float pixelmin_freq = 1488; // Should be 1500; but seems distorted... To investigate? TODOREWORK
            float pixelmax_freq = 2300;

        private:
            int line_length;
            int sync_length;

            int sync_line_buffer_len;
            float *sync_line_buffer = nullptr;

            std::vector<float> sync;

            // Calculate output of the quadratic demod for each input frequency
            float calcValFromFreq(float freq) { return (2. * M_PI * freq) / samplerate; }

            // Scale to image pixel
            float minval;
            float minvalimg;
            float maxval;

            // Scale pixel between 2 frequencies
            float lineGetScaledIMG(float v)
            {
                v = (v - minvalimg) / (maxval - minvalimg);
                if (v < 0)
                    v = 0;
                if (v > 1)
                    v = 1;
                return v;
            }

            // Used to skip to the next line in the sliding window
            int skip = -1;

            bool work();

        public:
            SSTVLineSyncBlock();
            ~SSTVLineSyncBlock();

            void init()
            {
                line_length = round(samplerate * line_time);
                sync_length = round(samplerate * sync_time);

                sync_line_buffer_len = line_length * 2; //+ sync_length;
                sync_line_buffer = new float[sync_line_buffer_len];
                memset(sync_line_buffer, 0, sync_line_buffer_len * sizeof(float));

                for (int i = 0; i < sync_length; i++)
                    sync.push_back(calcValFromFreq(1200));

                minval = calcValFromFreq(sync_freq);
                minvalimg = calcValFromFreq(pixelmin_freq);
                maxval = calcValFromFreq(pixelmax_freq);
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "samplerate", "float");
                add_param_simple(p, "line_time", "int");
                add_param_simple(p, "sync_time", "float");
                add_param_simple(p, "sync_freq", "float");
                add_param_simple(p, "pixelmin_freq", "float");
                add_param_simple(p, "pixelmax_freq", "float");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "samplerate")
                    return samplerate;
                else if (key == "line_time")
                    return line_time;
                else if (key == "sync_time")
                    return sync_time;
                else if (key == "sync_freq")
                    return sync_freq;
                else if (key == "pixelmin_freq")
                    return pixelmin_freq;
                else if (key == "pixelmax_freq")
                    return pixelmax_freq;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "samplerate")
                    samplerate = v;
                else if (key == "line_time")
                    line_time = v;
                else if (key == "sync_time")
                    sync_time = v;
                else if (key == "sync_freq")
                    sync_freq = v;
                else if (key == "pixelmin_freq")
                    pixelmin_freq = v;
                else if (key == "pixelmax_freq")
                    pixelmax_freq = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump