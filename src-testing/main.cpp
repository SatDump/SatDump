/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include "common/utils.h"

struct CalibChannelCfg
{
    bool valid;
    std::string token;
    std::string channel;
    std::string unit;
    double min;
    double max;
};

CalibChannelCfg tryParse(std::string str)
{
    CalibChannelCfg c;
    c.valid = false;

    c.token = str.substr(0, str.find_first_of('='));
    std::string parenthesis = str.substr(str.find_first_of('(') + 1, str.find_first_of(')') - str.find_first_of('(') - 1);
    auto parts = splitString(parenthesis, ',');

    if (parts.size() == 4)
    {
        c.channel = parts[0];
        c.unit = parts[1];
        c.min = std::stod(parts[2]);
        c.max = std::stod(parts[3]);
    }

    return c;
}

int main(int argc, char *argv[])
{
    initLogger();

    std::string setupEqu = "cch1=(channel_str"; // "cch1=(channel_str,unit_test,13,230)";

    std::string channel = setupEqu.substr(0, setupEqu.find_first_of('='));
    std::string parenthesis = setupEqu.substr(setupEqu.find_first_of('(') + 1, setupEqu.find_first_of(')') - setupEqu.find_first_of('(') - 1);

    logger->trace(channel);
    logger->trace(parenthesis);

    auto parts = splitString(parenthesis, ',');

    if (parts.size() == 4)
    {
        std::string channelstr = parts[0];
        std::string unit = parts[1];
        double min = std::stod(parts[2]);
        double max = std::stod(parts[3]);

        logger->trace("Token " + channel + " : " + unit + " Min " + std::to_string(min) + " Max " + std::to_string(max));
    }
}
