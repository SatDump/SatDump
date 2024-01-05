#pragma once

#include "core/module.h"

namespace geonetcast
{
    class GeoNetCastDecoderModule : public ProcessingModule
    {
    protected:
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        std::string directory;

    public:
        GeoNetCastDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~GeoNetCastDecoderModule();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}