#include "tle.h"
#include <fstream>
#include "common/utils.h"
#include "logger.h"
#include "core/config.h"

namespace satdump
{
    TLERegistry general_tle_registry;

    void updateTLEFile(std::string path)
    {
        std::vector<int> norads_to_fetch = config::main_cfg["tle_settings"]["tles_to_fetch"].get<std::vector<int>>();

        std::ofstream outfile(path);

        for (int norad : norads_to_fetch)
        {
            std::string url_str = config::main_cfg["tle_settings"]["url_template"].get<std::string>();

            while (url_str.find("%NORAD%") != std::string::npos)
                url_str.replace(url_str.find("%NORAD%"), 7, std::to_string(norad));

            logger->info(url_str);
            std::string result;

            if (perform_http_request(url_str, result) != 1)
                outfile << result;
        }
    }

    void loadTLEFileIntoRegistry(std::string path)
    {
        logger->info("Loading TLEs from " + path);

        // Parse
        std::ifstream tle_file(path);

        std::string line;
        int line_count = 0;
        int norad = 0;
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
                tle1 = line;
            }
            else if (line_count % 3 == 2)
            {
                tle2 = line;

                std::string noradstr = tle2.substr(2, tle2.substr(2, tle2.size() - 1).find(' '));
                norad = std::stoi(noradstr);

                logger->trace("Add satellite " + name);
                general_tle_registry.push_back({norad, name, tle1, tle2});
            }

            line_count++;
        }
    }
}