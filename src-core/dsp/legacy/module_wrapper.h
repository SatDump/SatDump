#pragma once

#include "common/dsp/complex.h"
#include "common/dsp/resamp/polyphase_bank.h"
#include "dsp/block.h"
#include "pipeline/module.h"
#include <memory>

namespace satdump
{
    namespace ndsp
    {
        class ModuleWrapperBlock : public Block
        {
        public:
            std::string module_id;
            nlohmann::json mod_cfg;
            std::shared_ptr<pipeline::ProcessingModule> mod;

            std::thread modThread;
            bool work2shouldrun;
            std::thread work2Thread;

        private:
            bool work();
            void work2();

        public:
            ModuleWrapperBlock();
            ~ModuleWrapperBlock();

            void start();
            void stop(bool stop_now = false, bool force = false);

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "module", "string");
                if (is_work_running())
                    p["module"]["disabled"] = true;
                add_param_simple(p, "cfg", "json");
                if (is_work_running())
                    p["cfg"]["disabled"] = true;
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "module")
                    return module_id;
                else if (key == "cfg")
                    return mod_cfg;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "module")
                    module_id = v;
                else if (key == "mod_cfg")
                    mod_cfg = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump