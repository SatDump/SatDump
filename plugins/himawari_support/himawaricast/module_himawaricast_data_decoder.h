#pragma once

#include "common/lrit/lrit_productizer.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "segmented_decoder.h"

namespace himawari
{
    namespace himawaricast
    {
        class HimawariCastDataDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            std::string directory;

            // Image re-composers
            std::map<std::string, SegmentedLRITImageDecoder> segmented_decoders;
            std::map<std::string, std::string> segmented_decoders_filenames;

            ::lrit::LRITProductizer productizer;

            void saveImageP(HIMxRITProductMeta meta, image::Image &img);

        public:
            HimawariCastDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~HimawariCastDataDecoderModule();
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace himawaricast
} // namespace himawari