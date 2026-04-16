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
        class SwitchBlock : public Block
        {
        public:
            std::mutex out_mtx;

            int noutputs = 0;
            int active_id = 0;

        private:
            bool work();

        public:
            SwitchBlock();
            ~SwitchBlock();

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                std::lock_guard<std::mutex> lock_mtx(out_mtx);

                nlohmann::ordered_json p;
                add_param_simple(p, "noutputs", "int");
                p["noutputs"]["disable"] = is_work_running();
                add_param_simple(p, "active_id", "int");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "noutputs")
                    return noutputs;
                else if (key == "active_id")
                    return active_id;
                throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                std::lock_guard<std::mutex> lock_mtx(out_mtx);

                if (key == "noutputs")
                {
                    int no = noutputs;
                    noutputs = v;

                    if (noutputs < 0)
                        noutputs = 0;

                    if (no != noutputs)
                    {
                        Block::outputs.clear();
                        for (int i = 0; i < noutputs; i++)
                        {
                            BlockIO o = {"out" + std::to_string(i + 1), getTypeSampleType<T>()};
                            o.fifo = std::make_shared<DSPStream>(4); // TODOREWORK
                            Block::outputs.push_back(o);
                        }
                    }

                    // Safety
                    if (active_id >= outputs.size())
                        active_id = 0;

                    return RES_IOUPD;
                }
                if (key == "active_id")
                {
                    active_id = v;

                    // Safety
                    if (active_id >= outputs.size())
                        active_id = 0;
                }
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump