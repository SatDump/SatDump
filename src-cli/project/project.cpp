#include "logger.h"
#include "init.h"
#include "project.h"
#include "common/cli_utils.h"

int main_project(int argc, char *argv[])
{
    // We don't wanna spam with init this time around
    logger->set_level(slog::LOG_ERROR);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    logger->info("Starting projection tool...");

    for (int i = 0; i < argc; i++)
    {
        // logger->debug(argv[i]);

        if (i != argc)
        {
            std::string flag = argv[i];

            if (flag[0] == '-' && flag[1] != '-') // Detect a flag
            {
                flag = flag.substr(1, flag.length()); // Remove the "--"

                //    logger->trace(flag);

                int end = 0;
                for (int ii = i + 1; ii < argc; ii++)
                {
                    end = ii;
                    std::string flag2 = argv[ii];
                    if (flag2[0] == '-' && flag2[1] != '-') // Detect a flag
                        break;
                }

                //  logger->error(end - i);
                auto params = parse_common_flags(end - i, argv + i + 1);
                //     logger->trace("\n" + params.dump(4));

                if (flag == "layer")
                {
                }
                else if (flag == "target")
                {
                }
                else
                {
                    logger->error("Invalid tag. Must be -layer or -target!");
                    return 1;
                }
            }
        }
    }

    return 0;
}
