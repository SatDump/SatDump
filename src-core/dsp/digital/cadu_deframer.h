#pragma once

#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include "common/dsp/complex.h"
#include "common/dsp/resamp/polyphase_bank.h"
#include "dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        class CADUDeframerBlock : public Block
        {
        private:
            int cadu_size = 8192;
            std::shared_ptr<deframing::BPSK_CCSDS_Deframer> def;

            int def_state = 0;

            bool needs_reinit = false;

        private:
            bool work();

        public:
            CADUDeframerBlock();
            ~CADUDeframerBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "cadu_size", "int");
                add_param_simple(p, "state", "stat");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "cadu_size")
                    return cadu_size;
                else if (key == "state")
                    return def_state;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "cadu_size")
                {
                    cadu_size = v;
                    needs_reinit = true;
                }
                else if (key == "state")
                    ;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump