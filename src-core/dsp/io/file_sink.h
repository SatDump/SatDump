#pragma once

#include "common/utils.h"
#include "dsp/block.h"
#include <fstream>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class FileSinkBlock : public Block
        {
        public:
            std::string p_file = "/tmp/afile";

        private:
            std::ofstream file_writer;

            bool work();

        public:
            FileSinkBlock();
            ~FileSinkBlock();

            void init() { file_writer = std::ofstream(p_file, std::ios::binary); }

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
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "file")
                    p_file = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump