#pragma once

#include "common/utils.h"
#include "dsp/block.h"
#include <fstream>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class FileSourceBlock : public Block
        {
        public:
            std::string p_file = "/tmp/afile";
            int p_buffer_size = 8192;

        private:
            int d_buffer_size;

            std::atomic<bool> d_eof;

            std::ifstream file_reader;

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
                d_buffer_size = p_buffer_size;

                file_reader = std::ifstream(p_file, std::ios::binary);

                d_filesize = getFilesize(p_file);
                d_progress = 0;
                d_eof = false;
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "file", "string");
                p["file"]["disable"] = is_work_running();
                add_param_simple(p, "buffer_size", "int");
                p["buffer_size"]["disable"] = is_work_running();
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "file")
                    return p_file;
                else if (key == "buffer_size")
                    return p_buffer_size;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "file")
                    p_file = v;
                else if (key == "buffer_size")
                    p_buffer_size = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump