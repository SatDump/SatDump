#include "logger.h"
#include "init.h"
#include "help_general.h"

void help_general()
{
	logger->error("");
	logger->error("Visit: www.satdump.org");
	logger->error("");
	logger->info("Many usecases of SatDump CLI are cover at the following link");
	logger->debug("www.satdump.org/posts/basic-usage/#cli-this-part-was-made-by-aang23");
	logger->info("");
	logger->info("This is SatDump v" + (std::string)SATDUMP_VERSION);
	logger->info("Live processing");
	logger->debug("	- Usage: satdump live + parameters");
	logger->debug("	- info: use 'satdump live' for more information on 'live' usage and parameters");	
	logger->info("record processing");
	logger->debug("	- Usage: satdump record + parameters");
	logger->debug("	- info: use 'satdump record' for more information on 'record' usage and parameters");	
	logger->info("autotrack feature");
	logger->debug("	- Usage: satdump autotrack + parameters");
	logger->debug("	- info: use 'satdump autotrack' for more information on 'autotrack' usage and parameters");	
	logger->info("SatDump GUI version");
	logger->debug("	- Usage: satdump-ui");
	logger->debug("	- info: GUI version of SatDump");
	logger->info("SDR probe");
	logger->debug("	- Usage: satdump sdr_probe");
	logger->debug("	- info: Return a list of local and remote receivers ");
	logger->info("General help");
	logger->debug("	- Usage: satdump help or -h");
	logger->debug("	- info: Display the general help function");
	logger->info("version of satdump");
	logger->debug("	- Usage: satdump version");
	logger->debug("	- info: Display the current version of SatDump. Also visible above");
	exit(0);
}
