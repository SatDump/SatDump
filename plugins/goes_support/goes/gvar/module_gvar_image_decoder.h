#pragma once

#include "core/module.h"
#include <fstream>
#include "image/infrared1_reader.h"
#include "image/infrared2_reader.h"
#include "image/visible_reader.h"
#include "image/sounder_reader.h"

namespace goes
{
    namespace gvar
    {
        struct GVARImages
        {
            // Images used as a buffer when writing it out
            image::Image image1;
            image::Image image2;
            image::Image image3;
            image::Image image4;
            image::Image image5;
            int sat_number;
            int vis_width;
            time_t image_time;
        };
        struct Block
            {
                uint8_t block_id;
                uint32_t original_counter;
                uint8_t *frame;
            };

        namespace events
        {
            struct GVARSaveChannelImagesEvent
            {
                GVARImages &images;
                std::tm *timeReadable;
                time_t timeUTC;
                std::string directory;
            };

            struct GVARSaveFCImageEvent
            {
                image::Image &false_color_image;
                int sat_number;
                std::tm *timeReadable;
                time_t timeUTC;
                std::string directory;
            };
        }

        class GVARImageDecoderModule : public ProcessingModule
        {
        protected:
            // Read buffer
            uint8_t *frame;

            std::ifstream data_in;
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // Utils values
            bool isImageInProgress, isSavingInProgress;
            float approx_progess;

            // Image readers
            InfraredReader1 infraredImageReader1;
            InfraredReader2 infraredImageReader2;
            VisibleReader visibleImageReader;
            SounderReader sounderReader;

            // Async image writing
            std::string directory;
            std::string directory_snd;
            bool writeImagesAync = false;
            std::thread imageSavingThread;
            std::mutex imageVectorMutex;
            std::vector<GVARImages> imagesVector;
            void writeImages(GVARImages &images, std::string directory);
            void writeSounder(time_t image_time);
            void writeImagesThread();

            int imageFrameCount;
            
            // Time when processing was started
            time_t image_time_fallback;

            // Stats
            std::vector<int> scid_stats;
            std::vector<int> vis_width_stats, ir_width_stats;
            std::vector<time_t> block_zero_timestamps;
            
            // UI Stuff
            unsigned int textureID = 0;
            uint32_t *textureBuffer;

        public:
            GVARImageDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GVARImageDecoderModule();
            static std::string getGvarFilename(int sat_number, std::tm *timeReadable, std::string channel);
            void process();
            void process_frame_buffer(std::vector<Block> &frame_buffer, time_t &image_time);
            void drawUI(bool window);
            std::vector<ModuleDataType> getInputTypes();
            std::vector<ModuleDataType> getOutputTypes();

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    }
}
