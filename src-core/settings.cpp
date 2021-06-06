#define SATDUMP_DLL_EXPORT 1
#include "settings.h"
#include "logger.h"
#include <fstream>
#include <filesystem>

SATDUMP_DLL nlohmann::json settings;

std::string settings_path;

void loadSettings(std::string path)
{
    settings_path = path;
    logger->info("Loading settings from file " + path);

    if (std::filesystem::exists(path))
    {
        std::ifstream istream(path);
        istream >> settings;
        istream.close();
    }
    else
    {
        logger->error("No settings file found! Using defaults.");
    }
}

void saveSettings()
{
    logger->info("Saving settings to file " + settings_path);

    std::ofstream ostream(settings_path);
    ostream << settings.dump(4);
    ostream.close();
}