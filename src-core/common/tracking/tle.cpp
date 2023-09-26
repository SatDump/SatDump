#define SATDUMP_DLL_EXPORT 1
#include "tle.h"
#include <fstream>
#include "common/utils.h"
#include "logger.h"
#include "core/config.h"
#include <filesystem>
#include "core/plugin.h"
#include <thread>

namespace satdump
{
    TLERegistry general_tle_registry;

    void updateTLEFile(std::string path)
    {
        std::vector<int> norads_to_fetch = config::main_cfg["tle_settings"]["tles_to_fetch"].get<std::vector<int>>();
        std::vector<std::string> urls_to_fetch = config::main_cfg["tle_settings"]["urls_to_fetch"].get<std::vector<std::string>>();

        if (!std::filesystem::exists(std::filesystem::path(path).parent_path()))
            std::filesystem::create_directories(std::filesystem::path(path).parent_path());

        std::ofstream outfile(path + ".tmp");
        bool success = true;

        for (int norad : norads_to_fetch)
        {
            std::string url_str = config::main_cfg["tle_settings"]["url_template"].get<std::string>();
            while (url_str.find("%NORAD%") != std::string::npos)
                url_str.replace(url_str.find("%NORAD%"), 7, std::to_string(norad));
            urls_to_fetch.push_back(url_str);
        }

        for (auto &url_str : urls_to_fetch)
        {

            logger->info(url_str);
            std::string result;

            int http_res = 1, trials = 0;
            while (http_res == 1 && trials < 10)
            {
                if ((http_res = perform_http_request(url_str, result)) != 1)
                {
                    outfile << result;
                    success = true;
                }
                else
                    success = false;
                trials++;
                if (!success)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    logger->info("Failed getting TLEs. Retrying...");
                }
            }

            if (!success)
                logger->warn("Failed to get TLE for %s", url_str.c_str());
        }

        outfile.close();

        if (success)
        {
            if (std::filesystem::exists(path))
                std::filesystem::remove(path);
            std::filesystem::rename(path + ".tmp", path);
        }
        else
        {
            logger->error("Error updating TLEs. Not updated.");
        }

        if (std::filesystem::exists(path + ".tmp"))
            std::filesystem::remove(path + ".tmp");
    }

    void loadTLEFileIntoRegistry(std::string path)
    {
        logger->info("Loading TLEs from " + path);

        // Parse
        std::ifstream tle_file(path);

        std::string line;
        int line_count = 0;
        int norad = 0;
        bool found_bad = false;
        std::string name, tle1, tle2;
        while (getline(tle_file, line))
        {
            if (line_count % 3 == 0)
            {
                name = line;
                name.erase(name.end() - 1); // Remove newline
                name.erase(std::find_if(name.rbegin(), name.rend(), [](char &c)
                                        { return c != ' '; })
                               .base(),
                           name.end()); // Remove useless spaces
            }
            else if (line_count % 3 == 1)
            {
                if (line.at(0) != '1')
                {
                    line_count = 0;
                    found_bad = true;
                    continue;
                }
                tle1 = line;
            }
            else if (line_count % 3 == 2)
            {
                if (line.at(0) != '2')
                {
                    line_count = 0;
                    found_bad = true;
                    continue;
                }
                tle2 = line;

                try
                {
                    std::string noradstr = tle2.substr(2, tle2.substr(2, tle2.size() - 1).find(' '));
                    norad = std::stoi(noradstr);
                }
                catch (std::exception&)
                {
                    line_count = 0;
                    found_bad = true;
                    continue;
                }

                // logger->trace("Add satellite " + name);
                general_tle_registry.push_back({norad, name, tle1, tle2});
            }

            line_count++;
        }

        if (found_bad)
            logger->warn("Bad TLEs found! Please verify your TLE file");

        //Sort and remove duplicates
        sort(general_tle_registry.begin(), general_tle_registry.end(), [](TLE& a, TLE& b) { return a.name < b.name; });
        general_tle_registry.erase(std::unique(general_tle_registry.begin(), general_tle_registry.end(), [](TLE& a, TLE& b) { return a.norad == b.norad; }),
            general_tle_registry.end());

        logger->info("%zu TLEs loaded!", general_tle_registry.size());
        eventBus->fire_event<TLEsUpdatedEvent>(TLEsUpdatedEvent());
    }

    void fetchTLENow(int norad)
    {
        std::string url_str = config::main_cfg["tle_settings"]["url_template"].get<std::string>();

        while (url_str.find("%NORAD%") != std::string::npos)
            url_str.replace(url_str.find("%NORAD%"), 7, std::to_string(norad));

        logger->info(url_str);
        std::string result;

        if (perform_http_request(url_str, result) != 1)
        {
            std::vector<std::string> lines = splitString(result, '\n');
            TLE newTLE;
            newTLE.name = lines[0];
            newTLE.name.erase(newTLE.name.end() - 1); // Remove newline
            newTLE.name.erase(std::find_if(newTLE.name.rbegin(), newTLE.name.rend(), [](char &c)
                                           { return c != ' '; })
                                  .base(),
                              newTLE.name.end()); // Remove useless spaces
            newTLE.line1 = lines[1];
            newTLE.line2 = lines[2];

            std::string noradstr = newTLE.line2.substr(2, newTLE.line2.substr(2, newTLE.line2.size() - 1).find(' '));
            newTLE.norad = std::stoi(noradstr);

            general_tle_registry.push_back(newTLE);
        }

        eventBus->fire_event<TLEsUpdatedEvent>(TLEsUpdatedEvent());
    }
}