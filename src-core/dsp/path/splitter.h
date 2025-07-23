#pragma once

#include "common/dsp/complex.h"
#include "dsp/block.h"
#include <mutex>
#include <string>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class SplitterBlock : public Block
        {
        public:
            int p_noutputs = 0;
            std::mutex vfos_mtx;

        public:
            struct IOInfo
            {
                std::string id;
                bool forward_terminator;
            };

        private:
            bool work();

        public:
            SplitterBlock();
            ~SplitterBlock();

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "noutputs", "int");
                p["noutputs"]["disable"] = is_work_running();
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "noutputs")
                    return p_noutputs;
                throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                if (key == "noutputs")
                {
                    int no = p_noutputs;
                    p_noutputs = v;

                    if (p_noutputs < 0)
                        p_noutputs = 0;

                    if (no != p_noutputs)
                    {
                        Block::outputs.clear();
                        for (int i = 0; i < p_noutputs; i++)
                        {
                            BlockIO o = {{"out" + std::to_string(i + 1), std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}};
                            o.blkdata = std::make_shared<IOInfo>(IOInfo{std::to_string(i + 1), true});
                            o.fifo = std::make_shared<DspBufferFifo>(4); // TODOREWORK
                            Block::outputs.push_back(o);
                        }
                    }
                    return RES_IOUPD;
                }
                else
                    throw satdump_exception(key);
                return RES_OK;
            }

        public:
            BlockIO &add_output(std::string id, bool forward_terminator = true)
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                BlockIO o = {{id, std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}};
                o.blkdata = std::make_shared<IOInfo>(IOInfo{id, forward_terminator});
                o.fifo = std::make_shared<DspBufferFifo>(4); // TODOREWORK
                Block::outputs.push_back(o);
                return outputs[outputs.size() - 1];
            }

            void del_output(std::string id, bool send_terminator) // TODOREWORk maybe a terminator_type_none, _propag, etc? Also maybe a handleTerminator doing it?
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                for (int i = 0; i < outputs.size(); i++)
                    if (((IOInfo *)outputs[i].blkdata.get())->id == id)
                    {
                        if (send_terminator)
                            (outputs.begin() + i)->fifo->wait_enqueue(DSPBuffer::newBufferTerminator());
                        outputs.erase(outputs.begin() + i);
                    }
            }
        };
    } // namespace ndsp
} // namespace satdump