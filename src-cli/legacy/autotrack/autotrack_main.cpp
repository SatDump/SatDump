#include "autotrack.h"

#include "init.h"
#include "logger.h"
#include <filesystem>
#include <signal.h>

// Catch CTRL+C to exit live properly!
bool autotrack_should_exit = false;
void sig_handler_autotrack(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
        autotrack_should_exit = true;
}

int main_autotrack(int argc, char *argv[])
{
    if (argc < 3) // Check overall command
    {
        logger->error("Usage : " + std::string(argv[0]) + " autotrack autotrack_config.json");
        logger->error("Sample command :");
        logger->error("./satdump autotrack autotrack_config.json");
        return 1;
    }

    if (!std::filesystem::exists(argv[2]))
    {
        logger->critical("%s does not exist! Exiting", argv[2]);
        return 1;
    }

    logger->error("CLI AutoTrack is still WIP!");

    nlohmann::json settings = loadJsonFile(argv[2]);
    nlohmann::json parameters = settings["parameters"];
    std::string output_folder = settings["output_folder"];

    // Create output dir
    if (!std::filesystem::exists(output_folder))
        std::filesystem::create_directories(output_folder);

    AutoTrackApp *app = new AutoTrackApp(settings, parameters, output_folder);

    // Attach signal
    signal(SIGINT, sig_handler_autotrack);
    signal(SIGTERM, sig_handler_autotrack);

    logger->info("Setup Done!");

    // Now, we wait
    while (1)
    {
        if (autotrack_should_exit)
        {
            logger->warn("Signal Received. Stopping.");
            break;
        }

        app->web_heartbeat();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    delete app;

    return 0;
}
