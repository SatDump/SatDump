#pragma once

#include "pipeline/module.h"
#include "pipeline/modules/instrument_utils.h"

namespace sstv
{
    class SSTVDecoderModule : public satdump::pipeline::ProcessingModule
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
        std::vector<satdump::pipeline::ModuleDataType> getInputTypes() { return {satdump::pipeline::DATA_FILE, satdump::pipeline::DATA_STREAM}; }
        std::vector<satdump::pipeline::ModuleDataType> getOutputTypes() { return {satdump::pipeline::DATA_FILE}; }

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace sstv