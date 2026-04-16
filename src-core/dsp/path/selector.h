#pragma once

#include "core/exception.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include <mutex>
#include <string>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class SelectorBlock : public Block
        {
        public:
            std::mutex in_mtx;

            int ninputs = 0;
            int active_id = 0;

        private:
            bool work();

        public:
            SelectorBlock();
            ~SelectorBlock();

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                std::lock_guard<std::mutex> lock_mtx(in_mtx);

                nlohmann::ordered_json p;
                add_param_simple(p, "ninputs", "int");
                p["ninputs"]["disable"] = is_work_running();
                add_param_simple(p, "active_id", "int");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "ninputs")
                    return ninputs;
                else if (key == "active_id")
                    return active_id;
                throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                std::lock_guard<std::mutex> lock_mtx(in_mtx);

                if (key == "ninputs")
                {
                    int no = ninputs;
                    ninputs = v;

                    if (ninputs < 0)
                        ninputs = 0;

                    if (no != ninputs)
                    {
                        Block::inputs.clear();
                        for (int i = 0; i < ninputs; i++)
                        {
                            BlockIO o = {"in" + std::to_string(i + 1), getTypeSampleType<T>()};
                            o.fifo = std::make_shared<DSPStream>(4); // TODOREWORK
                            Block::inputs.push_back(o);
                        }
                    }

                    // Safety
                    if (active_id >= inputs.size())
                        active_id = 0;

                    return RES_IOUPD;
                }
                if (key == "active_id")
                {
                    active_id = v;

                    // Safety
                    if (active_id >= inputs.size())
                        active_id = 0;
                }
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump