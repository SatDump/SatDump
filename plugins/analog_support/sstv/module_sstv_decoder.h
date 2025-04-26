#pragma once

#include "core/module.h"

namespace sstv
{
    class SSTVDecoderModule : public ProcessingModule
    {
    protected:
        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        long d_audio_samplerate;
        std::string d_sstv_mode;

        // UI Stuff
        instrument_status_t sstv_status = DECODING;
        bool has_to_update = false;
        unsigned int textureID = 0;
        uint32_t *textureBuffer = nullptr;

    public:
        SSTVDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SSTVDecoderModule();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes() { return {DATA_FILE, DATA_STREAM}; }
        std::vector<ModuleDataType> getOutputTypes() { return {DATA_FILE}; }

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace sstv