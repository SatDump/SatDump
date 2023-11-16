#pragma once

#include "core/module.h"
#include "segmented_decoder.h"

namespace himawari
{
    namespace himawaricast
    {
        class HimawariCastDataDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            std::string directory;

            // Image re-composers
            std::map<std::string, SegmentedLRITImageDecoder> segmented_decoders;
            std::map<std::string, std::string> segmented_decoders_filenames;

        public:
            HimawariCastDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~HimawariCastDataDecoderModule();
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
    } // namespace avhrr
} // namespace metop