#pragma once

#include "module.h"
#include <complex>
#include <fstream>
#include "libs/ctpl/ctpl_stl.h"
#include "image/svissr_reader.h"

namespace fengyun_svissr
{
    class SVISSRImageDecoderModule : public ProcessingModule
    {
    protected:
        // Settings
        const std::string sat_name;

        // Read buffer
        uint8_t *frame;

        std::ifstream data_in;
        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        // Utils values
        bool writingImage;
        bool backwardScan;
        int endCount;
        int nonEndCount;
        int lastNonZero;
        float approx_progess;

        // Image readers
        SVISSRReader vissrImageReader;

        // Images used as a buffer when writing it out
        image::Image<uint16_t> image1;
        image::Image<uint16_t> image2;
        image::Image<uint16_t> image3;
        image::Image<uint16_t> image4;
        image::Image<uint16_t> image5;

        // Saving is multithreaded
        std::shared_ptr<ctpl::thread_pool> imageSavingThreadPool;

        std::string getSvissrFilename(std::tm *timeReadable, std::string channel);
        void writeImages(std::string directory);

        // Stats
        std::vector<int> scid_stats;

        // UI Stuff
        unsigned int textureID = 0;
        uint32_t *textureBuffer;

    public:
        SVISSRImageDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SVISSRImageDecoderModule();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace elektro_arktika