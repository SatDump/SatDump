#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"
#include "skl.h"

namespace elektro_arktika
{
    namespace ggak
    {
        class GGAKInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            bool is_arktika = false;
            int sat_num = 0;

            std::mutex records_mtx;
            std::vector<SKLRecord> skl_records;

        public:
            GGAKInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace ggak
} // namespace elektro_arktika