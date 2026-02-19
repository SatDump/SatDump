#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pmss_deframer.h"
#include <cstdint>
#include <fstream>
#include <stdint.h>

namespace kanopus
{
    using namespace satdump::pipeline;

    class KanopusVDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        int8_t *soft_buffer;
        uint8_t *prediff_bits_buffer;
        uint8_t *bits_buffer;
        uint64_t *frameshift_buffer;

        int frame_count = 0;
        int pmss_frame_count = 0;

        bool synced = false;
        int skip = 0;
        int bad_sync = 0;

        PMSSDeframer pmss_deframer;

    protected:
        bool hard_symbs = false;
        bool dump_diff = false;
        bool dump_stream = false;

    protected:
        void process_bits(uint8_t *bits, int cnt);
        void write_frame();

    public:
        KanopusVDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~KanopusVDecoderModule();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace kanopus