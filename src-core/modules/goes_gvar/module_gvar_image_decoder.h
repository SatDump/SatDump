#pragma once

#include "module.h"
#include <complex>
#include <fstream>
#include "modules/common/ctpl/ctpl_stl.h"
#include "image/infrared1_reader.h"
#include "image/infrared2_reader.h"
#include "image/visible_reader.h"

namespace goes_gvar
{
    class GVARImageDecoderModule : public ProcessingModule
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
        float approx_progess;

        // Image readers
        InfraredReader1 infraredImageReader1;
        InfraredReader2 infraredImageReader2;
        VisibleReader visibleImageReader;

        // Images used as a buffer when writing it out
        cimg_library::CImg<unsigned short> image1;
        cimg_library::CImg<unsigned short> image2;
        cimg_library::CImg<unsigned short> image3;
        cimg_library::CImg<unsigned short> image4;
        cimg_library::CImg<unsigned short> image5;

        // Saving is multithreaded
        std::shared_ptr<ctpl::thread_pool> imageSavingThreadPool;

        std::string getGvarFilename(std::tm *timeReadable, int channel);
        void writeImages(std::string directory);

    public:
        GVARImageDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~GVARImageDecoderModule();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace elektro_arktika