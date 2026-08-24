#include "hard2soft.h"

#include "common/dsp/utils/random.h"
#include "logger.h"
#include <cstdint>
#include <fstream>

namespace satdump
{
    void HardToSoftCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_module = app->add_subcommand("hard2soft", "Convert hard bits to soft bits");
        sub_module->add_option("hard_file")->required();
        sub_module->add_option("soft_file")->required();
    }

    void HardToSoftCmdHandler::run(CLI::App *, CLI::App *subcom, bool)
    {
        std::string hpath = subcom->get_option("hard_file")->as<std::string>();
        std::string spath = subcom->get_option("soft_file")->as<std::string>();

        std::ifstream din(hpath, std::ios::binary);
        std::ofstream dou(spath, std::ios::binary);

        logger->info("Converting!");

        dsp::Random r;

        uint8_t buf;
        while (!din.eof())
        {
            din.read((char *)&buf, 1);
            for (int i = 7; i >= 0; i--)
            {
                int8_t v = ((buf >> i) & 1) ? 50 : -50;
                v += r.gasdev() * 10;
                dou.write((char *)&v, 1);
            }
        }

        logger->info("Done!");
    }
} // namespace satdump