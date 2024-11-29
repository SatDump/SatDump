#include "logger.h"

#include "live.h"
#include "offline.h"
#include "record.h"
#include "autotrack/autotrack.h"

#include "sdr_probe.h"
#include "help_general.h"

#include "project/project.h"

int main(int argc, char *argv[])
{
    // Init logger
    initLogger();

    if (argc < 2)
    {
        logger->error("Please specify either live/record or pipeline name!");
		logger->error("Use -h or help for information");
        return 1;
    }

    if (std::string(argv[1]) == "live")
    {
        int ret = main_live(argc, argv);
        if (ret != 0)
            return ret;
    }
    else if (std::string(argv[1]) == "record")
    {
        int ret = main_record(argc, argv);
        if (ret != 0)
            return ret;
    }
    else if (std::string(argv[1]) == "autotrack")
    {
        int ret = main_autotrack(argc, argv);
        if (ret != 0)
            return ret;
    }
    else if (std::string(argv[1]) == "project")
    {
        try
        {
            int ret = main_project(argc - 2, argv + 2);
            if (ret != 0)
                return ret;
        }
        catch (std::exception &e)
        {
            logger->error("Error running project! %s", e.what());
        }
    }
    else if (std::string(argv[1]) == "version" || std::string(argv[1]) == "--v")
    {
        logger->info("This is SatDump v" + (std::string)SATDUMP_VERSION);
        return 0;
    }
    //////////////
    else if (std::string(argv[1]) == "sdr_probe")
    {
        sdr_probe();
    }
    //////////////
	//////////////
    else if ((std::string(argv[1]) == "-h") || (std::string(argv[1]) == "help"))
    {
        help_general();
    }
    //////////////
    else
    {
        int ret = main_offline(argc, argv);
        if (ret != 0)
            return ret;
    }

    logger->info("Done! Goodbye");
}
