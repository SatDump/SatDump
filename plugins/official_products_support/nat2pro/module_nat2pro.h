#pragma once

#include "core/module.h"

namespace nat2pro
{
    class Nat2ProModule : public ProcessingModule
    {
    public:
        Nat2ProModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
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
