#pragma once

#include "core/module.h"
#include "image/infrared1_reader.h"
#include "image/infrared2_reader.h"
#include "image/sounder_reader.h"
#include "image/visible_reader.h"
#include <fstream>

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
            int vis_height;
            time_t image_time;
            int vis_xoff;
            int vis_yoff;
            float subsatlon;
        };

        struct Block
        {
            uint8_t block_id;
            uint32_t original_counter;
            uint8_t *frame;
        };

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

            void saveReceivedImage(time_t &image_time);

            // Time when processing was started
            time_t image_time_fallback;

            // Stats
            std::vector<int> scid_stats;
            std::vector<int> /*vis_width_stats,*/ ir_width_stats;
            std::vector<int> block_zero_timestamps;
            std::vector<int> block_zero_x_offset;
            std::vector<int> block_zero_y_offset;
            std::vector<int> block_zero_x_size;
            std::vector<int> block_zero_y_size;
            std::vector<int> block_zero_subsat_lon;

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
    } // namespace gvar
} // namespace goes
