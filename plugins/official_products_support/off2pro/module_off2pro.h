#pragma once

#include "core/module.h"

namespace off2pro
{
    class Off2ProModule : public ProcessingModule
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
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}
