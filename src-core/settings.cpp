#define SATDUMP_DLL_EXPORT 1
#include "settings.h"
#include "logger.h"
#include <fstream>

SATDUMP_DLL nlohmann::json settings;

std::string settings_path;

void loadSettings(std::string path)
{
    settings_path = path;
    logger->info("Loading settings from file " + path);

    std::ifstream istream(path);
    istream >> settings;
    istream.close();
}

void saveSettings()
{
    std::ofstream ostream(settings_path);
    ostream << settings.dump(4);
    ostream.close();
}