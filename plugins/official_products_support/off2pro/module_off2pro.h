#pragma once

#include "pipeline/module.h"

namespace off2pro
{
    class Off2ProModule : public satdump::pipeline::ProcessingModule
    {
    public:
        Off2ProModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

        double filesize = 0;
        double progress = 0;

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace off2pro
