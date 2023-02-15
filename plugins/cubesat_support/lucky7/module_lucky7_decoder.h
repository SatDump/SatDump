#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "common/simple_deframer.h"
#include "common/widgets/constellation.h"

namespace lucky7
{
    class Lucky7DecoderModule : public ProcessingModule
    {
    protected:
        uint8_t *frame_buffer;

        std::ifstream data_in;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

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
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace noaa
