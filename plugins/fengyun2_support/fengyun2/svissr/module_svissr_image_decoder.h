#pragma once

#include "image/svissr_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include <thread>

namespace fengyun_svissr
{
    class SVISSRImageDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        // Settings
        const std::string sat_name;

        // Read buffer
        uint8_t *frame;

        // Utils values
        bool backwardScan;
        bool writingImage = false;
        int valid_lines;
        float approx_progess;
        bool apply_correction;
        int global_counter;
        bool counter_locked = false;

        struct SVISSRBuffer
        {
            int scid;

            double timestamp;

            image::Image image1;
            image::Image image2;
            image::Image image3;
            image::Image image4;
            image::Image image5;

            std::string directory;
        };

        // Image readers
        SVISSRReader vissrImageReader;

        // Saving is multithreaded
        std::mutex images_queue_mtx;
        std::vector<std::shared_ptr<SVISSRBuffer>> images_queue;
        std::thread images_queue_thread;

        bool images_thread_should_run;
        void image_saving_thread_f()
        {
            while (images_thread_should_run || images_queue.size() > 0)
            {
                images_queue_mtx.lock();
                bool has_images = images_queue.size() > 0;
                images_queue_mtx.unlock();

                if (has_images)
                {
                    images_queue_mtx.lock();
                    auto img = images_queue[0];
                    images_queue.erase(images_queue.begin());
                    images_queue_mtx.unlock();
                    writeImages(*img);
                    continue;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        std::string getSvissrFilename(std::tm *timeReadable, std::string channel);
        void writeImages(SVISSRBuffer &buffer);

        // Stats
        std::vector<int> scid_stats;

        // UI Stuff
        float corr_history_ca[200];
        unsigned int textureID = 0;
        uint32_t *textureBuffer;

    public:
        SVISSRImageDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~SVISSRImageDecoderModule();
        void process();
        void drawUI(bool window);

        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace fengyun_svissr
