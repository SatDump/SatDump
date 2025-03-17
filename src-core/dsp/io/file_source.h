#pragma once

#include "dsp/block.h"
#include "common/dsp/io/baseband_interface.h"

namespace satdump
{
    namespace ndsp
    {
        class FileSourceBlock : public Block
        {
        public:
            std::string p_file = "/tmp/afile";
            dsp::BasebandType p_type = dsp::CF_32;
            int p_buffer_size = 8192;
            bool p_iq_swap = false;

        private:
            dsp::BasebandType d_type;

            int d_buffer_size;
            bool d_iq_swap;
            std::atomic<bool> d_eof;

            dsp::BasebandReader baseband_reader;

            bool work();

        public:
            // TODOREWORK
            std::atomic<uint64_t> d_filesize;
            std::atomic<uint64_t> d_progress;

        public:
            FileSourceBlock();
            ~FileSourceBlock();

            void init()
            {
                d_type = p_type;
                d_buffer_size = p_buffer_size;
                d_iq_swap = p_iq_swap;

                baseband_reader.set_file(p_file, p_type);

                d_filesize = baseband_reader.filesize;
                d_progress = 0;
                d_eof = false;
            }

            nlohmann::json get_cfg()
            {
                nlohmann::json v;
                v["file"] = p_file;
                v["type"] = (std::string)p_type;
                v["buffer_size"] = p_buffer_size;
                v["iq_swap"] = p_iq_swap;
                return v;
            }

            void set_cfg(nlohmann::json v)
            {
                setValFromJSONIfExists(p_file, v["file"]);
                std::string t = p_type;
                setValFromJSONIfExists(t, v["type"]);
                p_type = t;
                setValFromJSONIfExists(p_buffer_size, v["buffer_size"]);
                setValFromJSONIfExists(p_iq_swap, v["iq_swap"]);
            }
        };
    }
}