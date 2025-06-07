#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"
#include "sd_imager_reader.h"

namespace goes
{
    namespace sd
    {
        class SDImageDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            uint8_t *frame;
            uint16_t *frame_words;

            SDImagerReader img_reader;

        public:
            SDImageDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~SDImageDecoderModule();
            static std::string getGvarFilename(int sat_number, std::tm *timeReadable, std::string channel);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace sd
} // namespace goes