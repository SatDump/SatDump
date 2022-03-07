#include "config.h"
#include "nlohmann/json_utils.h"
#include "logger.h"

namespace satdump
{
    namespace config
    {
        nlohmann::ordered_json cfg;

        void loadConfig(std::string path)
        {
            logger->info("Loading config " + path);
            cfg = loadJsonFile(path);
        }
    }
}