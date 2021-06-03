#include "tle.h"
#include "resources.h"
#include "logger.h"
#include <map>
#include <fstream>
#include <filesystem>
#include "common/std/binder1st.h"

namespace tle
{
    std::map<int, TLE> tle_map;

    void loadTLEs()
    {
        std::string path = resources::getResourcePath("tle");

        if (std::filesystem::exists(path))
        {
            logger->info("Loading TLEs from " + path);

            std::filesystem::recursive_directory_iterator tleIterator(path);
            std::error_code iteratorError;
            while (tleIterator != std::filesystem::recursive_directory_iterator())
            {
                if (!std::filesystem::is_directory(tleIterator->path()))
                {
                    if (tleIterator->path().filename().string().find(".tle") != std::string::npos)
                    {
                        logger->trace("Found TLE file " + tleIterator->path().string());

                        // Parse
                        std::ifstream tle_file(tleIterator->path().string());

                        std::string line;
                        int line_count = 0;
                        int norad = 0;
                        std::string name, tle1, tle2;
                        while (getline(tle_file, line))
                        {
                            if (line_count % 3 == 0)
                            {
                                name = line;
                                name.erase(std::find_if(name.rbegin(), name.rend(), std2::bind1st(std::not_equal_to<char>(), ' ')).base(), name.end()); // Remove useless spaces
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

                                tle_map.emplace(std::pair<int, TLE>(norad, {norad, name, tle1, tle2}));
                            }

                            line_count++;
                        }
                    }
                }

                tleIterator.increment(iteratorError);
                if (iteratorError)
                    logger->critical(iteratorError.message());
            }
        }
        else
        {
            logger->warn("TLEs not found! Some things may not work.");
        }
    }

    TLE getTLEfromNORAD(int norad)
    {
        if (tle_map.count(norad) > 0)
        {
            return tle_map[norad];
        }
        else
        {
            logger->error("TLE from NORAD " + std::to_string(norad) + " not found!");
            return {norad, "", "", ""};
        }
    }
}