
#pragma once
#include <string>

#include "pipeline/module.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace seawifs {
    class SeaWIFsProcessingModule : public satdump::pipeline::base::FileStreamToFileStreamModule {
    protected:
        uint8_t *prelude;
        uint8_t *frame;

        std::string directory;
        std::vector<uint16_t> imageBuffer;

    private:
        void repack_words_to_16(uint8_t *input, int word_count, uint16_t *output_buffer);
        void write_images(uint32_t reception_timestamp);

    public:
        SeaWIFsProcessingModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SeaWIFsProcessingModule();
        void process();
        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID() { return "module_seawifs_decoder"; }
        virtual std::string getIDM() { return "SeaWIFs processing"; }
        static nlohmann::json getParams() { return {}; }  // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
};  // namespace seawifs