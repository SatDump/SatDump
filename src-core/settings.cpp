#define SATDUMP_DLL_EXPORT 1
#include "settings.h"
#include "logger.h"
#include <fstream>
#include <filesystem>
#include "satdump_vars.h"

SATDUMP_DLL nlohmann::json settings;
SATDUMP_DLL nlohmann::json global_cfg;

std::string settings_path;

void loadConfig()
{
    std::string config_path = "config.json";

    if (std::filesystem::exists("pipelines") && std::filesystem::is_directory("pipelines"))
        config_path = "config.json";
    else
        config_path = (std::string)RESOURCES_PATH + "config.json";

    logger->info("Loading config from file " + config_path);

    if (std::filesystem::exists(config_path))
    {
        std::ifstream istream(config_path);
        istream >> global_cfg;
        istream.close();
    }
    else
    {
        logger->error("No config file found! Using defaults.");
    }
}

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