#pragma once

#include "elektro_arktika/ggak/ggak.h"
#include "elektro_arktika/ggak/plot.h"
#include "elektro_arktika/ggak/skl.h"
#include "image/io.h"
#include "logger.h"
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>

namespace elektro_arktika
{
    namespace ggak
    {
        class GGAKIngestor
        {
        private:
            const std::string directory;

            std::mutex day_timer_mtx;
            std::map<std::string, time_t> ongoing_day_timers;

            std::thread bkg_th;

            time_t last_72h_processed = 0;

        public:
            GGAKIngestor(std::string dir) : directory(dir) { bkg_th = std::thread(&GGAKIngestor::proc_thread, this); }

            void work(GGAKFrame *frm)
            {
                double _time = getTimestamp(frm);

                time_t tt = _time;
                tm timeS = *gmtime(&tt);

                std::string dir = directory + "/" + std::to_string(timeS.tm_year + 1900) + "/" + std::to_string(timeS.tm_mon + 1) + "/" + std::to_string(timeS.tm_mday);
                if (!std::filesystem::exists(dir))
                    std::filesystem::create_directories(dir);
                std::ofstream(dir + "/ggak.cadu", std::ios::binary | std::ios::app).write((char *)frm, 224);

                time_t cur_time = time(0);

                day_timer_mtx.lock();
                if (ongoing_day_timers.count(dir) == 0)
                    ongoing_day_timers.emplace(dir, cur_time);
                ongoing_day_timers[dir] = cur_time;
                day_timer_mtx.unlock();
            }

            void proc_thread()
            {
                while (1)
                {
                    sleep(1);

                    time_t cur_time = time(0);

                    day_timer_mtx.lock();
                    for (auto &day : ongoing_day_timers)
                        if (cur_time - day.second > 30)
                        {
                            logger->info("Processing directory " + day.first);
                            process_day_cadu(day.first);
                            ongoing_day_timers.erase(day.first);
                            break;
                        }
                    day_timer_mtx.unlock();

                    if (cur_time - last_72h_processed > 60)
                    {
                        logger->info("Processing last 72h...");
                        process_last_72h(cur_time);
                        last_72h_processed = cur_time;
                    }
                }
            }

            void process_day_cadu(std::string directory)
            {
                std::ifstream fin(directory + "/ggak.cadu", std::ios::binary);
                GGAKFrame frm;

                std::vector<SKLRecord> skl_dat;

                if (!fin.good())
                    return;

                while (!fin.eof())
                {
                    fin.read((char *)&frm, 224);

                    if (frm.id == 48)
                    {
                        auto d = parseSKLRecord(&frm);
                        skl_dat.insert(skl_dat.end(), d.begin(), d.end());
                    }
                }

                std::sort(skl_dat.begin(), skl_dat.end(), [](auto &v1, auto &v2) { return v1.time < v2.time; });

                time_t latest_time = 0;
                for (auto &d : skl_dat)
                    if (d.time > latest_time)
                        latest_time = d.time;

                time_t day_start = latest_time - (latest_time % (3600 * 24));
                time_t day_end = day_start + 3600 * 24;

                std::vector<image::Image> all_skl_images = plotSKLData("ELEKTRO-L2", day_start, day_end, skl_dat);

                for (int i = 0; i < 12; i++)
                    image::save_img(all_skl_images[i], directory + "/SKL_" + std::to_string(i + 1) + ".jpg");
            }

            void process_last_72h(time_t end_time)
            {
                std::vector<SKLRecord> skl_dat;

                for (int i = 0; i < 3; i++)
                {
                    time_t ltime = end_time - i * 24 * 3600;

                    time_t dtime = ltime - (ltime % (3600 * 24));

                    time_t tt = dtime;
                    tm timeS = *gmtime(&tt);

                    std::string dir = directory + "/" + std::to_string(timeS.tm_year + 1900) + "/" + std::to_string(timeS.tm_mon + 1) + "/" + std::to_string(timeS.tm_mday);

                    if (std::filesystem::exists(dir + "/ggak.cadu"))
                    {
                        std::ifstream fin(dir + "/ggak.cadu", std::ios::binary);
                        GGAKFrame frm;

                        if (!fin.good())
                            continue;

                        while (!fin.eof())
                        {
                            fin.read((char *)&frm, 224);

                            if (frm.id == 48)
                            {
                                auto d = parseSKLRecord(&frm);
                                skl_dat.insert(skl_dat.end(), d.begin(), d.end());
                            }
                        }
                    }
                }

                std::sort(skl_dat.begin(), skl_dat.end(), [](auto &v1, auto &v2) { return v1.time < v2.time; });

                time_t latest_time = 0;
                for (auto &d : skl_dat)
                    if (d.time > latest_time)
                        latest_time = d.time;

                time_t day_start = end_time - 3600 * 24 * 2;
                time_t day_end = end_time;

                std::vector<image::Image> all_skl_images = plotSKLData("ELEKTRO-L2", day_start, day_end, skl_dat);

                for (int i = 0; i < 12; i++)
                    image::save_img(all_skl_images[i], directory + "/SKL_" + std::to_string(i + 1) + ".jpg");
            }
        };
    } // namespace ggak
} // namespace elektro_arktika