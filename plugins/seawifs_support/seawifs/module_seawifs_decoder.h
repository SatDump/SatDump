
#pragma once
#include <string>

#include "seawifs_headers.h"

#include "pipeline/module.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace seawifs
{
    class SeaWiFSProcessingModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        uint8_t *frame;

        std::vector<double> timestamps;
        std::string directory;
        std::vector<uint16_t> imageBuffer;

    private:
        void repack_words_to_16(uint8_t *input, int word_count, uint16_t *output_buffer);
        void write_images();

    public:
        SeaWiFSProcessingModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SeaWiFSProcessingModule();
        void process();
        void drawUI(bool window);
        nlohmann::json getModuleStats() { return satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats(); };

    public:
        static std::string getID() { return "module_seawifs_decoder"; }
        virtual std::string getIDM() { return "SeaWiFS processing"; }
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}; // namespace seawifs