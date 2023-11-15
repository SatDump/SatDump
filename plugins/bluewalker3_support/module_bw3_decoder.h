#pragma once

#include "core/module.h"

namespace bluewalker3
{
    class BW3DecoderModule : public ProcessingModule
    {
    protected:
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        const int d_cadu_size;
        const int d_payload_size;

    public:
        BW3DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}