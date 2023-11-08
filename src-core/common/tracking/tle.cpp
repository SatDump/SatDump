#define SATDUMP_DLL_EXPORT 1
#include "tle.h"
#include <fstream>
#include "common/utils.h"
#include "logger.h"
#include "core/config.h"
#include <filesystem>
#include "core/plugin.h"
#include <thread>
#include "nlohmann/json_utils.h"

namespace satdump
{
    TLERegistry general_tle_registry;

    int parseTLEStream(std::istream &inputStream, TLERegistry &new_registry)
    {
        std::string this_line;
        std::deque<std::string> tle_lines;
        int total_lines = 0;
        while (std::getline(inputStream, this_line))
        {
            this_line.erase(0, this_line.find_first_not_of(" \t\n\r"));
            this_line.erase(this_line.find_last_not_of(" \t\n\r") + 1);
            if(this_line != "")
                tle_lines.push_back(this_line);

            if (tle_lines.size() == 3)
            {
                bool checksum_good = true;
                for (int i = 1; i < 3; i++)
                {
                    if (!isdigit(tle_lines[i].back()) || !isdigit(tle_lines[i].front()) || tle_lines[i].front() - '0' != i)
                    {
                        checksum_good = false;
                        break;
                    }

                    int checksum = tle_lines[i].back() - '0';
                    int actualsum = 0;
                    for (int j = 0; j < (int)tle_lines[i].size() - 1; j++)
                    {
                        if (tle_lines[i][j] == '-')
                            actualsum++;
                        else if (isdigit(tle_lines[i][j]))
                            actualsum += tle_lines[i][j] - '0';
                    }

                    checksum_good = checksum == actualsum % 10;
                    if (!checksum_good)
                        break;
                }
                if (!checksum_good)
                {
                    tle_lines.pop_front();
                    continue;
                }

                int norad;
                try
                {
                    std::string noradstr = tle_lines[2].substr(2, tle_lines[2].substr(2, tle_lines[2].size() - 1).find(' '));
                    norad = std::stoi(noradstr);
                }
                catch (std::exception&)
                {
                    tle_lines.pop_front();
                    continue;
                }

                new_registry.push_back({norad, tle_lines[0], tle_lines[1], tle_lines[2]});
                tle_lines.clear();
                total_lines++;
            }
        }

        //Sort and remove duplicates
        sort(new_registry.begin(), new_registry.end(), [](TLE& a, TLE& b) { return a.name < b.name; });
        new_registry.erase(std::unique(new_registry.begin(), new_registry.end(), [](TLE& a, TLE& b) { return a.norad == b.norad; }),
            new_registry.end());

        return total_lines;
    }

    void updateTLEFile(std::string path)
    {
        std::vector<int> norads_to_fetch = config::main_cfg["tle_settings"]["tles_to_fetch"].get<std::vector<int>>();
        std::vector<std::string> urls_to_fetch = config::main_cfg["tle_settings"]["urls_to_fetch"].get<std::vector<std::string>>();

        if (!std::filesystem::exists(std::filesystem::path(path).parent_path()))
            std::filesystem::create_directories(std::filesystem::path(path).parent_path());

        bool success = true;
        TLERegistry new_registry;

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
                    std::istringstream tle_stream(result);
                    success = parseTLEStream(tle_stream, new_registry) > 0;
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
            {
                logger->warn("Failed to get TLE for %s", url_str.c_str());
                break;
            }
        }

        if (success)
        {
            std::ofstream outfile(path, std::ios::trunc);
            for (TLE& tle : new_registry)
                outfile << tle.name << std::endl << tle.line1 << std::endl << tle.line2 << std::endl << std::endl;
            outfile.close();
            general_tle_registry = new_registry;
            config::main_cfg["user"]["tles_last_updated"] = time(NULL);
            config::saveUserConfig();
            logger->info("%zu TLEs loaded!", new_registry.size());
            eventBus->fire_event<TLEsUpdatedEvent>(TLEsUpdatedEvent());
        }
        else
            logger->error("Error updating TLEs. Not updated.");
    }

    void autoUpdateTLE(std::string path)
    {
        std::string update_setting = getValueOrDefault<std::string>(config::main_cfg["satdump_general"]["tle_update_interval"]["value"], "1 day");
        time_t next_update = getValueOrDefault<time_t>(config::main_cfg["user"]["tles_last_updated"], 0);
        bool honor_setting = true;
        if (update_setting == "Never")
            honor_setting = false;
        else if (update_setting == "4 hours")
            next_update += 14400;
        else if (update_setting == "1 day")
            next_update += 86400;
        else if (update_setting == "3 days")
            next_update += 259200;
        else if (update_setting == "7 days")
            next_update += 604800;
        else if (update_setting == "14 days")
            next_update += 1209600;
        else
        {
            logger->error("Invalid TLE Auto-update interval: %s", update_setting.c_str());
            honor_setting = false;
        }

        if ((honor_setting && time(NULL) > next_update) || satdump::general_tle_registry.size() == 0)
            updateTLEFile(path);
    }

    void loadTLEFileIntoRegistry(std::string path)
    {
        logger->info("Loading TLEs from " + path);
        std::ifstream tle_file(path);
        TLERegistry new_registry;
        parseTLEStream(tle_file, new_registry);
        tle_file.close();
        logger->info("%zu TLEs loaded!", new_registry.size());
        general_tle_registry = new_registry;
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
            std::istringstream tle_stream(result);
            int loaded_count = parseTLEStream(tle_stream, general_tle_registry);
            logger->info("%d TLEs added!", loaded_count);
        }

        eventBus->fire_event<TLEsUpdatedEvent>(TLEsUpdatedEvent());
    }
}
