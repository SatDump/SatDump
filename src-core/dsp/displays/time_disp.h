#pragma once

#include "common/dsp/complex.h"
#include "common/widgets/constellation.h"
#include "dsp/block.h"
#include "imgui/imgui.h"

namespace satdump
{
    namespace ndsp
    {
        class TimeDisplayBlock : public Block
        {
        public:
            std::mutex time_history_mtx;
            std::vector<float> time_history = std::vector<float>(1024);

        public:
            TimeDisplayBlock();
            ~TimeDisplayBlock();

            bool work();

            void draw(ImVec2 size);

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "size", "int");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "size")
                {
                    time_history_mtx.lock();
                    int size = time_history.size();
                    time_history_mtx.unlock();
                    return size;
                }
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "size")
                {
                    time_history_mtx.lock();
                    if ((int)v > 1)
                        time_history.resize(v);
                    time_history_mtx.unlock();
                }
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump