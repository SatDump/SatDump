#pragma once

#include "core/module.h"
#include <fstream>
#include "msu_vis_reader.h"
#include "msu_ir_reader.h"

namespace elektro_arktika
{
    namespace msugs
    {
        class MSUGSDecoderModule : public ProcessingModule
        {
        protected:
            std::ifstream data_in;
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Readers
            MSUVISReader vis1_reader;
            MSUVISReader vis2_reader;
            MSUVISReader vis3_reader;
            MSUIRReader infr_reader;

            // Statuses
            instrument_status_t channels_statuses[10] = {DECODING, DECODING, DECODING, DECODING, DECODING, DECODING, DECODING, DECODING, DECODING, DECODING};

        public:
            MSUGSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace msugs
} // namespace elektro_arktika