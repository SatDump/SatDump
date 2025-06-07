#pragma once

#include "common/simple_deframer.h"
#include "common/widgets/constellation.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace lucky7
{
    class Lucky7DecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        uint8_t *frame_buffer;

        struct ImagePayload
        {
            int total_chunks;
            std::vector<bool> has_chunks;
            std::vector<uint8_t> payload;

            int get_missing()
            {
                int m = 0;
                for (bool v : has_chunks)
                    m += !v;
                return m;
            }

            int get_present()
            {
                int m = 0;
                for (bool v : has_chunks)
                    m += v;
                return m;
            }
        };

    public:
        Lucky7DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~Lucky7DecoderModule();
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace lucky7
