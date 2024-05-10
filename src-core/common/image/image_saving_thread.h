#pragma once

#include "image.h"
#include <thread>
#include <mutex>
#include "logger.h"
#include "io.h"

namespace image
{
    class ImageSavingThread
    {
    private:
        std::thread saving_thread;
        std::vector<std::pair<Image, std::string>> queue;
        std::mutex queue_mutex;

        bool thread_should_run = true;
        void actual_saving_thread()
        {
            while (thread_should_run || queue.size() > 0)
            {
                queue_mutex.lock();
                bool has_images = queue.size() > 0;
                queue_mutex.unlock();

                if (has_images)
                {
                    queue_mutex.lock();
                    auto img = queue[0];
                    queue.erase(queue.begin());
                    queue_mutex.unlock();

                    // logger->debug("Queue length %d", queue.size());
                    logger->info("Saving " + img.second);
                    save_img(img.first, img.second);
                    continue;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

    public:
        bool use_thread;

    public:
        ImageSavingThread(bool use_thread) : use_thread(use_thread)
        {
            if (use_thread)
                saving_thread = std::thread(&ImageSavingThread::actual_saving_thread, this);
        }

        ~ImageSavingThread()
        {
            thread_should_run = false;
            if (use_thread && saving_thread.joinable())
                saving_thread.join();
        }

        void push(Image &img, std::string path)
        {
            if (use_thread)
            {
                queue_mutex.lock();
                queue.push_back({img, path});
                queue_mutex.unlock();
            }
            else
            {
                logger->info("Saving " + path);
                save_img(img, path);
            }
        }
    };
}