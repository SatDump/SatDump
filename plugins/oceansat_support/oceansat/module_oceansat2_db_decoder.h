#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"

namespace oceansat
{
    class Oceansat2DBDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        int8_t *buffer;

        int frame_count;

        uint8_t dqpsk_demod(int8_t *buffer)
        {
            bool a = buffer[0] > 0;
            bool b = buffer[1] > 0;

            if (a)
            {
                if (b)
                    return 0x0;
                else
                    return 0x3;
            }
            else
            {
                if (b)
                    return 0x1;
                else
                    return 0x2;
            }
        }

    public:
        Oceansat2DBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~Oceansat2DBDecoderModule();
        void process();
        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace oceansat