#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"

namespace ax25
{
    class AX25DecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        int8_t *input_buffer;

        bool d_nrzi;
        bool d_g3ruh;

        int frm_cnt = 0;

    public:
        AX25DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~AX25DecoderModule();
        void process();
        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace ax25
