#pragma once

#include "common/dsp/io/baseband_interface.h"
#include "dsp/block.h"

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

            dsp::BasebandReader baseband_reader;

            bool work();

        public:
            // TODOREWORK
            std::atomic<uint64_t> d_filesize;
            std::atomic<uint64_t> d_progress;
            std::atomic<bool> d_eof;

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

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "file", "string");
                p["file"]["disable"] = is_work_running();
                add_param_simple(p, "type", "string");
                p["type"]["disable"] = is_work_running();
                add_param_simple(p, "buffer_size", "int");
                p["buffer_size"]["disable"] = is_work_running();
                add_param_simple(p, "iq_swap", "bool");
                p["iq_swap"]["disable"] = is_work_running();
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "file")
                    return p_file;
                else if (key == "type")
                    return (std::string)p_type;
                else if (key == "buffer_size")
                    return p_buffer_size;
                else if (key == "iq_swap")
                    return p_iq_swap;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "file")
                    p_file = v;
                else if (key == "type")
                    p_type = v.get<std::string>();
                else if (key == "buffer_size")
                    p_buffer_size = v;
                else if (key == "iq_swap")
                    p_iq_swap = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
