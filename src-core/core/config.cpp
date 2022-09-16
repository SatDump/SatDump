#define SATDUMP_DLL_EXPORT 1
#include "config.h"
#include "nlohmann/json_utils.h"
#include "logger.h"
#include <filesystem>

namespace satdump
{
    namespace config
    {
        nlohmann::ordered_json master_cfg;
        nlohmann::ordered_json main_cfg;

        std::string user_cfg_path;

        void loadConfig(std::string path, std::string user_path)
        {
            logger->info("Loading config " + path);
            master_cfg = loadJsonFile(path);
            main_cfg = master_cfg;

            loadUserConfig(user_path);
        }

        void loadUserConfig(std::string user_path)
        {
            std::string final_path = "";

            if (std::filesystem::exists("settings.json")) // First try loading in current folder
                final_path = "settings.json";
            else if (std::filesystem::exists(user_path + "/settings.json"))
                final_path = user_path + "/settings.json";

            if (final_path != "")
            {
                logger->info("Loading user settings " + final_path);
                nlohmann::ordered_json diff_json = loadJsonFile(final_path);
                main_cfg = merge_json_diffs(master_cfg, diff_json);
                user_cfg_path = final_path;

                if (diff_json.contains("user"))
                    main_cfg["user"] = diff_json["user"];
            }
            else
            {
                logger->warn("No user configuration found! Keeping defaults.");
                user_cfg_path = user_path + "/settings.json";
            }
        }

        void saveUserConfig()
        {
            nlohmann::ordered_json diff_json = perform_json_diff(master_cfg, main_cfg); // Get diff between user settings and the master CFG

            if (main_cfg.contains("user"))
                diff_json["user"] = main_cfg["user"];

            if (!std::filesystem::exists(std::filesystem::path(user_cfg_path).parent_path()) && std::filesystem::path(user_cfg_path).has_parent_path())
                std::filesystem::create_directories(std::filesystem::path(user_cfg_path).parent_path());

            logger->info("Saving user config at " + user_cfg_path);
            saveJsonFile(user_cfg_path, diff_json);
        }
    }
}